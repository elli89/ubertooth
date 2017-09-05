#include <ctime>

#include "packetsource_ubertooth.h"

PacketsourceUbertooth::PacketsourceUbertooth(libusb_device_handle* devh)
{
	this->devh = devh;
}

PacketsourceUbertooth::~PacketsourceUbertooth()
{
}

void PacketsourceUbertooth::start()
{
}

void PacketsourceUbertooth::stop()
{
}

usb_pkt_rx PacketsourceUbertooth::receive()
{
	while (fifo.empty())
		usleep(1);

	usb_pkt_rx pkt = fifo.front();
	fifo.pop();
	return pkt;
}

void PacketsourceUbertooth::poll()
{
	while (!PacketsourceUbertooth::exit_thread) {
		struct timeval tv = { 1, 0 };
		if ( libusb_handle_events_timeout(NULL, &tv) < 0 ) {
			PacketsourceUbertooth::exit_thread = true;
			break;
		}
		usleep(1);
	}
}
