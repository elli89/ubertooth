#pragma once

#include "ubertooth_interface.h"
#include <queue>

class Packetsource
{
public:
	Packetsource();
	~Packetsource();

	void start();
	void stop();

	usb_pkt_rx receive();
protected:
	std::queue<usb_pkt_rx> fifo;
};
