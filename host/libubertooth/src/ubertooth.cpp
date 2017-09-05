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

#include <signal.h>

#include <iostream>

#include "ubertooth.h"
#include "ubertooth_callback.h"
#include "ubertooth_control.h"
#include "ubertooth_interface.h"

#ifndef RELEASE
#define RELEASE "unknown"
#endif
#ifndef VERSION
#define VERSION "unknown"
#endif

uint32_t systime;
FILE* infile = NULL;
FILE* dumpfile = NULL;
int max_ac_errors = 2;

int do_exit = 1;
pthread_t poll_thread;

unsigned int packet_counter_max;

void print_version() {
	std::cout << "libubertooth " << VERSION << " (" << RELEASE << "), libbtbb " << btbb_get_version() << " (" << btbb_get_release() << ")" << std::endl;
}

ubertooth_t* cleanup_devh = NULL;
static void cleanup(int sig __attribute__((unused)))
{
	if (cleanup_devh)
		cleanup_devh->stop_ubertooth = 1;
}

static void cleanup_exit(int sig __attribute__((unused)))
{
	if (cleanup_devh)
		ubertooth_stop(cleanup_devh);

	exit(0);
}

void register_cleanup_handler(ubertooth_t* ut, int do_exit) {
	cleanup_devh = ut;

	/* Clean up on ctrl-C. */
	if (do_exit) {
		signal(SIGINT, cleanup_exit);
		signal(SIGQUIT, cleanup_exit);
		signal(SIGTERM, cleanup_exit);
	} else {
		signal(SIGINT, cleanup);
		signal(SIGQUIT, cleanup);
		signal(SIGTERM, cleanup);
	}
}

ubertooth_t* timeout_dev = NULL;
void stop_transfers(int sig __attribute__((unused))) {
	if (timeout_dev)
		timeout_dev->stop_ubertooth = 1;
}

void ubertooth_set_timeout(ubertooth_t* ut, int seconds) {
	/* Upon SIGALRM, call stop_transfers() */
	if (signal(SIGALRM, stop_transfers) == SIG_ERR) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}
	timeout_dev = ut;
	alarm(seconds);
}

static struct libusb_device_handle* find_ubertooth_device(int ubertooth_device)
{
	struct libusb_device **usb_list = NULL;
	struct libusb_context *ctx = NULL;
	struct libusb_device_handle *devh = NULL;
	struct libusb_device_descriptor desc;
	int usb_devs, i, r, ret, ubertooths = 0;
	int ubertooth_devs[] = {0,0,0,0,0,0,0,0};

	usb_devs = libusb_get_device_list(ctx, &usb_list);
	for(i = 0 ; i < usb_devs ; ++i) {
		r = libusb_get_device_descriptor(usb_list[i], &desc);
		if(r < 0)
			std::cerr << "couldn't get usb descriptor for dev " << i << "!" << std::endl;
		if ((desc.idVendor == TC13_VENDORID && desc.idProduct == TC13_PRODUCTID)
		    || (desc.idVendor == U0_VENDORID && desc.idProduct == U0_PRODUCTID)
		    || (desc.idVendor == U1_VENDORID && desc.idProduct == U1_PRODUCTID))
		{
			ubertooth_devs[ubertooths] = i;
			ubertooths++;
		}
	}
	if(ubertooths == 1) {
		ret = libusb_open(usb_list[ubertooth_devs[0]], &devh);
		if (ret)
			show_libusb_error(ret);
	}
	else if (ubertooths == 0)
		return NULL;
	else {
		if (ubertooth_device < 0) {
			std::cerr << "multiple Ubertooth devices found! Use '-U' to specify device number" << std::endl;
			uint8_t serial[17], r;
			for(i = 0 ; i < ubertooths ; ++i) {
				libusb_get_device_descriptor(usb_list[ubertooth_devs[i]], &desc);
				ret = libusb_open(usb_list[ubertooth_devs[i]], &devh);
				if (ret) {
					std::cerr << "  Device " << i << ": "
					show_libusb_error(ret);
				}
				else {
					r = cmd_get_serial(devh, serial);
					if(r==0) {
						std::cerr << "  Device " << i << ": "
						print_serial(serial, stderr);
					}
					libusb_close(devh);
				}
			}
			devh = NULL;
		} else {
			ret = libusb_open(usb_list[ubertooth_devs[ubertooth_device]], &devh);
			if (ret) {
					show_libusb_error(ret);
					devh = NULL;
				}
		}
	}
libusb_free_device_list(usb_list,1);
	return devh;
}

int Ubertooth::stream_rx_usb(rx_callback cb, void* cb_args)
{
	// init USB transfer
	int r = bulk_init();
	if (r < 0)
		return r;

	r = ubertooth_bulk_thread_start();
	if (r < 0)
		return r;

	// tell ubertooth to send packets
	r = cmd_rx_syms(devh);
	if (r < 0)
		return r;

	// receive and process each packet
	while(!stop_ubertooth) {
		bulk_wait();
		r = bulk_receive(cb, cb_args);
	}

	bulk_thread_stop();

	return 1;
}

/* file should be in full USB packet format (ubertooth-dump -f) */
int stream_rx_file(ubertooth_t* ut, FILE* fp, rx_callback cb, void* cb_args)
{
	uint8_t buf[PKT_LEN];
	size_t nitems;

	while(1) {
		uint32_t systime_be;
		nitems = fread(&systime_be, sizeof(systime_be), 1, fp);
		if (nitems != 1)
			return 0;
		systime = (time_t)be32toh(systime_be);

		nitems = fread(buf, sizeof(buf[0]), PKT_LEN, fp);
		if (nitems != PKT_LEN)
			return 0;
		fifo.push((usb_pkt_rx*)buf);
		(*cb)(ut, cb_args);
	}
}

void rx_afh(ubertooth_t* ut, btbb_piconet* pn, int timeout)
{
	int r = btbb_init(max_ac_errors);
	if (r < 0)
		return;

	cmd_set_channel(ut->devh, 9999);

	if (timeout) {
		ubertooth_set_timeout(ut, timeout);

		cmd_afh(ut->devh);
		stream_rx_usb(ut, cb_afh_initial, pn);

		cmd_stop(ut->devh);
		ut->stop_ubertooth = 0;

		btbb_print_afh_map(pn);
	}

	/*
	 * Monitor changes in AFH channel map
	 */
	cmd_clear_afh_map(ut->devh);
	cmd_afh(ut->devh);
	stream_rx_usb(ut, cb_afh_monitor, pn);
}

void rx_afh_r(ubertooth_t* ut, btbb_piconet* pn, int timeout __attribute__((unused)))
{
	static uint32_t lasttime;

	int r = btbb_init(max_ac_errors);
	int i, j;
	if (r < 0)
		return;

	cmd_set_channel(ut->devh, 9999);

	cmd_afh(ut->devh);

	// init USB transfer
	r = ubertooth_bulk_init(ut);
	if (r < 0)
		return;

	Bulk::thread_start();

	// tell ubertooth to send packets
	r = cmd_rx_syms(ut->devh);
	if (r < 0)
		return;

	// receive and process each packet
	while(!ut->stop_ubertooth) {
		ubertooth_bulk_receive(ut, cb_afh_r, pn);
		if(lasttime < time(NULL)) {
			lasttime = time(NULL);
			std::cout << (uint32_t)time(NULL) << " ";
			// btbb_print_afh_map(pn);

			uint8_t* afh_map = btbb_piconet_get_afh_map(pn);
			for (i=0; i<10; i++)
				for (j=0; j<8; j++)
					if (afh_map[i] & (1<<j))
						std::cout << 1;
					else
						std::cout << 0;
			std::cout << std::endl;
		}
	}

	Bulk::thread_stop();
}

void rx_btle_file(FILE* fp)
{
	ubertooth_t* ut = ubertooth_init();
	if (ut == NULL)
		return;

	stream_rx_file(ut, fp, cb_btle, NULL);
}

void ubertooth_unpack_symbols(const uint8_t* buf, bool* unpacked)
{
	int i, j;

	for (i = 0; i < SYM_LEN; i++) {
		/* output one byte for each received symbol (0x00 or 0x01) */
		for (j = 0; j < 8; j++) {
			unpacked[i * 8 + j] = ((buf[i] << j) & 0x80) >> 7;
		}
	}
}

static void cb_dump_bitstream(void* args __attribute__((unused)))
{
	int i;
	char nl = '\n';

	usb_pkt_rx usb = fifo.pop();
	usb_pkt_rx* rx = &usb;
	char bitstream[BANK_LEN];
	ubertooth_unpack_symbols((uint8_t*)rx->data, (bool*)bitstream);

	// convert to ascii
	for (i = 0; i < BANK_LEN; ++i)
		bitstream[i] += 0x30;

	std::cerr << "rx block timestamp " << rx->clk100ns << " * 100 nanoseconds" << std::endl;
	if (dumpfile == NULL) {
		fwrite(bitstream, sizeof(uint8_t), BANK_LEN, stdout);
		fwrite(&nl, sizeof(uint8_t), 1, stdout);
	} else {
		fwrite(bitstream, sizeof(uint8_t), BANK_LEN, dumpfile);
		fwrite(&nl, sizeof(uint8_t), 1, dumpfile);
	}
}

static void cb_dump_full(ubertooth_t* ut, void* args __attribute__((unused)))
{
	usb_pkt_rx usb = fifo.pop();
	usb_pkt_rx* rx = &usb;

	std::cerr << "rx block timestamp " << rx->clk100ns << " * 100 nanoseconds" << std::endl;
	uint32_t time_be = htobe32((uint32_t)time(NULL));
	if (dumpfile == NULL) {
		fwrite(&time_be, 1, sizeof(time_be), stdout);
		fwrite((uint8_t*)rx, sizeof(uint8_t), PKT_LEN, stdout);
	} else {
		fwrite(&time_be, 1, sizeof(time_be), dumpfile);
		fwrite((uint8_t*)rx, sizeof(uint8_t), PKT_LEN, dumpfile);
		fflush(dumpfile);
	}
}

/* dump received symbols to stdout */
void rx_dump(ubertooth_t* ut, int bitstream)
{
	if (bitstream)
		stream_rx_usb(ut, cb_dump_bitstream, NULL);
	else
		stream_rx_usb(ut, cb_dump_full, NULL);
}

void Ubertooth::stop()
{
	stop_ubertooth = true;
}

Ubertooth::~Ubertooth()
{
	if (devh != NULL) {
		cmd_stop(devh);
		libusb_release_interface(devh, 0);
	}
	libusb_close(devh);
	libusb_exit(NULL);

	if (h_pcap_bredr) {
		btbb_pcap_close(h_pcap_bredr);
		h_pcap_bredr = NULL;
	}
	if (h_pcap_le) {
		lell_pcap_close(h_pcap_le);
		h_pcap_le = NULL;
	}

	if (h_pcapng_bredr) {
		btbb_pcapng_close(h_pcapng_bredr);
		h_pcapng_bredr = NULL;
	}
	if (h_pcapng_le) {
		lell_pcapng_close(h_pcapng_le);
		h_pcapng_le = NULL;
	}
}

Ubertooth::Ubertooth()
{
	devh = NULL;
	bulk = NULL;
	stop_ubertooth = false;
	abs_start_ns = 0;
	start_clk100ns = 0;
	last_clk100ns = 0;
	clk100ns_upper = 0;

	h_pcap_bredr = NULL;
	h_pcap_le = NULL;
	h_pcapng_bredr = NULL;
	h_pcapng_le = NULL;
}

int Ubertooth::connect(int ubertooth_device)
{
	int r = libusb_init(NULL);
	if (r < 0) {
		std::cerr << "libusb_init failed (got 1.0?)" << std::endl;
		return -1;
	}

	devh = find_ubertooth_device(ubertooth_device);
	if (devh == NULL) {
		std::cerr << "could not open Ubertooth device" << std::endl;
		stop();
		return -1;
	}

	r = libusb_claim_interface(devh, 0);
	if (r < 0) {
		std::cerr << "usb_claim_interface error " << r << std::endl;
		stop();
		return -1;
	}

	bulk = new Bulk(devh);

	return 1;
}

Ubertooth::Ubertooth(int ubertooth_device)
{
	Ubertooth();

	connect(ubertooth_device);
}

int Ubertooth::get_api(uint16_t* version) {
	struct libusb_device_descriptor desc;
	libusb_device* dev = libusb_get_device(devh);
	int result = libusb_get_device_descriptor(dev, &desc);
	if (result < 0) {
		if (result == LIBUSB_ERROR_PIPE) {
			std::cerr << "control message unsupported" << std::endl;
		} else {
			show_libusb_error(result);
		}
		return result;
	}
	*version = desc.bcdDevice;
	return 0;
}

int Ubertooth::check_api() {
	uint16_t version;
	int result = get_api(&version);
	if (result < 0) {
		return result;
	}

	if (version < UBERTOOTH_API_VERSION) {
		std::cerr << "Ubertooth API version "
		          << ((version>>8)&0xFF) << "." << (version&0xFF)
				  << " found, libubertooth "
				  << VERSION
				  << " requires "
				  << ((version>>8)&0xFF) << "." << (version&0xFF)
				  << std::endl;
		std::cerr << "Please upgrade to latest released firmware." << std::endl;
		std::cerr << "See: https://github.com/greatscottgadgets/ubertooth/wiki/Firmware" << std::endl;
		stop();
		return -1;
	}
	else if (version > UBERTOOTH_API_VERSION) {
		std::cerr << "Ubertooth API version "
		          << ((version>>8)&0xFF) << "." << (version&0xFF)
		          << " found, newer than that supported by libubertooth ("
		          << ((UBERTOOTH_API_VERSION>>8)&0xFF) << "." << (UBERTOOTH_API_VERSION&0xFF)
		          << ")." << std::endl;
		std::cerr << "Things will still work, but you might want to update your host tools." << std::endl;
	}
	return 0;
}
