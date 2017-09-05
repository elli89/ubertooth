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
#include "packetsource.h"
#include <btbb.h>

class Ubertooth
{
private:
	Packetsource* source = NULL;

	struct libusb_device_handle* devh = NULL;

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

	int connect(int ubertooth_device);
	void start();
	void stop();
	usb_pkt_rx receive();
};

#endif /* __UBERTOOTH_H__ */
