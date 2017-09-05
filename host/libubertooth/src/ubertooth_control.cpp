/*
 * Copyright 2010-2013 Michael Ossmann, Dominic Spill
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

#include <string.h>
#include <btbb.h>
#include <iostream>
#include "ubertooth.h"

#define CTRL_IN     (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT    (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)

void Ubertooth::show_libusb_error(int error_code)
{
	std::cerr << "libUSB Error: " << libusb_error_name(error_code) << ": " << libusb_strerror((libusb_error)error_code) << " (" << error_code << ")" << std::endl;
}

void Ubertooth::cmd_callback(struct libusb_transfer* transfer)
{
	if(transfer->status != 0) {
		show_libusb_error(transfer->status);
	}
	libusb_free_transfer(transfer);
}

void Ubertooth::cmd_trim_clock(uint16_t offset)
{
	uint8_t data[2] = {
		(uint8_t)((offset >> 8) & 0xff),
		(uint8_t)((offset >> 0) & 0xff)
	};

	cmd_async(CTRL_OUT, Command::TRIM_CLOCK, 0, 0, data, 2);
}

void Ubertooth::cmd_fix_clock_drift(int16_t ppm)
{
	uint8_t data[2] = {
		(uint8_t)((ppm >> 8) & 0xff),
		(uint8_t)((ppm >> 0) & 0xff)
	};

	cmd_async(CTRL_OUT, Command::FIX_CLOCK_DRIFT, 0, 0, data, 2);
}

int Ubertooth::cmd_ping()
{
	return cmd_sync(CTRL_IN, Command::PING, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_rx_syms()
{
	return cmd_sync(CTRL_OUT, Command::RX_SYMBOLS, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_tx_syms()
{
	return cmd_async(CTRL_OUT, Command::TX_SYMBOLS, 0, 0, NULL, 0);
}

int Ubertooth::cmd_specan(uint16_t low_freq, uint16_t high_freq)
{
	return cmd_sync(CTRL_OUT, Command::SPECAN, low_freq, high_freq, NULL, 0, 1000);
}

int Ubertooth::cmd_led_specan(uint16_t rssi_threshold)
{
	return cmd_sync(CTRL_OUT, Command::LED_SPECAN, rssi_threshold, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_set_usrled(uint16_t state)
{
	return cmd_sync(CTRL_OUT, Command::SET_USRLED, state, 0, NULL, 0, 1000);
}

uint8_t Ubertooth::cmd_get_usrled()
{
	uint8_t state;

	cmd_sync(CTRL_IN, Command::GET_USRLED, 0, 0, &state, 1, 1000);
	return state;
}

int Ubertooth::cmd_set_rxled(uint16_t state)
{
	return cmd_sync(CTRL_OUT, Command::SET_RXLED, state, 0, NULL, 0, 1000);
}

uint8_t Ubertooth::cmd_get_rxled()
{
	uint8_t state;

	cmd_sync(CTRL_IN, Command::GET_RXLED, 0, 0, &state, 1, 1000);
	return state;
}

int Ubertooth::cmd_set_txled(uint16_t state)
{
	return cmd_sync(CTRL_OUT, Command::SET_TXLED, state, 0, NULL, 0, 1000);
}

uint8_t Ubertooth::cmd_get_txled()
{
	uint8_t state;

	cmd_sync(CTRL_IN, Command::GET_TXLED, 0, 0, &state, 1, 1000);
	return state;
}

uint8_t Ubertooth::cmd_get_modulation()
{
	uint8_t modulation;

	cmd_sync(CTRL_IN, Command::GET_MOD, 0, 0, &modulation, 1, 1000);
	return modulation;
}

uint16_t Ubertooth::cmd_get_channel()
{
	uint8_t result[2];

	cmd_sync(CTRL_IN, Command::GET_CHANNEL, 0, 0, result, 2, 1000);

	return result[0] | (result[1] << 8);
}


int Ubertooth::cmd_set_channel(uint16_t channel)
{
	return cmd_sync(CTRL_OUT, Command::SET_CHANNEL, channel, 0, NULL, 0, 1000);
}

uint32_t Ubertooth::cmd_get_partnum()
{
	uint8_t result[5];

	cmd_sync(CTRL_IN, Command::GET_PARTNUM, 0, 0, result, 5, 1000);

	if (result[0] != 0) {
		std::cerr <<  "result not zero: " << result[0] << std::endl;
		return 0;
	}
	return result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24);
}

void Ubertooth::cmd_get_serial(uint8_t* serial)
{
	cmd_sync(CTRL_IN, Command::GET_SERIAL, 0, 0, serial, 17, 1000);

	if (serial[0] != 0) {
		std::cerr << "result not zero: " << serial[0] << std::endl;
	}
}

int Ubertooth::cmd_set_modulation(uint16_t mod)
{
	return cmd_sync(CTRL_OUT, Command::SET_MOD, mod, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_set_isp()
{
	return cmd_sync(CTRL_OUT, Command::SET_ISP, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_reset()
{
	return cmd_sync(CTRL_OUT, Command::RESET, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_stop()
{
	return cmd_sync(CTRL_OUT, Command::STOP, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_set_paen(uint16_t state)
{
	return cmd_sync(CTRL_OUT, Command::SET_PAEN, state, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_set_hgm(uint16_t state)
{
	return cmd_sync(CTRL_OUT, Command::SET_HGM, state, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_tx_test()
{
	return cmd_sync(CTRL_OUT, Command::TX_TEST, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_flash()
{
	return cmd_sync(CTRL_OUT, Command::FLASH, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_get_palevel()
{
	uint8_t level;

	cmd_sync(CTRL_IN, Command::GET_PALEVEL, 0, 0, &level, 1, 3000);
	return level;
}

int Ubertooth::cmd_set_palevel(uint16_t level)
{
	return cmd_sync(CTRL_OUT, Command::SET_PALEVEL, level, 0, NULL, 0, 3000);
}

int Ubertooth::cmd_get_rangeresult(rangetest_result *rr)
{
	uint8_t result[5];

	int r = cmd_sync(CTRL_IN, Command::RANGE_CHECK, 0, 0, result, sizeof(result), 3000);
	if (r < LIBUSB_SUCCESS) {
		return r;
	}

	rr->valid       = result[0];
	rr->request_pa  = result[1];
	rr->request_num = result[2];
	rr->reply_pa    = result[3];
	rr->reply_num   = result[4];

	return 0;
}

int Ubertooth::cmd_range_test()
{
	return cmd_sync(CTRL_OUT, Command::RANGE_TEST, 0, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_repeater()
{
	return cmd_sync(CTRL_OUT, Command::REPEATER, 0, 0, NULL, 0, 1000);
}

void Ubertooth::cmd_get_rev_num(char* version, size_t len)
{
	uint8_t result[2 + 1 + 255];
	uint16_t result_ver;
	int r;
	r = cmd_sync(CTRL_IN, Command::GET_REV_NUM, 0, 0,
			result, sizeof(result), 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		snprintf(version, len - 1, "error: %d", r);
		version[len-1] = '\0';
		return;
	} else if (r < 0) {
		show_libusb_error(r);
		snprintf(version, len - 1, "error: %d", r);
		version[len-1] = '\0';
		return;
	}

	result_ver = result[0] | (result[1] << 8);
	if (r == 2) { // old-style SVN rev
		sprintf(version, "%u", result_ver);
	} else {
		len = MIN(r - 3, MIN(len - 1, result[2]));
		memcpy(version, &result[3], len);
		version[len] = '\0';
	}
}

void Ubertooth::cmd_get_compile_info(char* compile_info, size_t len)
{
	uint8_t result[1 + 255];
	int r;
	r = cmd_sync(CTRL_IN, Command::GET_COMPILE_INFO, 0, 0,
			result, sizeof(result), 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		snprintf(compile_info, len - 1, "error: %d", r);
		compile_info[len-1] = '\0';
		return;
	} else if (r < 0) {
		show_libusb_error(r);
		snprintf(compile_info, len - 1, "error: %d", r);
		compile_info[len-1] = '\0';
		return;
	}

	len = MIN(r - 1, MIN(len - 1, result[0]));
	memcpy(compile_info, &result[1], len);
	compile_info[len] = '\0';
}

uint8_t Ubertooth::cmd_get_board_id()
{
	uint8_t board_id;

	cmd_sync(CTRL_IN, Command::GET_BOARD_ID, 0, 0, &board_id, 1, 1000);

	return board_id;
}

int Ubertooth::cmd_set_squelch(uint16_t level)
{
	return cmd_sync(CTRL_OUT, Command::SET_SQUELCH, level, 0, NULL, 0, 3000);
}

int Ubertooth::cmd_get_squelch()
{
	uint8_t level;

	cmd_sync(CTRL_IN, Command::GET_SQUELCH, 0, 0, &level, 1, 3000);

	return level;
}

int Ubertooth::cmd_set_bdaddr(uint64_t address)
{
	uint64_t syncword;
	unsigned char data[16];

	syncword = btbb_gen_syncword(address & 0xffffff);
	//printf("syncword=%#llx\n", syncword);
	for(int r=0; r < 8; r++)
		data[r] = (address >> (8*r)) & 0xff;
	for(int r=0; r < 8; r++)
		data[r+8] = (syncword >> (8*r)) & 0xff;

	return cmd_sync(CTRL_OUT, Command::SET_BDADDR, 0, 0, data, sizeof(data), 1000);
}

int Ubertooth::cmd_start_hopping(int clkn_offset, int clk100ns_offset)
{
	uint8_t data[6];

	for(int r=0; r < 4; r++)
		data[r] = (clkn_offset >> (8*(3-r))) & 0xff;

	data[4] = (clk100ns_offset >> 8) & 0xff;
	data[5] = (clk100ns_offset >> 0) & 0xff;

	return cmd_async(CTRL_OUT, Command::START_HOPPING, 0, 0, data, 6);
}

int Ubertooth::cmd_set_clock(uint32_t clkn)
{
	uint8_t data[4];

	for(int r=0; r < 4; r++)
		data[r] = (clkn >> (8*r)) & 0xff;

	return cmd_sync(CTRL_OUT, Command::SET_CLOCK, 0, 0, data, 4, 1000);
}

uint32_t Ubertooth::cmd_get_clock()
{
	uint32_t clock = 0;
	unsigned char data[4];

	cmd_sync(CTRL_IN, Command::GET_CLOCK, 0, 0, data, 4, 3000);

	clock = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;

	return clock;
}

int Ubertooth::cmd_btle_sniffing(uint16_t num)
{
	return cmd_sync(CTRL_OUT, Command::BTLE_SNIFFING, num, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_set_afh_map(uint8_t* afh_map)
{
	return cmd_async(CTRL_OUT, Command::SET_AFHMAP, 0, 0, afh_map, 10);
}

int Ubertooth::cmd_clear_afh_map()
{
	return cmd_sync(CTRL_OUT, Command::CLEAR_AFHMAP, 0, 0, NULL, 0, 1000);
}

uint32_t Ubertooth::cmd_get_access_address()
{
	uint32_t access_address = 0;
	unsigned char data[4];

	cmd_sync(CTRL_IN, Command::GET_ACCESS_ADDRESS, 0, 0, data, 4, 3000);

	access_address = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	return access_address;
}

int Ubertooth::cmd_set_access_address(uint32_t access_address)
{
	uint8_t data[4];
	for(int i=0; i < 4; i++)
		data[i] = (access_address >> (8*i)) & 0xff;

	return cmd_sync(CTRL_OUT, Command::SET_ACCESS_ADDRESS, 0, 0, data, 4, 1000);
}

int Ubertooth::cmd_do_something(uint8_t* data, size_t len)
{
	return cmd_sync(CTRL_OUT, Command::DO_SOMETHING, 0, 0, data, len, 1000);
}

int Ubertooth::cmd_do_something_reply(uint8_t* data, size_t len)
{
	return cmd_sync(CTRL_IN, Command::DO_SOMETHING_REPLY, 0, 0, data, len, 3000);
}

uint8_t Ubertooth::cmd_get_crc_verify()
{
	uint8_t verify;

	cmd_sync(CTRL_IN, Command::GET_CRC_VERIFY, 0, 0, &verify, 1, 1000);

	return verify;
}

int Ubertooth::cmd_set_crc_verify(int verify)
{
	return cmd_sync(CTRL_OUT, Command::SET_CRC_VERIFY, verify, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_poll(usb_pkt_rx* p)
{
	return cmd_sync(CTRL_IN, Command::POLL, 0, 0, (uint8_t*)p, sizeof(usb_pkt_rx), 1000);
}

int Ubertooth::cmd_btle_promisc()
{
	return cmd_sync(CTRL_OUT, Command::BTLE_PROMISC, 0, 0, NULL, 0, 1000);
}

uint16_t Ubertooth::cmd_read_register(uint8_t reg)
{
	uint8_t data[2];

	cmd_sync(CTRL_IN, Command::READ_REGISTER, reg, 0, data, 2, 1000);

	return (data[0] << 8) | data[1];
}

int Ubertooth::cmd_btle_slave(uint8_t *mac_address)
{
	return cmd_sync(CTRL_OUT, Command::BTLE_SLAVE, 0, 0, mac_address, 6, 1000);
}

int Ubertooth::cmd_btle_set_target(uint8_t *mac_address)
{
	return cmd_sync(CTRL_OUT, Command::BTLE_SET_TARGET, 0, 0, mac_address, 6, 1000);
}

int Ubertooth::cmd_set_jam_mode(int mode)
{
	return cmd_sync(CTRL_OUT, Command::JAM_MODE, mode, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_ego(int mode)
{
	return cmd_sync(CTRL_OUT, Command::EGO, mode, 0, NULL, 0, 1000);
}

int Ubertooth::cmd_afh()
{
	return cmd_sync(CTRL_OUT, Command::AFH, 0, 0, NULL, 0, 1000);
}

void Ubertooth::cmd_hop()
{
	cmd_async(CTRL_OUT, Command::HOP, 0, 0, NULL, 0);
}

int Ubertooth::cmd_sync(uint8_t type,
             Command command,
             uint16_t wValue,
             uint16_t wIndex,
             uint8_t* data,
             uint16_t size,
             unsigned int timeout)
{
	int r = libusb_control_transfer(devh, type, (uint8_t)command, wValue, wIndex, data, size, timeout);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	else if (r < size) {
		std::cerr << "Only " << r << " of " << size << " bytes transferred" << std::endl;
		return r;
	}

	return 0;
}

int Ubertooth::cmd_async(uint8_t type,
                         Command command,
                         uint16_t wValue,
                         uint16_t wIndex,
                         uint8_t* data,
                         uint16_t size)
{
	uint8_t buffer[LIBUSB_CONTROL_SETUP_SIZE + size];
	struct libusb_transfer* xfer = libusb_alloc_transfer(0);

	libusb_fill_control_setup(buffer, type, (uint8_t)command, wValue, wIndex, size);
	if(size > 0)
		memcpy ( &buffer[LIBUSB_CONTROL_SETUP_SIZE], data, size );
	libusb_fill_control_transfer(xfer, devh, buffer, cmd_callback, NULL, 1000);
	xfer->status = (libusb_transfer_status)(LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER);

	int r = libusb_submit_transfer(xfer);
	if (r < 0)
		show_libusb_error(r);

	return r;
}
