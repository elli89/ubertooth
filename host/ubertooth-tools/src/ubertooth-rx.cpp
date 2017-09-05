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

#include "ubertooth.h"
#include <btbb.h>
#include <cstdlib>
#include <getopt.h>
#include <iomanip>
#include <iostream>

Ubertooth ut;

static void
usage()
{
    std::cout << "ubertooth-dump - output a continuous stream of received bits\n";
    std::cout << "Usage:\n";
    std::cout << "\t-h this help\n";
    std::cout << "\t-U<0-7> set ubertooth device to use\n";
}

void ubertooth_unpack_symbols(const uint8_t* buf, std::array<bool, BANK_LEN>& unpacked)
{
    for (size_t i = 0; i < SYM_LEN; i++) {
        for (size_t j = 0; j < 8; j++) {
            unpacked[i * 8 + j] = (bool)((buf[i] << j) & 0x80) >> 7;
        }
    }
}

void ubertooth_rx(usb_pkt_rx* rx, btbb_piconet* pn)
{
    const int max_ac_errors = 3;
    std::array<bool, BANK_LEN> syms;

    int offset;
    uint16_t clk_offset;
    int r;
    uint32_t lap = LAP_ANY;
    // uint8_t uap = UAP_ANY;
    uint32_t clkn;

    btbb_packet* pkt;

    ubertooth_unpack_symbols(rx->data, syms);

    if (rx->pkt_type != PacketType::BR_PACKET) {
        goto out;
    }

    if (rx->status & PacketStatus::DISCARD) {
        goto out;
    }

    /* Sanity check */
    if (rx->channel > (NUM_BREDR_CHANNELS - 1))
        goto out;

    /* Look for packets with specified LAP, if given. Otherwise
     * search for any packet. */
    if (pn) {
        lap = btbb_piconet_get_flag(pn, BTBB_LAP_VALID) ? btbb_piconet_get_lap(pn)
                                                        : LAP_ANY;
        // uap = btbb_piconet_get_flag(pn, BTBB_UAP_VALID) ? btbb_piconet_get_uap(pn)
        //                                                 : UAP_ANY;
    }

    /* Pass packet-pointer-pointer so that
     * packet can be created in libbtbb. */
    offset = btbb_find_ac((char*)syms.data(), BANK_LEN, lap, max_ac_errors, &pkt);
    if (offset < 0)
        goto out;

    /* calculate the offset between the first bit of the AC and the rising edge
     * of CLKN */
    clk_offset = (le32toh(rx->clk100ns) + offset * 10 + 6250 - 4000) % 6250;

    btbb_packet_set_modulation(pkt, BTBB_MOD_GFSK);
    btbb_packet_set_transport(pkt, BTBB_TRANSPORT_ANY);

    /* Once offset is known for a valid packet, copy in symbols
     * and other rx data. CLKN here is the 312.5us CLK27-0. The
     * btbb library can shift it be CLK1 if needed. */
    clkn = (le32toh(rx->clkn_high) << 20) + (le32toh(rx->clk100ns) + offset * 10 - 4000) / 3125;
    btbb_packet_set_data(
        pkt, (char*)syms.data() + offset, BANK_LEN * 10 - offset, rx->channel, clkn);

    printf("systime=%u ch=%2d LAP=%06x err=%u clkn=%u clk_offset=%u s=%d n=%d "
           "snr=%d\n",
        (uint32_t)time(NULL),
        btbb_packet_get_channel(pkt),
        btbb_packet_get_lap(pkt),
        btbb_packet_get_ac_errors(pkt),
        clkn,
        clk_offset,
        rx->rssi_max,
        rx->rssi_min,
        rx->rssi_max - rx->rssi_min);

    r = btbb_process_packet(pkt, pn);

    if (r < 0)
        ut.cmd_start_hopping(btbb_piconet_get_clk_offset(pn), 0);

out:
    if (pkt)
        btbb_packet_unref(pkt);
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
    int ubertooth_device = -1;
    btbb_piconet* pn = NULL;

    while ((opt = getopt(argc, argv, "bhclU:")) != EOF) {
        switch (opt) {
        case 'U':
            ubertooth_device = std::stoi(optarg);
            break;
        case 'h':
        default:
            usage();
            return 1;
        }
    }

    ut.connect(ubertooth_device);

    ut.cmd_rx_syms();

    ut.start();
    while (ut.isRunning()) {
        usb_pkt_rx pkt = ut.receive();
        ubertooth_rx(&pkt, pn);
    }
    ut.stop();
    return 0;
}
