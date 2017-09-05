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

#include <vector>
#include <libusb-1.0/libusb.h>


constexpr const uint16_t U0_VENDORID    = 0x1d50;
constexpr const uint16_t U0_PRODUCTID   = 0x6000;
constexpr const uint16_t U1_VENDORID    = 0x1d50;
constexpr const uint16_t U1_PRODUCTID   = 0x6002;
constexpr const uint16_t TC13_VENDORID  = 0xffff;
constexpr const uint16_t TC13_PRODUCTID = 0x0004;


class BasicUbertooth
{
protected:
	struct libusb_device_handle* devh = NULL;

	int connect_to_ubertooth_device(int ubertooth_device);

public:
	BasicUbertooth();
	BasicUbertooth(int ubertooth_device);

	~BasicUbertooth();

	int connect(int ubertooth_device);

	int cmd_flash();

protected:

	static void show_libusb_error(int error_code);

	static void cmd_callback(struct libusb_transfer* transfer);
	int cmd_sync(uint8_t type,
	             uint8_t command,
	             uint16_t wValue,
	             uint16_t wIndex,
	             uint8_t* data,
	             uint16_t size,
	             unsigned int timeout);
	int cmd_async(uint8_t type,
	              uint8_t command,
	              uint16_t wValue,
	              uint16_t wIndex,
	              uint8_t* data,
	              uint16_t size);
};
