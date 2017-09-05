#pragma once

#include <libusb-1.0/libusb.h>
#include "ubertooth_interface.h"
#include "packetsource.h"
#include <queue>
#include <thread>

class PacketsourceUbertooth : public Packetsource
{
public:
	PacketsourceUbertooth(libusb_device_handle* devh);
	~PacketsourceUbertooth();

	void start();
	void stop();

	usb_pkt_rx receive();

protected:
	libusb_device_handle* devh;
	struct libusb_transfer* rx_xfer;

	bool stop_transfer;

	static void thread_start();
	static void thread_stop();

	static bool exit_thread;
	static std::thread* poll_thread;
	static void poll();

	static void cb_xfer(struct libusb_transfer* xfer);
};
