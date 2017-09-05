/*
 * Copyright 2010, 2011 Michael Ossmann
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

#include <iostream>
#include <iomanip>

#include "ubertooth.h"

#include "packetsource_ubertooth.h"

void Ubertooth::start()
{
	stop_ubertooth = false;
	source->start();
}

void Ubertooth::stop()
{
	stop_ubertooth = true;
	source->stop();
	cmd_stop();
}

usb_pkt_rx Ubertooth::receive()
{
	return source->receive();
}

Ubertooth::Ubertooth(int ubertooth_device)
{
	connect(ubertooth_device);
	source = new PacketsourceUbertooth(devh);
}

Ubertooth::~Ubertooth()
{
	stop();

	delete source;

	if (h_pcap_bredr != NULL)
		btbb_pcap_close(h_pcap_bredr);
	if (h_pcap_le != NULL)
		lell_pcap_close(h_pcap_le);
	if (h_pcapng_bredr != NULL)
		btbb_pcapng_close(h_pcapng_bredr);
	if (h_pcapng_le != NULL)
		lell_pcapng_close(h_pcapng_le);
}
