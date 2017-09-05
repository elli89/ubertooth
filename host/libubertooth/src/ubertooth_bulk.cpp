#include "ubertooth_bulk.h"


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


}

Bulk::~Bulk()
{
	if(rx_xfer != NULL)
		libusb_cancel_transfer(rx_xfer);
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
