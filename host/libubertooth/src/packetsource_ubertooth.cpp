#include <chrono>

#include "packetsource_ubertooth.h"
#include "ubertooth_control.h"

PacketsourceUbertooth::PacketsourceUbertooth(libusb_device_handle* devh)
{
	this->devh = devh;
	stop_transfer = true;
}

PacketsourceUbertooth::~PacketsourceUbertooth()
{
	stop();

	delete poll_thread;
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
				fprintf(stderr, "Failed to submit USB transfer (%d)\n", r);
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
		fprintf(stderr, "Failed to submit USB transfer (%d)\n", r);
}

void PacketsourceUbertooth::start()
{
	stop_transfer = false;

	rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(rx_xfer, devh, DATA_IN, (uint8_t*)buffer, PKT_LEN, cb_xfer, this, TIMEOUT);

	int r = libusb_submit_transfer(rx_xfer);
	if (r < 0) {
		fprintf(stderr, "rx_xfer submission: %d\n", r);
	}

	PacketsourceUbertooth::thread_start();
}

void PacketsourceUbertooth::stop()
{
	stop_transfer = true;

	if(rx_xfer != NULL)
		libusb_cancel_transfer(rx_xfer);

	PacketsourceUbertooth::exit_thread = true;
	PacketsourceUbertooth::thread_stop();
}

usb_pkt_rx PacketsourceUbertooth::receive()
{
	while (fifo.empty() && !stop_transfer)
		std::this_thread::sleep_for(std::chrono::microseconds(1));

	// FIXME handle empty fifo
	usb_pkt_rx pkt = fifo.front();
	fifo.pop();
	return pkt;
}

void PacketsourceUbertooth::thread_start()
{
	exit_thread = false;

	PacketsourceUbertooth::poll_thread = new std::thread(PacketsourceUbertooth::poll);
}

void PacketsourceUbertooth::thread_stop()
{
	PacketsourceUbertooth::exit_thread = true;

	PacketsourceUbertooth::poll_thread->join();
}

void PacketsourceUbertooth::poll()
{
	while (!PacketsourceUbertooth::exit_thread) {
		struct timeval tv = { 1, 0 };
		if ( libusb_handle_events_timeout(NULL, &tv) < 0 ) {
			PacketsourceUbertooth::exit_thread = true;
			break;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}
