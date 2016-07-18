/*
 * Copyright 2015 Hannes Ellinger
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

#include "ubertooth_callback_utils.h"

#include <string.h>

int8_t cc2400_rssi_to_dbm( const int8_t rssi )
{
	/* models the cc2400 datasheet fig 22 for 1M as piece-wise linear */
	if (rssi < -48) {
		return -120;
	}
	else if (rssi <= -45) {
		return 6*(rssi+28);
	}
	else if (rssi <= 30) {
		return (int8_t) ((99*((int)rssi-62))/110);
	}
	else if (rssi <= 35) {
		return (int8_t) ((60*((int)rssi-35))/11);
	}
	else {
		return 0;
	}
}

#define RSSI_HISTORY_LEN 10

/* Ignore packets with a SNR lower than this in order to reduce
 * processor load.  TODO: this should be a command line parameter. */

int8_t rssi_history[NUM_BREDR_CHANNELS][RSSI_HISTORY_LEN] = {{INT8_MIN}};

void determine_signal_and_noise( usb_pkt_rx *rx, int8_t * sig, int8_t * noise )
{
	int8_t * channel_rssi_history = rssi_history[rx->channel];
	int8_t rssi;
	int i;

	/* Shift rssi max history and append current max */
	memmove(channel_rssi_history,
	        channel_rssi_history+1,
	        RSSI_HISTORY_LEN-1);
	channel_rssi_history[RSSI_HISTORY_LEN-1] = rx->rssi_max;

#if 0
	/* Signal starts in oldest bank, but may cross into second
	 * oldest bank.  Take the max or the 2 maxs. */
	rssi = MAX(channel_rssi_history[0], channel_rssi_history[1]);
#else
	/* Alternatively, use all banks in history. */
	rssi = channel_rssi_history[0];
	for (i = 1; i < RSSI_HISTORY_LEN; i++)
		rssi = MAX(rssi, channel_rssi_history[i]);
#endif
	*sig = cc2400_rssi_to_dbm( rssi );

	/* Noise is an IIR of averages */
	/* FIXME: currently bogus */
	*noise = cc2400_rssi_to_dbm( rx->rssi_avg );
}

uint64_t now_ns( void )
{
/* As per Apple QA1398 */
#if defined( __APPLE__ )
	mach_timebase_info_data_t sTimebaseInfo;
	uint64_t ts = mach_absolute_time( );
	if (sTimebaseInfo.denom == 0) {
		(void) mach_timebase_info(&sTimebaseInfo);
	}
	return (ts*sTimebaseInfo.numer/sTimebaseInfo.denom);
#else
	struct timespec ts = { 0, 0 };
	(void) clock_gettime( CLOCK_REALTIME, &ts );
	return (1000000000ull*(uint64_t) ts.tv_sec) + (uint64_t) ts.tv_nsec;
#endif
}

void track_clk100ns( ubertooth_t* ut, const usb_pkt_rx* rx )
{
	/* track clk100ns */
	if (!ut->start_clk100ns) {
		ut->last_clk100ns = ut->start_clk100ns = rx->clk100ns;
		ut->abs_start_ns = now_ns( );
	}
	/* detect clk100ns roll-over */
	if (rx->clk100ns < ut->last_clk100ns) {
		ut->clk100ns_upper += 1;
	}
	ut->last_clk100ns = rx->clk100ns;
}

uint64_t now_ns_from_clk100ns( ubertooth_t* ut, const usb_pkt_rx* rx )
{
	track_clk100ns( ut, rx );
	return ut->abs_start_ns +
	       100ull*(uint64_t)((rx->clk100ns - ut->start_clk100ns) & 0xffffffff) +
	       ((100ull*ut->clk100ns_upper)<<32);
}

#define CLOCK_TRIM_THRESHOLD 2
int trim(ubertooth_t* ut, usb_pkt_rx* rx, int offset)
{
	/* calculate the offset between the first bit of the AC and the rising edge of CLKN */
	uint16_t clk_offset = (le32toh(rx->clk100ns) + offset*10 + 6250 - 4000) % 6250;
	uint32_t clkn = (le32toh(rx->clkn_high) << 20) + (le32toh(rx->clk100ns) + offset*10 - 4000) / 3125;

	if (ut->trim_counter < -CLOCK_TRIM_THRESHOLD
	    || ((clk_offset < CLK_TUNE_TIME) && !ut->calibrated)) {
		printf("offset < CLK_TUNE_TIME\n");
		printf("CLK100ns Trim: %d\n", 6250 + clk_offset - CLK_TUNE_TIME);
		cmd_trim_clock(ut->devh, 6250 + clk_offset - CLK_TUNE_TIME);
		ut->trim_counter = 0;
		if (ut->calibrated) {
			printf("Clock drifted %d in %f s. %d PPM too slow.\n",
			       (clk_offset-CLK_TUNE_TIME),
			       (double)(clkn-ut->clkn_trim)/3200,
			       (clk_offset-CLK_TUNE_TIME) * 320 / (int32_t)(clkn-ut->clkn_trim));
			cmd_fix_clock_drift(ut->devh, (clk_offset-CLK_TUNE_TIME) * 320 / (int32_t)(clkn-ut->clkn_trim));
		}
		ut->clkn_trim = clkn;
		ut->calibrated = 1;
		return 1;
	} else if (ut->trim_counter > CLOCK_TRIM_THRESHOLD
	           || ((clk_offset > CLK_TUNE_TIME) && !ut->calibrated)) {
		printf("offset > CLK_TUNE_TIME\n");
		printf("CLK100ns Trim: %d\n", clk_offset - CLK_TUNE_TIME);
		cmd_trim_clock(ut->devh, clk_offset - CLK_TUNE_TIME);
		ut->trim_counter = 0;
		if (ut->calibrated) {
			printf("Clock drifted %d in %f s. %d PPM too fast.\n",
			       (clk_offset-CLK_TUNE_TIME),
			       (double)(clkn-ut->clkn_trim)/3200,
			       (clk_offset-CLK_TUNE_TIME) * 320 / (clkn-ut->clkn_trim));
			cmd_fix_clock_drift(ut->devh, (clk_offset-CLK_TUNE_TIME) * 320 / (clkn-ut->clkn_trim));
		}
		ut->clkn_trim = clkn;
		ut->calibrated = 1;
		return 1;
	}

	if (clk_offset < CLK_TUNE_TIME - CLK_TUNE_OFFSET) {
		ut->trim_counter--;
		return 1;
	} else if (clk_offset > CLK_TUNE_TIME + CLK_TUNE_OFFSET) {
		ut->trim_counter++;
		return 1;
	} else {
		ut->trim_counter = 0;
	}

	return 0;
}

void dump(FILE* dumpfile, uint32_t systime, usb_pkt_rx* rx)
{
	uint32_t systime_be = htobe32(systime);
	if (dumpfile == NULL) {
		fwrite(&systime_be, sizeof(systime_be), 1, stdout);
		fwrite(rx, sizeof(usb_pkt_rx), 1, stdout);
	} else {
		fwrite(&systime_be, sizeof(systime_be), 1, dumpfile);
		fwrite(rx, sizeof(usb_pkt_rx), 1, dumpfile);
		fflush(dumpfile);
	}
}

void pcap(ubertooth_t* ut, usb_pkt_rx* rx, uint16_t lap, uint8_t uap, btbb_packet* pkt)
{
	uint64_t nowns = now_ns_from_clk100ns( ut, rx );

	int8_t signal_level = rx->rssi_max;
	int8_t noise_level = rx->rssi_min;
	determine_signal_and_noise( rx, &signal_level, &noise_level );

	/* Dump to PCAP/PCAPNG if specified */
	if (ut->h_pcap_bredr) {
		btbb_pcap_append_packet(ut->h_pcap_bredr, nowns,
		                        signal_level, noise_level,
		                        lap, uap, pkt);
	}
	if (ut->h_pcapng_bredr) {
		btbb_pcapng_append_packet(ut->h_pcapng_bredr, nowns,
		                          signal_level, noise_level,
		                          lap, uap, pkt);
	}
}

void print_br(uint32_t systime, usb_pkt_rx* rx, btbb_packet* pkt)
{
	int8_t signal_level = rx->rssi_max;
	int8_t noise_level = rx->rssi_min;
	determine_signal_and_noise( rx, &signal_level, &noise_level );
	int8_t snr = signal_level - noise_level;

	printf("systime=%u ch=%2d LAP=%06x err=%u clkn=%u s=%d n=%d snr=%d\n",
	       systime,
	       btbb_packet_get_channel(pkt),
	       btbb_packet_get_lap(pkt),
	       btbb_packet_get_ac_errors(pkt),
	       btbb_packet_get_clkn(pkt),
	       signal_level,
	       noise_level,
	       snr
	);
}

int find_packet(usb_pkt_rx* rx, uint32_t lap, btbb_packet** pkt)
{
	/* Sanity check */
	if (rx->channel >= NUM_BREDR_CHANNELS)
		return -1;

	if (rx->pkt_type != BR_PACKET)
		return -1;

	if (rx->status & DISCARD)
		return -1;

	char syms[BANK_LEN];
	ubertooth_unpack_symbols((uint8_t*)rx->data, syms);

	/* Pass packet-pointer-pointer so that
	 * packet can be created in libbtbb. */
	int offset = btbb_find_ac(syms, BANK_LEN, lap, max_ac_errors, pkt);
	if (offset < 0)
		return -1;

	uint32_t clkn = (le32toh(rx->clkn_high) << 20) + (le32toh(rx->clk100ns) + offset*10 - 4000) / 3125;

	btbb_packet_set_channel(*pkt, rx->channel);
	btbb_packet_set_clkn(*pkt, clkn);
	btbb_packet_set_modulation(*pkt, BTBB_MOD_GFSK);
	btbb_packet_set_transport(*pkt, BTBB_TRANSPORT_ANY);

	return offset;
}

void fill_packet(usb_pkt_rx* rx, btbb_packet* pkt, int offset)
{
	char syms[BANK_LEN];
	ubertooth_unpack_symbols((uint8_t*)rx->data, syms);

	/* Once offset is known for a valid packet, copy in symbols
	 * and other rx data. CLKN here is the 312.5us CLK27-0. The
	 * btbb library can shift it be CLK1 if needed. */
	btbb_packet_set_symbols(pkt, syms + offset, BANK_LEN - offset);
}
