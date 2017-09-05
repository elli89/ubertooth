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
#include "ubertooth_control.h"

#define CTRL_IN     (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT    (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)

void show_libusb_error(int error_code)
{
	std::cerr << "libUSB Error: " << libusb_error_name(error_code) << ": " << libusb_strerror((libusb_error)error_code) << " (" << error_code << ")" << std::endl;
}

static void callback(struct libusb_transfer* transfer)
{
	if(transfer->status != 0) {
		show_libusb_error(transfer->status);
	}
	libusb_free_transfer(transfer);
}

void cmd_trim_clock(struct libusb_device_handle* devh, uint16_t offset)
{
	uint8_t data[2] = {
		(uint8_t)((offset >> 8) & 0xff),
		(uint8_t)((offset >> 0) & 0xff)
	};

	ubertooth_cmd_async(devh, CTRL_OUT, Command::UBERTOOTH_TRIM_CLOCK, data, 2);
}

void cmd_fix_clock_drift(struct libusb_device_handle* devh, int16_t ppm)
{
	uint8_t data[2] = {
		(uint8_t)((ppm >> 8) & 0xff),
		(uint8_t)((ppm >> 0) & 0xff)
	};

	ubertooth_cmd_async(devh, CTRL_OUT, Command::UBERTOOTH_FIX_CLOCK_DRIFT, data, 2);
}

int cmd_ping(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_PING, 0, 0, NULL, 0, 1000);
}

int cmd_rx_syms(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_RX_SYMBOLS, 0, 0, NULL, 0, 1000);
}

int cmd_tx_syms(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_async(devh, CTRL_OUT, Command::UBERTOOTH_TX_SYMBOLS, 0, 0);
}

int cmd_specan(struct libusb_device_handle* devh, uint16_t low_freq, uint16_t high_freq)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SPECAN, low_freq, high_freq, NULL, 0, 1000);
}

int cmd_led_specan(struct libusb_device_handle* devh, uint16_t rssi_threshold)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_LED_SPECAN, rssi_threshold, 0, NULL, 0, 1000);
}

int cmd_set_usrled(struct libusb_device_handle* devh, uint16_t state)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_USRLED, state, 0, NULL, 0, 1000);
}

int cmd_get_usrled(struct libusb_device_handle* devh)
{
	uint8_t state;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_USRLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_set_rxled(struct libusb_device_handle* devh, uint16_t state)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_RXLED, state, 0, NULL, 0, 1000);
}

int cmd_get_rxled(struct libusb_device_handle* devh)
{
	uint8_t state;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_RXLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_set_txled(struct libusb_device_handle* devh, uint16_t state)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_TXLED, state, 0, NULL, 0, 1000);
}

int cmd_get_txled(struct libusb_device_handle* devh)
{
	uint8_t state;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_TXLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_get_modulation(struct libusb_device_handle* devh)
{
	uint8_t modulation;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_MOD, 0, 0,
			&modulation, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return modulation;
}

int cmd_get_channel(struct libusb_device_handle* devh)
{
	uint8_t result[2];
	int r;
	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_CHANNEL, 0, 0,
			result, 2, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return result[0] | (result[1] << 8);
}


int cmd_set_channel(struct libusb_device_handle* devh, uint16_t channel)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_CHANNEL, channel, 0, NULL, 0, 1000);
}

int cmd_get_partnum(struct libusb_device_handle* devh)
{
	uint8_t result[5];
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_PARTNUM, 0, 0,
			result, 5, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	if (result[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", result[0]);
		return 0;
	}
	return result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24);
}

void print_serial(uint8_t *serial, FILE *fileptr)
{
	int i;
	if(fileptr == NULL)
		fileptr = stdout;

	fprintf(fileptr, "Serial No: ");
	for(i=1; i<17; i++)
		fprintf(fileptr, "%02x", serial[i]);
	fprintf(fileptr, "\n");
}

int cmd_get_serial(struct libusb_device_handle* devh, uint8_t *serial)
{
	int r;
	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_SERIAL, 0, 0,
			serial, 17, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	if (serial[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", serial[0]);
		return 1;
	}
	return 0;
}

int cmd_set_modulation(struct libusb_device_handle* devh, uint16_t mod)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_MOD, mod, 0, NULL, 0, 1000);
}

int cmd_set_isp(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_ISP, 0, 0, NULL, 0, 1000);
}

int cmd_reset(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_RESET, 0, 0, NULL, 0, 1000);
}

int cmd_stop(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_STOP, 0, 0, NULL, 0, 1000);
}

int cmd_set_paen(struct libusb_device_handle* devh, uint16_t state)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_PAEN, state, 0, NULL, 0, 1000);
}

int cmd_set_hgm(struct libusb_device_handle* devh, uint16_t state)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_HGM, state, 0, NULL, 0, 1000);
}

int cmd_tx_test(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_TX_TEST, 0, 0, NULL, 0, 1000);
}

int cmd_flash(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_FLASH, 0, 0, NULL, 0, 1000);
}

int cmd_get_palevel(struct libusb_device_handle* devh)
{
	uint8_t level;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_PALEVEL, 0, 0,
			&level, 1, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return level;
}

int cmd_set_palevel(struct libusb_device_handle* devh, uint16_t level)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_PALEVEL, level, 0, NULL, 0, 3000);
}

int cmd_get_rangeresult(struct libusb_device_handle* devh,
		rangetest_result *rr)
{
	uint8_t result[5];
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_RANGE_CHECK, 0, 0,
			result, sizeof(result), 3000);
	if (r < LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}

	rr->valid       = result[0];
	rr->request_pa  = result[1];
	rr->request_num = result[2];
	rr->reply_pa    = result[3];
	rr->reply_num   = result[4];

	return 0;
}

int cmd_range_test(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_RANGE_TEST, 0, 0, NULL, 0, 1000);
}

int cmd_repeater(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_REPEATER, 0, 0, NULL, 0, 1000);
}

void cmd_get_rev_num(struct libusb_device_handle* devh, char *version, uint8_t len)
{
	uint8_t result[2 + 1 + 255];
	uint16_t result_ver;
	int r;
	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_REV_NUM, 0, 0,
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

void cmd_get_compile_info(struct libusb_device_handle* devh, char *compile_info, uint8_t len)
{
	uint8_t result[1 + 255];
	int r;
	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_COMPILE_INFO, 0, 0,
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

int cmd_get_board_id(struct libusb_device_handle* devh)
{
	uint8_t board_id;
	int r;
	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_BOARD_ID, 0, 0,
			&board_id, 1, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return board_id;
}

int cmd_set_squelch(struct libusb_device_handle* devh, uint16_t level)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_SQUELCH, level, 0, NULL, 0, 3000);
}

int cmd_get_squelch(struct libusb_device_handle* devh)
{
	uint8_t level;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_SQUELCH, 0, 0,
			&level, 1, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return level;
}

int cmd_set_bdaddr(struct libusb_device_handle* devh, uint64_t address)
{
	int r, data_len;
	uint64_t syncword;
	data_len = 16;
	unsigned char data[data_len];

	syncword = btbb_gen_syncword(address & 0xffffff);
	//printf("syncword=%#llx\n", syncword);
	for(r=0; r < 8; r++)
		data[r] = (address >> (8*r)) & 0xff;
	for(r=0; r < 8; r++)
		data[r+8] = (syncword >> (8*r)) & 0xff;

	r = ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_BDADDR, 0, 0,
		data, data_len, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	} else if (r < data_len) {
		fprintf(stderr, "Only %d of %d bytes transferred\n", r, data_len);
		return 1;
	}
	return 0;
}

int cmd_start_hopping(struct libusb_device_handle* devh, int clkn_offset, int clk100ns_offset)
{
	int r;
	uint8_t data[6];
	for(r=0; r < 4; r++)
		data[r] = (clkn_offset >> (8*(3-r))) & 0xff;

	data[4] = (clk100ns_offset >> 8) & 0xff;
	data[5] = (clk100ns_offset >> 0) & 0xff;

	return ubertooth_cmd_async(devh, CTRL_OUT, Command::UBERTOOTH_START_HOPPING, data, 6);
}

int cmd_set_clock(struct libusb_device_handle* devh, uint32_t clkn)
{
	int r;
	uint8_t data[4];
	for(r=0; r < 4; r++)
		data[r] = (clkn >> (8*r)) & 0xff;

	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_CLOCK, 0, 0, data, 4, 1000);
}

uint32_t cmd_get_clock(struct libusb_device_handle* devh)
{
	uint32_t clock = 0;
	unsigned char data[4];
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_CLOCK, 0, 0,
			data, 4, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	clock = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	printf("Read clock = 0x%x\n", clock);
	return clock;
}

int cmd_btle_sniffing(struct libusb_device_handle* devh, uint16_t num)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_BTLE_SNIFFING, num, 0, NULL, 0, 1000);
}

int cmd_set_afh_map(struct libusb_device_handle* devh, uint8_t* afh_map)
{
	return ubertooth_cmd_async(devh, CTRL_OUT, Command::UBERTOOTH_SET_AFHMAP, afh_map, 10);
}

int cmd_clear_afh_map(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_CLEAR_AFHMAP, 0, 0, NULL, 0, 1000);
}

uint32_t cmd_get_access_address(struct libusb_device_handle* devh)
{
	uint32_t access_address = 0;
	unsigned char data[4];
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_ACCESS_ADDRESS, 0, 0,
			data, 4, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	access_address = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	return access_address;
}

int cmd_set_access_address(struct libusb_device_handle* devh, uint32_t access_address)
{
	uint8_t data[4];
	for(int i=0; i < 4; i++)
		data[i] = (access_address >> (8*i)) & 0xff;

	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_ACCESS_ADDRESS, 0, 0, data, 4, 1000);
}

int cmd_do_something(struct libusb_device_handle *devh, unsigned char *data, int len)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_DO_SOMETHING, 0, 0, data, len, 1000);
}

int cmd_do_something_reply(struct libusb_device_handle* devh, unsigned char *data, int len)
{
	int r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_DO_SOMETHING_REPLY, 0, 0,
				data, len, 3000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return r;
}

int cmd_get_crc_verify(struct libusb_device_handle* devh)
{
	uint8_t verify;
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_GET_CRC_VERIFY, 0, 0,
			&verify, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return verify;
}

int cmd_set_crc_verify(struct libusb_device_handle* devh, int verify)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_SET_CRC_VERIFY, verify, 0, NULL, 0, 1000);
}

int cmd_poll(struct libusb_device_handle* devh, usb_pkt_rx *p)
{
	int r;

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_POLL, 0, 0,
			(uint8_t *)p, sizeof(usb_pkt_rx), 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return r;
}

int cmd_btle_promisc(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_BTLE_PROMISC, 0, 0, NULL, 0, 1000);
}

int cmd_read_register(struct libusb_device_handle* devh, uint8_t reg)
{
	int r;
	uint8_t data[2];

	r = ubertooth_cmd_sync(devh, CTRL_IN, Command::UBERTOOTH_READ_REGISTER, reg, 0,
			data, 2, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}

	return (data[0] << 8) | data[1];
}

int cmd_btle_slave(struct libusb_device_handle* devh, uint8_t *mac_address)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_BTLE_SLAVE, 0, 0, mac_address, 6, 1000);
}

int cmd_btle_set_target(struct libusb_device_handle* devh, uint8_t *mac_address)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_BTLE_SET_TARGET, 0, 0, mac_address, 6, 1000);
}

int cmd_set_jam_mode(struct libusb_device_handle* devh, int mode)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_JAM_MODE, mode, 0, NULL, 0, 1000);
}

int cmd_ego(struct libusb_device_handle* devh, int mode)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_EGO, mode, 0, NULL, 0, 1000);
}

int cmd_afh(struct libusb_device_handle* devh)
{
	return ubertooth_cmd_sync(devh, CTRL_OUT, Command::UBERTOOTH_AFH, 0, 0, NULL, 0, 1000);
}

int cmd_hop(struct libusb_device_handle* devh)
{
	uint8_t buffer[LIBUSB_CONTROL_SETUP_SIZE];
	struct libusb_transfer *xfer = libusb_alloc_transfer(0);

	libusb_fill_control_setup(buffer, CTRL_OUT, (uint8_t)Command::UBERTOOTH_HOP, 0, 0, 0);
	libusb_fill_control_transfer(xfer, devh, buffer, callback, NULL, 1000);
	libusb_submit_transfer(xfer);

	return 0;
}

int ubertooth_cmd_sync(struct libusb_device_handle* devh,
                       uint8_t type,
                       Command command,
                       uint16_t wValue,
                       uint16_t wIndex,
                       uint8_t* data,
                       uint16_t size,
                       unsigned int timeout)
{
	int r;

	r = libusb_control_transfer(devh, type, (uint8_t)command, wValue, wIndex, data, size, timeout);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}

	return 0;
}

int ubertooth_cmd_async(struct libusb_device_handle* devh,
                        uint8_t type,
                        Command command,
                        uint8_t* data,
                        uint16_t size)
{
	int r = 0;

	uint8_t buffer[LIBUSB_CONTROL_SETUP_SIZE + size];
	struct libusb_transfer* xfer = libusb_alloc_transfer(0);

	libusb_fill_control_setup(buffer, type, (uint8_t)command, 0, 0, size);
	if(size > 0)
		memcpy ( &buffer[LIBUSB_CONTROL_SETUP_SIZE], data, size );
	libusb_fill_control_transfer(xfer, devh, buffer, callback, NULL, 1000);
	xfer->status = (libusb_transfer_status)(LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER);

	r = libusb_submit_transfer(xfer);
	if (r < 0)
		show_libusb_error(r);

	return r;
}
