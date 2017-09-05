#pragma once

#include <libusb-1.0/libusb.h>
#include "ubertooth_interface.h"
#include "packetsource.h"

class PacketsourceUbertooth : public Packetsource
{
public:
	PacketsourceUbertooth(libusb_device_handle* devh);
	~PacketsourceUbertooth();

	void start();
	void stop();

	usb_pkt_rx receive();

protected:
	libusb_device_handle* devh = NULL;
	struct libusb_transfer* rx_xfer = NULL;

	bool stop_transfer;

	static void thread_start();
	static void thread_stop();

	static bool exit_thread;
	static pthread_t poll_thread;
	static void* poll(void* arg __attribute__((unused)));

	static void cb_xfer(struct libusb_transfer* xfer);
	static usb_pkt_rx buffer;
};
