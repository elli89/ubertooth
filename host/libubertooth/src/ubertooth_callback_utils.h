/*
 * Copyright 2015 Hannes Ellinger
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

#ifndef __UBERTOOTH_CALLBACK_UTILS_H__
#define __UBERTOOTH_CALLBACK_UTILS_H__

#include "ubertooth_control.h"
#include "ubertooth.h"

int8_t cc2400_rssi_to_dbm( const int8_t rssi );
void determine_signal_and_noise( usb_pkt_rx *rx, int8_t * sig, int8_t * noise );
uint64_t now_ns( void );
void track_clk100ns( ubertooth_t* ut, const usb_pkt_rx* rx );
uint64_t now_ns_from_clk100ns( ubertooth_t* ut, const usb_pkt_rx* rx );
int trim(ubertooth_t* ut, usb_pkt_rx* rx, int offset);
void dump(FILE* dumpfile, uint32_t systime, usb_pkt_rx* rx);
void pcap(ubertooth_t* ut, usb_pkt_rx* rx, uint16_t lap, uint8_t uap, btbb_packet* pkt);
void print_br(uint32_t systime, usb_pkt_rx* rx, btbb_packet* pkt);
int find_packet(usb_pkt_rx* rx, uint32_t lap, btbb_packet** pkt);
void fill_packet(usb_pkt_rx* rx, btbb_packet* pkt, int offset);

#endif /* __UBERTOOTH_CALLBACK_UTILS_H__ */
