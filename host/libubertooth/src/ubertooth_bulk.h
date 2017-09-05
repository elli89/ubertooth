#pragma once

#include <libusb-1.0/libusb.h>
#include <queue>
#include <thread>
#include "ubertooth_interface.h"
#include "ubertooth_control.h"
#include "ubertooth_callback.h"
#include "packetsource.h"

class Bulk : public Packetsource
{
private:
	std::queue<usb_pkt_rx> fifo;
	struct libusb_transfer* rx_xfer;


	usb_pkt_rx* buffer;

	bool stop_transfer;

	static void cb_xfer(struct libusb_transfer* xfer);

	static bool exit_thread;
	static std::thread poll_thread;
	static void poll();

public:
	Bulk(libusb_device_handle* devh);
	~Bulk();

	void wait();
	int receive(rx_callback cb, void* cb_args);
	void stop();
	static int thread_start();
	static void thread_stop();
};
