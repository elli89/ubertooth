#include "ubertooth_bulk.h"


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
