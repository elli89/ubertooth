/*
 * Copyright 2012 Dominic Spill
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

#ifndef __UBERTOOTH_INTERFACE_H
#define __UBERTOOTH_INTERFACE_H


#if defined __MACH__
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#define htobe32 EndianU32_NtoB
#define be32toh EndianU32_BtoN
#define le32toh EndianU32_LtoN
#define htobe64 EndianU64_NtoB
#define be64toh EndianU64_BtoN
#define htole16 EndianU16_NtoL
#define htole32 EndianU32_NtoL
#else
#include <endian.h>
#endif

#include <stdint.h>

// increment on every API change
#define UBERTOOTH_API_VERSION 0x0102

#define DMA_SIZE 50

#define NUM_BREDR_CHANNELS 79

#define U0_VENDORID    0x1d50
#define U0_PRODUCTID   0x6000
#define U1_VENDORID    0x1d50
#define U1_PRODUCTID   0x6002
#define TC13_VENDORID  0xffff
#define TC13_PRODUCTID 0x0004

#define DATA_IN     (0x82 | LIBUSB_ENDPOINT_IN)
#define DATA_OUT    (0x05 | LIBUSB_ENDPOINT_OUT)
#define TIMEOUT     20000

/* RX USB packet parameters */
#define PKT_LEN       64
#define SYM_LEN       50
#define BANK_LEN      (SYM_LEN * 8)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

/*
 * CLK_TUNE_TIME is the duration in units of 100 ns that we reserve for tuning
 * the radio while frequency hopping.  We start the tuning process
 * CLK_TUNE_TIME * 100 ns prior to the start of an upcoming time slot.
*/
#define CLK_TUNE_TIME   2250
#define CLK_TUNE_OFFSET  200

enum class Command : uint8_t {
	PING               = 0,
	RX_SYMBOLS         = 1,
	TX_SYMBOLS         = 2,
	GET_USRLED         = 3,
	SET_USRLED         = 4,
	GET_RXLED          = 5,
	SET_RXLED          = 6,
	GET_TXLED          = 7,
	SET_TXLED          = 8,
	GET_1V8            = 9,
	SET_1V8            = 10,
	GET_CHANNEL        = 11,
	SET_CHANNEL        = 12,
	RESET              = 13,
	GET_SERIAL         = 14,
	GET_PARTNUM        = 15,
	GET_PAEN           = 16,
	SET_PAEN           = 17,
	GET_HGM            = 18,
	SET_HGM            = 19,
	TX_TEST            = 20,
	STOP               = 21,
	GET_MOD            = 22,
	SET_MOD            = 23,
	SET_ISP            = 24,
	FLASH              = 25,
	// BOOTLOADER_FLASH   = 26,
	SPECAN             = 27,
	GET_PALEVEL        = 28,
	SET_PALEVEL        = 29,
	REPEATER           = 30,
	RANGE_TEST         = 31,
	RANGE_CHECK        = 32,
	GET_REV_NUM        = 33,
	LED_SPECAN         = 34,
	GET_BOARD_ID       = 35,
	SET_SQUELCH        = 36,
	GET_SQUELCH        = 37,
	SET_BDADDR         = 38,
	START_HOPPING      = 39,
	SET_CLOCK          = 40,
	GET_CLOCK          = 41,
	BTLE_SNIFFING      = 42,
	GET_ACCESS_ADDRESS = 43,
	SET_ACCESS_ADDRESS = 44,
	DO_SOMETHING       = 45,
	DO_SOMETHING_REPLY = 46,
	GET_CRC_VERIFY     = 47,
	SET_CRC_VERIFY     = 48,
	POLL               = 49,
	BTLE_PROMISC       = 50,
	SET_AFHMAP         = 51,
	CLEAR_AFHMAP       = 52,
	READ_REGISTER      = 53,
	BTLE_SLAVE         = 54,
	GET_COMPILE_INFO   = 55,
	BTLE_SET_TARGET    = 56,
	BTLE_PHY           = 57,
	WRITE_REGISTER     = 58,
	JAM_MODE           = 59,
	EGO                = 60,
	AFH                = 61,
	HOP                = 62,
	TRIM_CLOCK         = 63,
	// GET_API_VERSION    = 64,
	WRITE_REGISTERS    = 65,
	READ_ALL_REGISTERS = 66,
	RX_GENERIC         = 67,
	TX_GENERIC_PACKET  = 68,
	FIX_CLOCK_DRIFT    = 69
};

enum class jam_modes {
	JAM_NONE       = 0,
	JAM_ONCE       = 1,
	JAM_CONTINUOUS = 2,
};

enum class modulations {
	MOD_BT_BASIC_RATE = 0,
	MOD_BT_LOW_ENERGY = 1,
	MOD_80211_FHSS    = 2,
	MOD_NONE          = 3
};

enum class usb_pkt_types {
	BR_PACKET  = 0,
	LE_PACKET  = 1,
	MESSAGE    = 2,
	KEEP_ALIVE = 3,
	SPECAN     = 4,
	LE_PROMISC = 5,
	EGO_PACKET = 6,
};

enum class hop_mode {
	HOP_NONE      = 0,
	HOP_SWEEP     = 1,
	HOP_BLUETOOTH = 2,
	HOP_BTLE      = 3,
	HOP_DIRECT    = 4,
	HOP_AFH       = 5,
};

enum class usb_pkt_status {
	DMA_OVERFLOW  = 0x01,
	DMA_ERROR     = 0x02,
	FIFO_OVERFLOW = 0x04,
	CS_TRIGGER    = 0x08,
	RSSI_TRIGGER  = 0x10,
	DISCARD       = 0x20,
};

/*
 * USB packet for Bluetooth RX (64 total bytes)
 */
typedef struct {
	uint8_t  pkt_type;
	uint8_t  status;
	uint8_t  channel;
	uint8_t  clkn_high;
	uint32_t clk100ns;
	int8_t   rssi_max;   // Max RSSI seen while collecting symbols in this packet
	int8_t   rssi_min;   // Min ...
	int8_t   rssi_avg;   // Average ...
	uint8_t  rssi_count; // Number of ... (0 means RSSI stats are invalid)
	uint8_t  reserved[2];
	uint8_t  data[DMA_SIZE];
} usb_pkt_rx;

typedef struct {
	uint64_t address;
	uint64_t syncword;
} bdaddr;

typedef struct {
	uint8_t valid;
	uint8_t request_pa;
	uint8_t request_num;
	uint8_t reply_pa;
	uint8_t reply_num;
} rangetest_result;

typedef struct {
	uint16_t synch;
	uint16_t syncl;
	uint16_t channel;
	uint8_t  length;
	uint8_t  pa_level;
	uint8_t  data[DMA_SIZE];
} generic_tx_packet;

#endif /* __UBERTOOTH_INTERFACE_H */
