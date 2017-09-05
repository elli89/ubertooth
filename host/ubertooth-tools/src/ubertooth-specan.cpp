/*
 * Copyright 2010 Michael Ossmann
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

#include <getopt.h>
#include <string>
#include "ubertooth.h"
#include "ubertooth_interface.h"
#include <iostream>
#include <signal.h>

Ubertooth* stopdevice;
static void cleanup(int sig __attribute__((unused)))
{
	stopdevice->stop();
}

static void usage()
{
	std::cerr << "ubertooth-specan - output a continuous stream of signal strengths\n";
	std::cerr << "\n";
	std::cerr << "!!!!!\n";
	std::cerr << "NOTE: you probably want ubertooth-specan-ui\n";
	std::cerr << "!!!!!\n";
	std::cerr << "\n";
	std::cerr << "Usage:\n";
	std::cerr << "\t-h this help\n";
	std::cerr << "\t-g output suitable for feedgnuplot\n";
	std::cerr << "\t-G output suitable for 3D feedgnuplot\n";
	std::cerr << "\t-d <filename> output to file\n";
	std::cerr << "\t-l lower frequency (default 2402)\n";
	std::cerr << "\t-u upper frequency (default 2480)\n";
	std::cerr << "\t-U<0-7> set ubertooth device to use\n";
}

int main(int argc, char* argv[])
{
	int opt, r = 0;
	int lower= 2402, upper= 2480;
	int ubertooth_device = -1;

	while ((opt=getopt(argc,argv,"hgGd:l::u::U:")) != EOF) {
		switch(opt) {
		case 'd':
			break;
		case 'l':
			if (optarg)
				lower= std::stoi(optarg);
			else
				std::cout << "lower: " << lower << std::endl;
			break;
		case 'u':
			if (optarg)
				upper= std::stoi(optarg);
			else
				std::cout << "upper: " << upper << std::endl;
			break;
		case 'U':
			ubertooth_device = std::stoi(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return 1;
		}
	}

	Ubertooth ut(ubertooth_device);

	// setup cleanup handler
	stopdevice = &ut;
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);

	// tell ubertooth to start specan and send packets
	ut.cmd_specan(lower, upper);

	ut.start();

	while (ut.isRunning()) {
		usb_pkt_rx rx = ut.receive();

		/* process each received block */
		for (int j = 0; j < DMA_SIZE -2; j+=3) {
			std::cout << rx.data[j+1];
			std::cout << rx.data[j+2];
			std::cout << rx.data[j];
		}
	}

	ut.stop();
	std::cerr << "Ubertooth stopped" << std::endl;
	return r;
}
