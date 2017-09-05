/*
 * Copyright 2010 - 2013 Michael Ossmann, Dominic Spill, Will Code, Mike Ryan
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

#pragma once

#include "basic_ubertooth.h"
#include "ubertooth_interface.h"
#include "packetsource.h"
#include <btbb.h>

class Ubertooth : public BasicUbertooth
{
protected:
	Packetsource* source = NULL;

	uint64_t abs_start_ns   = 0;
	uint32_t start_clk100ns = 0;
	uint64_t last_clk100ns  = 0;
	uint64_t clk100ns_upper = 0;

	btbb_pcap_handle* h_pcap_bredr = NULL;
	lell_pcap_handle* h_pcap_le = NULL;
	btbb_pcapng_handle* h_pcapng_bredr = NULL;
	lell_pcapng_handle* h_pcapng_le = NULL;

public:
	Ubertooth();
	Ubertooth(int ubertooth_device);

	~Ubertooth();

	void start();
	void stop();
	usb_pkt_rx receive();

public:
	void cmd_trim_clock(uint16_t offset);
	void cmd_fix_clock_drift(int16_t ppm);
	int cmd_ping();
	int cmd_rx_syms();
	int cmd_tx_syms();
	int cmd_specan(uint16_t low_freq, uint16_t high_freq);
	int cmd_led_specan(uint16_t rssi_threshold);
	int cmd_set_usrled(uint16_t state);
	uint8_t cmd_get_usrled();
	int cmd_set_rxled(uint16_t state);
	uint8_t cmd_get_rxled();
	int cmd_set_txled(uint16_t state);
	uint8_t cmd_get_txled();
	uint32_t cmd_get_partnum();
	void cmd_get_serial(uint8_t* serial);
	int cmd_set_modulation(Modulation mod);
	uint8_t cmd_get_modulation();
	int cmd_set_isp();
	int cmd_reset();
	int cmd_stop();
	int cmd_set_paen(uint16_t state);
	int cmd_set_hgm(uint16_t state);
	int cmd_tx_test();
	int cmd_get_palevel();
	int cmd_set_palevel(uint16_t level);
	uint16_t cmd_get_channel();
	int cmd_set_channel(uint16_t channel);
	int cmd_get_rangeresult(rangetest_result* rr);
	int cmd_range_test();
	int cmd_repeater();
	void cmd_get_rev_num(char* version, size_t len);
	void cmd_get_compile_info(char* compile_info, size_t len);
	uint8_t cmd_get_board_id();
	int cmd_set_squelch(uint16_t level);
	int cmd_get_squelch();
	int cmd_set_bdaddr(uint64_t bdaddr);
	int cmd_set_syncword(uint64_t syncword);
	int cmd_next_hop(uint16_t clk);
	int cmd_start_hopping(int clkn_offset, int clk100ns_offset);
	int cmd_set_clock(uint32_t clkn);
	uint32_t cmd_get_clock();
	int cmd_set_afh_map(uint8_t* afh_map);
	int cmd_clear_afh_map();
	int cmd_btle_sniffing(uint16_t num);
	uint32_t cmd_get_access_address();
	int cmd_set_access_address(uint32_t access_address);
	int cmd_do_something(uint8_t* data, size_t len);
	int cmd_do_something_reply(uint8_t* data, size_t len);
	uint8_t cmd_get_crc_verify();
	int cmd_set_crc_verify(int verify);
	int cmd_poll(usb_pkt_rx* p);
	int cmd_btle_promisc();
	uint16_t cmd_read_register(uint8_t reg);
	int cmd_btle_slave(uint8_t* mac_address);
	int cmd_btle_set_target(uint8_t* mac_address);
	int cmd_set_jam_mode(int mode);
	int cmd_ego(int mode);
	int cmd_afh();
	void cmd_hop();
};
