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
#include "dfu_interface.h"

#include <fstream>

class DfuUbertooth : public BasicUbertooth
{

public:
	DfuUbertooth();
	DfuUbertooth(int ubertooth_device);

	~DfuUbertooth();

	int connect_to_dfu_device(int ubertooth_device);

	int enter_dfu_mode();

	int connect(int ubertooth_device);
	int detach();
	int upload(const std::string& filename);
	int download(const std::string& filename);

public:

	int cmd_detach();
	size_t cmd_download(size_t block, uint8_t* buffer);
	size_t cmd_upload(size_t block, uint8_t* buffer);
	int cmd_get_status();
	int cmd_clear_status();
	uint8_t cmd_get_state();
	int cmd_abort();
};
