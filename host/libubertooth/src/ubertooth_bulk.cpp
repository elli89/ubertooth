#include "ubertooth_bulk.h"

void Bulk::cb_xfer(struct libusb_transfer *xfer)
{
	int r;

	Bulk* bulk = (Bulk*) xfer->user_data;

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

	bulk->fifo.push(bulk->buffer);

	r = libusb_submit_transfer(xfer);
	if (r < 0)
		fprintf(stderr, "Failed to submit USB transfer (%d)\n", r);
}

void Bulk::poll()
{
	int r = 0;

	while (!Bulk::exit_thread) {
		struct timeval tv = { 1, 0 };
		r = libusb_handle_events_timeout(NULL, &tv);
		if (r < 0) {
			Bulk::exit_thread = true;
			break;
		}
		usleep(1);
	}
}

int Bulk::thread_start()
{
	exit_thread = false;

	Bulk::poll_thread = std::thread(Bulk::poll);
}

void Bulk::thread_stop()
{
	Bulk::exit_thread = true;

	Bulk::poll_thread.join();
}

Bulk::Bulk(libusb_device_handle* devh)
{
	int r;

	stop_transfer = false;

	rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(rx_xfer, devh, DATA_IN, (uint8_t*)buffer, PKT_LEN, cb_xfer, this, TIMEOUT);

	r = libusb_submit_transfer(rx_xfer);
	if (r < 0) {
		fprintf(stderr, "rx_xfer submission: %d\n", r);
	}
}

Bulk::~Bulk()
{
	if(rx_xfer != NULL)
		libusb_cancel_transfer(rx_xfer);
}

void Bulk::wait()
{
	while (fifo.empty() && !stop_transfer)
		usleep(1);
}

int Bulk::receive(rx_callback cb, void* cb_args)
{
	if (!fifo.empty()) {
		(*cb)(cb_args);
		if(stop_transfer) {
			if(rx_xfer)
				libusb_cancel_transfer(rx_xfer);
			return 1;
		}
		fflush(stderr);
		return 0;
	} else {
		usleep(1);
		return -1;
	}
}
