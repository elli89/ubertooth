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

#include <signal.h>

#include <iostream>

#include "ubertooth.h"
#include "ubertooth_interface.h"

#include "packetsource_ubertooth.h"

static struct libusb_device_handle* find_ubertooth_device(int ubertooth_device)
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **usb_list = NULL;
	struct libusb_device_handle *devh = NULL;
	struct libusb_device_descriptor desc;
	int usb_devs, i, r, ret, ubertooths = 0;
	int ubertooth_devs[] = {0,0,0,0,0,0,0,0};

	usb_devs = libusb_get_device_list(ctx, &usb_list);
	for(i = 0 ; i < usb_devs ; ++i) {
		r = libusb_get_device_descriptor(usb_list[i], &desc);
		if(r < 0)
			std::cerr << "couldn't get usb descriptor for dev " << i << "!" << std::endl;
		if ((desc.idVendor == TC13_VENDORID && desc.idProduct == TC13_PRODUCTID)
		    || (desc.idVendor == U0_VENDORID && desc.idProduct == U0_PRODUCTID)
		    || (desc.idVendor == U1_VENDORID && desc.idProduct == U1_PRODUCTID))
		{
			ubertooth_devs[ubertooths] = i;
			ubertooths++;
		}
	}
	if(ubertooths == 1) {
		ret = libusb_open(usb_list[ubertooth_devs[0]], &devh);
		if (ret)
			show_libusb_error(ret);
	}
	else if (ubertooths == 0)
		return NULL;
	else {
		if (ubertooth_device < 0) {
			std::cerr << "multiple Ubertooth devices found! Use '-U' to specify device number" << std::endl;
			uint8_t serial[17], r;
			for(i = 0 ; i < ubertooths ; ++i) {
				libusb_get_device_descriptor(usb_list[ubertooth_devs[i]], &desc);
				ret = libusb_open(usb_list[ubertooth_devs[i]], &devh);
				if (ret) {
					std::cerr << "  Device " << i << ": ";
					show_libusb_error(ret);
				}
				else {
					r = cmd_get_serial(devh, serial);
					if(r==0) {
						std::cerr << "  Device " << i << ": ";
						print_serial(serial, stderr);
					}
					libusb_close(devh);
				}
			}
			devh = NULL;
		} else {
			ret = libusb_open(usb_list[ubertooth_devs[ubertooth_device]], &devh);
			if (ret) {
					show_libusb_error(ret);
					devh = NULL;
				}
		}
	}
	return devh;
}

void Ubertooth::start()
{
	source->start();
}

void Ubertooth::stop()
{
	source->stop();
}

usb_pkt_rx Ubertooth::receive()
{
	return source->receive();
}

Ubertooth::Ubertooth()
{
	devh = NULL;
	source = NULL;
	abs_start_ns = 0;
	start_clk100ns = 0;
	last_clk100ns = 0;
	clk100ns_upper = 0;

	h_pcap_bredr = NULL;
	h_pcap_le = NULL;
	h_pcapng_bredr = NULL;
	h_pcapng_le = NULL;
}

Ubertooth::Ubertooth(int ubertooth_device)
{
	Ubertooth();

	connect(ubertooth_device);
}

Ubertooth::~Ubertooth()
{
	stop();

	if (devh != NULL) {
		cmd_stop(devh);
		libusb_release_interface(devh, 0);
	}
	libusb_close(devh);
	libusb_exit(NULL);

	if (h_pcap_bredr)
		btbb_pcap_close(h_pcap_bredr);
	if (h_pcap_le)
		lell_pcap_close(h_pcap_le);
	if (h_pcapng_bredr)
		btbb_pcapng_close(h_pcapng_bredr);
	if (h_pcapng_le)
		lell_pcapng_close(h_pcapng_le);
}

int Ubertooth::connect(int ubertooth_device)
{
	int r = libusb_init(NULL);
	if (r < 0) {
		std::cerr << "libusb_init failed (got 1.0?)" << std::endl;
		return -1;
	}

	devh = find_ubertooth_device(ubertooth_device);
	if (devh == NULL) {
		std::cerr << "could not open Ubertooth device" << std::endl;
		stop();
		return -1;
	}

	r = libusb_claim_interface(devh, 0);
	if (r < 0) {
		std::cerr << "usb_claim_interface error " << r << std::endl;
		stop();
		return -1;
	}

	source = new PacketsourceUbertooth(devh);

	return 1;
}
