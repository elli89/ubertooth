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

#include "basic_ubertooth.h"

#include <vector>

int BasicUbertooth::connect_to_ubertooth_device(int ubertooth_device)
{
	struct libusb_device** usb_devices;
	struct libusb_device_descriptor desc;
	size_t usb_devs = libusb_get_device_list(NULL, &usb_devices);

	std::vector<libusb_device*> ubertooths;

	for(size_t i = 0 ; i < usb_devs ; ++i) {
		libusb_get_device_descriptor(usb_devices[i], &desc);
		if ((desc.idVendor == TC13_VENDORID && desc.idProduct == TC13_PRODUCTID)
		    || (desc.idVendor == U0_VENDORID && desc.idProduct == U0_PRODUCTID)
		    || (desc.idVendor == U1_VENDORID && desc.idProduct == U1_PRODUCTID))
		{
			ubertooths.push_back(usb_devices[i]);
		}
	}

	if(ubertooths.size() == 0) {
		std::cerr << "Could not find any Ubertooth devices" << std::endl;
		return -1;
	}

	int r = libusb_open(ubertooths[ubertooth_device], &devh);
	if (r) {
		std::cerr << "could not open Ubertooth device" << std::endl;
		show_libusb_error(r);
		return -1;
	}

	r = libusb_claim_interface(devh, 0);
	if (r < 0) {
		std::cerr << "usb_claim_interface error " << r << std::endl;
		return -1;
	}
	libusb_free_device_list(usb_devices, 1);

	return 1;
}

BasicUbertooth::BasicUbertooth(int ubertooth_device)
{
	int r = libusb_init(NULL);
	if (r < 0) {
		std::cerr << "libusb_init failed (got 1.0?)" << std::endl;
	}
	connect(ubertooth_device);
}

BasicUbertooth::BasicUbertooth()
{
	int r = libusb_init(NULL);
	if (r < 0) {
		std::cerr << "libusb_init failed (got 1.0?)" << std::endl;
	}
}

BasicUbertooth::~BasicUbertooth()
{
	if (devh != NULL) {
		// cmd_stop(); FIXME
		libusb_release_interface(devh, 0);
		libusb_close(devh);
	}
	libusb_exit(NULL);
}

int BasicUbertooth::connect(int ubertooth_device)
{
	if (ubertooth_device < 0) {
		// list_ubertooth_devices();
		ubertooth_device = 0;
	}

	connect_to_ubertooth_device(ubertooth_device);

	std::cout << "connected" << std::endl;

	return 1;
}
