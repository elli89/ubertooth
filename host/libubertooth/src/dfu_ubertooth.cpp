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


#include <unistd.h>  // sleep


#include "dfu_ubertooth.h"

int DfuUbertooth::connect_to_dfu_device(int ubertooth_device)
{
	struct libusb_device** usb_devices;
	struct libusb_device_descriptor desc;
	size_t usb_devs = libusb_get_device_list(NULL, &usb_devices);

	std::vector<libusb_device*> ubertooths;

	for(size_t i = 0 ; i < usb_devs ; ++i) {
		libusb_get_device_descriptor(usb_devices[i], &desc);
		if ((desc.idVendor == TC13_VENDORID && desc.idProduct == TC13_PRODUCTID) ||
		    (desc.idVendor == U1_DFU_VENDORID && desc.idProduct == U1_DFU_PRODUCTID))
		{
			ubertooths.push_back(usb_devices[i]);
		}
	}

	if(ubertooths.size() == 0) {
		std::cerr << "Could not find any DFU devices" << std::endl;
		return -1;
	}

	int r = libusb_open(ubertooths[ubertooth_device], &devh);
	if (r) {
		std::cerr << "could not open DFU device" << std::endl;
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

int DfuUbertooth::connect(int ubertooth_device)
{
	if (ubertooth_device < 0) {
		// list_ubertooth_devices();
		ubertooth_device = 0;
	}

	int r = connect_to_dfu_device(ubertooth_device);

	if (r < 1) {
		r = connect_to_ubertooth_device(ubertooth_device);
		if (r < 1)
			return -1;

		this->cmd_flash();

		libusb_release_interface(devh, 0);
		libusb_close(devh);

		std::cout << "Switching to DFU mode..." << std::endl;

		int count = 0;
		while (connect_to_dfu_device(ubertooth_device) < 1 && (count++) < 5) {
			sleep(1);
		}
	}

	std::cout << "Connected to Ubertooth in DFU mode" << std::endl;

	return 1;
}


int DfuUbertooth::enter_dfu_mode() {
	while(1) {
		DfuState state = (DfuState)cmd_get_state();
		if(state == DfuState::DFU_IDLE)
			break;
		switch(state) {
			case DfuState::DFU_DOWNLOAD_SYNC:
			case DfuState::DFU_DOWNLOAD_IDLE:
			case DfuState::DFU_MANIFEST_SYNC:
			case DfuState::DFU_UPLOAD_IDLE:
			case DfuState::DFU_MANIFEST:
				if(cmd_abort() < 0) {
					std::cerr << "Unable to abort transaction from state: " << (int)state << std::endl;
					return -1;
				}
				break;
			case DfuState::APP_DETACH:
			case DfuState::DFU_DOWNBUSY:
			case DfuState::DFU_MANIFEST_WAIT_RESET:
				sleep(1);
				break;
			case DfuState::APP_IDLE:
				// We don't support this state in the application
				//detach();
				break;
			case DfuState::DFU_ERROR:
				cmd_clear_status();
				break;
			case DfuState::DFU_IDLE:
				break;
		}
	}
	return 0;
}

int DfuUbertooth::detach() {
	int rv = enter_dfu_mode();
	if(rv < 0) {
		std::cerr << "Detach failed: could not enter DFU mode\n";
		return rv;
	}
	DfuState state = (DfuState)cmd_get_state();
	if(state == DfuState::DFU_IDLE) {
		cmd_detach();
		std::cout << "Detached\n";
	} else {
		std::cerr << "In unexpected state: " << (int)state << std::endl;
		return 1;
	}
	return 0;
}

int DfuUbertooth::upload(const std::string& filename)
{
	int rv;
	size_t block = 0;

	rv = enter_dfu_mode();
	if(rv < 0) {
		std::cerr << "Upload failed: could not enter DFU mode\n";
		return rv;
	}

	std::ofstream upfile(filename, std::ios::binary);
	while(true) {
		uint8_t buffer[BLOCK_SIZE];

		rv = cmd_upload(block, buffer);

		std::cout << ".";
		if(rv == BLOCK_SIZE) {
			upfile.write((char*)buffer, BLOCK_SIZE);
		} else {
			upfile.write((char*)buffer, rv);
			break;
		}
		block++;
	}
	upfile.close();
 	std::cout << "\n";
	return 0;
}

int DfuUbertooth::download(const std::string& filename) {
	uint8_t buffer[BLOCK_SIZE];
	size_t block = 0;

	enter_dfu_mode();

	std::ifstream downfile(filename, std::ios::binary | std::ios::ate);
	size_t filelength = downfile.tellg();
	downfile.seekg(0, std::ios::beg);
	for(size_t i=0; i<(filelength/BLOCK_SIZE); i++) {
		downfile.read((char*)buffer, BLOCK_SIZE);
		cmd_download(block, buffer);

		cmd_get_status();
		std::cout << ".";
		block++;
	}

	downfile.read((char*)buffer, filelength%BLOCK_SIZE);

	for(size_t i=filelength%BLOCK_SIZE; i<BLOCK_SIZE; i++)
		buffer[i] = 0xFF;

	cmd_download(block, buffer);

	cmd_get_status();
	std::cout << "." << std::endl;

	downfile.close();
	return 0;
}


DfuUbertooth::DfuUbertooth(int ubertooth_device)
{
	int r = libusb_init(NULL);
	if (r < 0) {
		std::cerr << "libusb_init failed (got 1.0?)" << std::endl;
	}
	connect(ubertooth_device);
}

DfuUbertooth::DfuUbertooth()
{
	int r = libusb_init(NULL);
	if (r < 0) {
		std::cerr << "libusb_init failed (got 1.0?)" << std::endl;
	}
}

DfuUbertooth::~DfuUbertooth()
{
	// if (devh != NULL) {
	// 	// cmd_stop(); FIXME
	// 	libusb_release_interface(devh, 0);
	// 	libusb_close(devh);
	// }
	// libusb_exit(NULL);
}
