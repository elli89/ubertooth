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

#include "string"
#include "ubertooth.h"
#include <cstdlib>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sighandler.h>

static void usage()
{
    std::cout << "ubertooth-dump - output a continuous stream of received bits\n";
    std::cout << "Usage:\n";
    std::cout << "\t-h this help\n";
    std::cout << "\t-b only dump received bitstream (GnuRadio style)\n";
    std::cout << "\t-c classic modulation\n";
    std::cout << "\t-l LE modulation\n";
    std::cout << "\t-U<0-7> set ubertooth device to use\n";
    std::cout << "\t-d filename\n";
    std::cout << "\nThis program sends binary data to stdout.  You probably don't want to\n";
    std::cout << "run it from a terminal without redirecting the output.\n";
}

/*
 * The normal output format is in chunks of 64 bytes in the USB RX packet format
 * 50 of those 64 bytes contain the received symbols (packed 8 per byte).
 *
 * The -b output format is a stream of bytes, each either 0x00 or 0x01
 * representing the symbol determined by the demodulator (GnuRadio style)
 */

int main(int argc, char* argv[])
{
    int opt;
    bool bitstream = false;
    Modulation mod = Modulation::BT_BASIC_RATE;
    int ubertooth_device = -1;

    while ((opt = getopt(argc, argv, "bhclU:")) != EOF) {
        switch (opt) {
        case 'b':
            bitstream = true;
            break;
        case 'c':
            mod = Modulation::BT_BASIC_RATE;
            break;
        case 'l':
            mod = Modulation::BT_LOW_ENERGY;
            break;
        case 'U':
            ubertooth_device = std::stoi(optarg);
            break;
        case 'h':
        default:
            usage();
            return 1;
        }
    }

    Ubertooth ut(ubertooth_device);

    // setup cleanup handler
    Sighandler::attach(&ut);

    ut.cmd_set_modulation(mod);

    ut.cmd_rx_syms();

    ut.start();
    while (ut.isRunning()) {
        usb_pkt_rx pkt = ut.receive();

        // print
        std::cerr << "rx block timestamp " << pkt.clk100ns << " * 100 nanoseconds\n";
        for (auto databyte : pkt.data) {
            if (bitstream) {
                for (int j = 0; j < 8; j++) {
                    std::cout << ((databyte >> (7 - j)) & 0x01);
                }
            } else {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(databyte);
            }
        }
        std::cout << std::dec << std::endl;
    }
    ut.stop();
    return 0;
}
