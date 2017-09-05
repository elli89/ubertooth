/*
 * Copyright 2015 Mike Ryan
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ego.h"
#include <cstdlib>

/*
 * This code performs several functions related to the Yuneec E-GO electric
 * skateboard:
 *
 *  - connection following
 *  - continuous RX on a single channel
 *  - jamming
 */

int enqueue_with_ts(PacketType type, uint8_t* buf, uint32_t ts);
#define CLK100NS (3125*(clkn & 0xfffff) + T0TC)
extern volatile uint32_t clkn; // TODO: replace with timer1
extern volatile uint8_t requested_mode;
extern volatile uint16_t channel;

static const uint16_t channels[4] = { 2408, 2418, 2423, 2469 };

enum class EgoState : size_t {
	INIT = 0,
	START_RX = 1,
	CAP = 2,
	SLEEP = 3,
	START_JAMMING = 4,
	JAMMING = 5,
};

typedef struct {
	EgoState state;
	int channel_index;
	uint32_t sleep_start;
	uint32_t sleep_duration;
	int timer_active;

	// used by jamming
	int packet_observed;
	uint32_t anchor;
} ego_fsm_state_t;

typedef void (*ego_st_handler)(ego_fsm_state_t *);

#define EGO_PACKET_LEN 36
typedef struct _ego_packet_t {
	uint8_t rxbuf[EGO_PACKET_LEN];
	uint32_t rxtime;
} ego_packet_t;

static void ssp_start(void) {
	// make sure the (active low) slave select signal is not active
	DIO_SSEL_SET;

	// enable SSP
	DIO_SSP_CR1 |= SSPCR1_SSE;

	// activate slave select pin
	DIO_SSEL_CLR;
}

static void ssp_stop() {
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable SSP
	DIO_SSP_CR1 &= ~SSPCR1_SSE;
}

static void ego_init(void) {
	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	dio_ssp_init();
}

static void ego_deinit(void) {
	cc2400_strobe(SRFOFF);
	ssp_stop(); // TODO disable SSP
	ICER0 = ICER0_ICE_USB;
}

static void rf_on(void) {
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x04c0); // un-buffered mode, 2FSK
	// 0 00 00 1 001 10 0 00 0 0
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 24 MSB bits of SYNC_WORD
	//      |  | +---------------> 1 byte of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> un-buffered mode
	cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
	cc2400_set(MDMCTRL, 0x0026); // 150 kHz frequency deviation
	cc2400_set(GRDEC,  3);       // 250 kbit

    // 630f9ffe86
	cc2400_set(SYNCH,   0x630f);
	cc2400_set(SYNCL,   0x9ffe);

	while (!(cc2400_status() & XOSC16M_STABLE));

	ssp_start();

	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);

	cc2400_strobe(SRX);
}

static void do_rx(ego_packet_t *packet) {
	int i;
	for (i = 0; i < EGO_PACKET_LEN; i++) {
		// make sure there are bytes ready
		while (!(SSP1SR & SSPSR_RNE)) ;
		packet->rxbuf[i] = (uint8_t)DIO_SSP_DR;
	}
}

static inline int sync_received(void) {
	return cc2400_status() & SYNC_RECEIVED;
}

// sleep for some milliseconds
static void sleep_ms(ego_fsm_state_t *state, uint32_t duration) {
	state->sleep_start = CLK100NS;
	state->sleep_duration = duration * 1000*10;
}

// sleep for some milliseconds relative to the current anchor point
static void sleep_ms_anchor(ego_fsm_state_t *state, uint32_t duration) {
	state->sleep_start = state->anchor;
	state->sleep_duration = duration * 1000*10;
}

static inline int sleep_elapsed(ego_fsm_state_t *state) {
	uint32_t now = CLK100NS;
	if (now < state->sleep_start)
		now += 3276800000;
	return (now - state->sleep_start) >= state->sleep_duration;
}


/////////////
// states

// do nothing
static void nop_state(ego_fsm_state_t *state __attribute__((unused))) {
}

// used in follow and jam mode, override the channel supplied by user
static void init_state(ego_fsm_state_t *state) {
	state->channel_index = 0;
	channel = channels[state->channel_index];
	state->state = EgoState::START_RX;
}

static void start_rf_state(ego_fsm_state_t *state) {
	rf_on();
	state->state = EgoState::CAP;
}

static void cap_state(ego_fsm_state_t *state) {
	ego_packet_t packet;
	packet.rxtime = CLK100NS;

	if (sleep_elapsed(state)) {
		sleep_ms(state, 4);
		state->state = EgoState::SLEEP;
	}

	if (sync_received()) {
		RXLED_SET;
		do_rx(&packet);
		enqueue_with_ts(PacketType::EGO_PACKET, packet.rxbuf, packet.rxtime);
		RXLED_CLR;

		sleep_ms(state, 6);
		state->state = EgoState::SLEEP;
	}

	// kill RF on state change
	if (state->state != EgoState::CAP) {
		cc2400_strobe(SRFOFF);
		ssp_stop();
		state->timer_active = 1;
	}
}

static void sleep_state(ego_fsm_state_t *state) {
	if (sleep_elapsed(state)) {
		// change channel
		state->channel_index = (state->channel_index + 1) % 4;
		channel = channels[state->channel_index];

		// set 7 ms timeout for RX
		sleep_ms(state, 7);
		state->timer_active = 1;

		state->state = EgoState::START_RX;
	}
}

// continuous cap states (reuses START_RX state)
static void continuous_init_state(ego_fsm_state_t *state) {
	state->state = EgoState::START_RX;
}

static void continuous_cap_state(ego_fsm_state_t *state __attribute__((unused))) {
	ego_packet_t packet;
	packet.rxtime = CLK100NS;

	if (sync_received()) {
		RXLED_SET;
		do_rx(&packet);
		enqueue_with_ts(PacketType::EGO_PACKET, packet.rxbuf, packet.rxtime);
		RXLED_CLR;

		// restart cap with radio warm
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));
		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		cc2400_strobe(SRX);
	}
}

// jammer states
static void jam_cap_state(ego_fsm_state_t *state) {
	if (sync_received()) {
		state->state = EgoState::START_JAMMING;
		state->packet_observed = 1;
		state->anchor = CLK100NS;
	}
	if (state->timer_active && sleep_elapsed(state)) {
		state->state = EgoState::START_JAMMING;
		state->packet_observed = 0;
		sleep_ms(state, 11); // 11 ms hop interval
	}

	// state changed, kill radio
	if (state->state != EgoState::CAP) {
		cc2400_strobe(SRFOFF);
		ssp_stop();
	}
}

static void start_jamming_state(ego_fsm_state_t *state) {
#ifdef TX_ENABLE
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x334b); // with PRNG
	// cc2400_set(GRMDM,   0x04e0); // un-buffered mode, 2FSK
	cc2400_set(GRMDM,   0x04c0); // un-buffered mode, 2FSK
	// 0 00 00 1 001 10 0 00 0 0
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 24 MSB bits of SYNC_WORD
	//      |  | +---------------> 1 byte of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> un-buffered mode
	cc2400_set(FSDIV,   channel); // no IF for TX
	cc2400_set(MDMCTRL, 0x0026); // 150 kHz frequency deviation
	cc2400_set(GRDEC,  3);       // 250 kbit
	cc2400_set(FREND, 0xf);

	while (!(cc2400_status() & XOSC16M_STABLE));

	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);

#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif

	cc2400_strobe(STX);
	TXLED_SET;
#endif

	state->state = EgoState::JAMMING;
	sleep_ms_anchor(state, 2);
}

void jamming_state(ego_fsm_state_t *state) {
	if (sleep_elapsed(state)) {
		cc2400_strobe(SRFOFF);
#ifdef UBERTOOTH_ONE
		PAEN_CLR;
#endif
		TXLED_CLR;

		// change channel
		state->channel_index = (state->channel_index + 1) % 4;
		channel = channels[state->channel_index];

		state->state = EgoState::SLEEP;
		sleep_ms_anchor(state, 6);
	}
}

static void jam_sleep_state(ego_fsm_state_t *state) {
	if (sleep_elapsed(state)) {
		state->state = EgoState::START_RX;
		state->timer_active = 1;
		sleep_ms_anchor(state, 11);
	}
}

void ego_main(EgoMode mode) {
	const ego_st_handler *handler; // set depending on mode
	ego_fsm_state_t state;

	state.state = EgoState::INIT;
	state.channel_index = 0;
	state.timer_active = 0;

	// hopping connection following
	static const ego_st_handler follow_handler[] = {
		init_state,
		start_rf_state,
		cap_state,
		sleep_state,
		nop_state,
		nop_state,
		nop_state,
	};

	// continuous rx on a single channel
	static const ego_st_handler continuous_rx_handler[] = {
		continuous_init_state, // do not override user channel
		start_rf_state,
		continuous_cap_state,
		nop_state,
		nop_state,
		nop_state,
	};

	// jamming
	static const ego_st_handler jam_handler[] = {
		init_state,
		start_rf_state,
		jam_cap_state,
		jam_sleep_state,
		start_jamming_state,
		jamming_state,
	};

	switch (mode) {
		case EgoMode::FOLLOW:
			handler = follow_handler;
			break;
		case EgoMode::CONTINUOUS_RX:
			handler = continuous_rx_handler;
			break;
#ifdef TX_ENABLE
		case EgoMode::JAM:
			handler = jam_handler;
			break;
#endif
		default: // should never happen
			requested_mode = MODE_IDLE;
			return;
	}

	ego_init();

	while (1) {
		if (requested_mode != MODE_EGO)
			break;
		handler[(size_t)(state.state)](&state);
	}

	ego_deinit();
}
