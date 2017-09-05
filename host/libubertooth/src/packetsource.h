#pragma once

#include "ubertooth_interface.h"
#include <queue>

class Packetsource
{
public:
	Packetsource() {};
	virtual ~Packetsource() {};

	virtual void start()=0;
	virtual void stop()=0;
	virtual usb_pkt_rx receive()=0;

protected:
	std::queue<usb_pkt_rx> fifo;
};
