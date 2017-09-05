/* -*- c -*- */
/*
 * Copyright 2015 Dominic Spill
 *
 * This file is part of Project Ubertooth
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
 * along with libbtbb; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <getopt.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include "dfu_ubertooth.h"
#include "dfu_interface.h"


/*
 * DFU suffix / signature / CRC
 */

/* CRC32 implementation for DFU suffix */
static uint32_t crc32(uint8_t* data, size_t data_len) {
	uint32_t crc = 0xffffffff;

	for(size_t i=0; i<data_len; i++)
		crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xff];
	return crc;
}

static int check_suffix(const std::string& filename)
{
	std::cout << "Checking firmware signature\n";

	std::ifstream signedfile(filename, std::ios::binary | std::ios::ate);

	size_t data_length = signedfile.tellg();
	if (data_length > 4)
		data_length -= 4; // Ignore 4 byte CRC
	else {
		std::cerr << "file too small" << std::endl;
		signedfile.close();
		return 1;
	}

	DFU_suffix suffix;
	signedfile.seekg(-16, signedfile.end); // Start of SFU suffix
	signedfile.read((char*)(&suffix), 16);

	if(suffix.bLength != 16) {
		std::cerr << "Unknown DFU suffix length: " << suffix.bLength << std::endl;
		signedfile.close();
		return 1;
	}

	// We only know about dfu version 1.0/1.1
	// This needs to be smarter to support other versions if/when they exist
	if((suffix.bcdDFU != 0x0100) && (suffix.bcdDFU != 0x0101)) {
		std::cerr << "Unknown DFU version: %04x\n", suffix.bcdDFU;
		signedfile.close();
		return 1;
	}

	// Suffix bytes are reversed
	if(!((suffix.ucDfuSig[0]==0x55) &&
	     (suffix.ucDfuSig[1]==0x46) &&
	     (suffix.ucDfuSig[2]==0x44))) {
		std::cerr << "DFU Signature mismatch: not a DFU file\n";
		signedfile.close();
		return 1;
	}

	uint8_t* data = new uint8_t[data_length];

	signedfile.seekg(0);
	signedfile.read((char*)data, data_length);
	uint32_t crc = crc32(data, data_length);
	delete[] data;
	if(crc != suffix.dwCRC) {
		std::cerr << "CRC mismatch: calculated: 0x%x, found: 0x%x\n", crc, suffix.dwCRC;
		signedfile.close();
		return 1;
	}
	signedfile.close();
	return 0;
}

/* Add a DFU suffix to infile and write to outfile
 * suffix must already contain VID/PID
 */
int sign(const std::string& filename, uint16_t idVendor, uint16_t idProduct) {
	std::ifstream infile(filename, std::ios::binary | std::ios::ate);
	std::ofstream outfile(filename.substr(0,filename.find_last_of('.'))+".dfu", std::ios::binary);
	DFU_suffix* suffix;

	infile.seekg(0, infile.end);
	size_t data_length = infile.tellg();
	size_t buffer_length = data_length + sizeof(DFU_suffix); // Add suffix

	uint8_t* buffer = new uint8_t[buffer_length];

	infile.seekg(0);
	data_length = infile.readsome((char*)buffer, data_length);
	infile.close();

	suffix = (DFU_suffix *) (buffer + data_length);
	suffix->idVendor    = idVendor;
	suffix->idProduct   = idProduct;
	suffix->bcdDevice   = 0;
	suffix->bcdDFU      = 0x0100;
	suffix->ucDfuSig[0] = 0x55;
	suffix->ucDfuSig[1] = 0x46;
	suffix->ucDfuSig[2] = 0x44;
	suffix->bLength     = 16;
	suffix->dwCRC = crc32(buffer, data_length + 12);

	outfile.write((char*)buffer, buffer_length);
	outfile.close();
	delete[] buffer;
	return 0;
}

static void usage()
{
	std::cout << "ubertooth-dfu - Ubertooth firmware update tool\n";
	std::cout << "\n";
	std::cout << "To update firmware, run:\n";
	std::cout << "\tubertooth-dfu -d bluetooth_rxtx.dfu -r\n";
	std::cout << "\n";
	std::cout << "Usage:\n";
	std::cout << "\t-u <filename> upload - read firmware from device\n";
	std::cout << "\t-d <filename> download - write DFU file to device\n";
	std::cout << "\t-r reset Ubertooth after other operations complete\n";
	std::cout << "\n";
	std::cout << "Miscellaneous:\n";
	std::cout << "\t-s <filename> add DFU suffix to binary firmware file\n";
	std::cout << "\t-U <0-7> set ubertooth device to use\n";
}

int main(int argc, char** argv) {
	std::string downfile_name;
	std::string upfile_name;
	std::string infile_name;
	bool upload, download, reset = false;
	int opt, ubertooth_device = -1;

	int rv;

	while ((opt=getopt(argc,argv,"hd:u:s:rU:")) != EOF) {
		switch(opt) {
		case 'd':
			downfile_name = optarg;
			download = true;
			break;
		case 'u':
			upfile_name = optarg;
			upload = true;
			break;
		case 's':
			infile_name = optarg;
			sign(infile_name, U1_DFU_VENDORID, U1_DFU_PRODUCTID);
			break;
		case 'r':
			reset = true;
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		default:
		case 'h':
			usage();
			return 1;
		}
	}

	if (upload || download || reset) {
		DfuUbertooth ut(ubertooth_device);

		if(upload) {
			rv = ut.upload(upfile_name);
			if(rv) {
				std::cerr << "Firmware update failed\n";
				return rv;
			}
		}

		if(download) {
			rv = check_suffix(downfile_name);
			if(rv) {
				std::cerr << "Signature check failed, firmware will not be update\n";
				return rv;
			}
			rv = ut.download(downfile_name);
			if(rv) {
				std::cerr << "Firmware update failed\n";
				return rv;
			}
		}

		if(reset) {
			ut.detach();
		}
	}

	return 0;
}
