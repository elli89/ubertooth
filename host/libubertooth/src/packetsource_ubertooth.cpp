#include <chrono>

#include "packetsource_ubertooth.h"
#include "ubertooth_control.h"
#include <iostream> // FIXME
#include <pthread.h>
#include <unistd.h>

volatile bool exit_thread;
pthread_t poll_thread;

void* poll(void* arg __attribute__((unused)))
{
	while (!exit_thread) {
		struct timeval tv = { 1, 0 };
		if ( libusb_handle_events_timeout(NULL, &tv) < 0 ) {
			exit_thread = true;
			break;
		}
		usleep(1);
	}
	return NULL;
}

void thread_start()
{
	exit_thread = false;

	pthread_create(&poll_thread, NULL, poll, NULL);
}

void thread_stop()
{
	exit_thread = true;

	pthread_join(poll_thread, NULL);
	std::cout << "blu" << std::endl;
}

PacketsourceUbertooth::PacketsourceUbertooth(libusb_device_handle* devh)
{
	this->devh = devh;
	stop_transfer = true;
}

PacketsourceUbertooth::~PacketsourceUbertooth()
{
	stop();
}

usb_pkt_rx* buffer;

void PacketsourceUbertooth::cb_xfer(struct libusb_transfer* xfer)
{
	int r;

	PacketsourceUbertooth* bulk = (PacketsourceUbertooth*) xfer->user_data;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		if(xfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
			r = libusb_submit_transfer(xfer);
			if (r < 0)
				std::cerr << "Failed to submit USB transfer (" << r << ")" << std::endl;
			return;
		}
		libusb_free_transfer(xfer);
		xfer = NULL;
		return;
	}

	if(bulk->stop_transfer)
		return;

	bulk->fifo.push(*buffer);

	r = libusb_submit_transfer(xfer);
	if (r < 0)
		std::cerr << "Failed to submit USB transfer (" << r << ")" << std::endl;
}

void PacketsourceUbertooth::start()
{
	stop_transfer = false;

	rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(rx_xfer, devh, DATA_IN, (uint8_t*)buffer, PKT_LEN, cb_xfer, this, TIMEOUT);

	int r = libusb_submit_transfer(rx_xfer);
	if (r < 0) {
		std::cerr << "rx_xfer submission: " << r << std::endl;
	}

	thread_start();
}

void PacketsourceUbertooth::stop()
{
	stop_transfer = true;

	if(rx_xfer != NULL)
		libusb_cancel_transfer(rx_xfer);

	thread_stop();
}

usb_pkt_rx PacketsourceUbertooth::receive()
{
	while (fifo.empty() && !stop_transfer)
		usleep(1);

	// FIXME handle empty fifo
	usb_pkt_rx pkt;
	if (!fifo.empty()) {
		pkt = fifo.front();
		fifo.pop();
	}
	return pkt;
}
