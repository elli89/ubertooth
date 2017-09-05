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

#ifndef __UBERTOOTH_H__
#define __UBERTOOTH_H__

#include "ubertooth_control.h"
#include "ubertooth_bulk.h"
#include "packetsource.h"
#include <btbb.h>

/* specan output types
 * see https://github.com/dkogan/feedgnuplot for plotter */
enum class specan_modes {
	SPECAN_STDOUT         = 0,
	SPECAN_GNUPLOT_NORMAL = 1,
	SPECAN_GNUPLOT_3D     = 2,
	SPECAN_FILE           = 3
};

enum class board_ids {
	BOARD_ID_UBERTOOTH_ZERO = 0,
	BOARD_ID_UBERTOOTH_ONE  = 1,
	BOARD_ID_TC13BADGE      = 2
};

class Ubertooth
{
private:
	Packetsource* source;

	Bulk* bulk;

	struct libusb_device_handle* devh;

	uint8_t stop_ubertooth;
	uint64_t abs_start_ns;
	uint32_t start_clk100ns;
	uint64_t last_clk100ns;
	uint64_t clk100ns_upper;

	btbb_pcap_handle* h_pcap_bredr;
	lell_pcap_handle* h_pcap_le;
	btbb_pcapng_handle* h_pcapng_bredr;
	lell_pcapng_handle* h_pcapng_le;

public:
	Ubertooth();
	Ubertooth(int ubertooth_device);

	~Ubertooth();

	void register_cleanup_handler(int do_exit);

	int connect(int ubertooth_device);
	void stop();
	int get_api(uint16_t* version);
	int check_api();
	void set_timeout(int seconds);

	int stream_rx_file(FILE* fp, rx_callback cb, void* cb_args);

	void rx_dump(int full);
	void rx_btle();
	void rx_btle_file(FILE* fp);
	void rx_afh(btbb_piconet* pn, int timeout);
	void rx_afh_r(btbb_piconet* pn, int timeout);

	void print_version();
	void ubertooth_unpack_symbols(const uint8_t* buf, char* unpacked);
};

typedef struct {
	unsigned allowed_access_address_errors;
} btle_options;

extern uint32_t systime;
extern FILE* infile;
extern FILE* dumpfile;
extern int max_ac_errors;

#endif /* __UBERTOOTH_H__ */
