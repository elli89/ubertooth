#include "inttypes.h"

inline int get_api(uint16_t* version) {
	struct libusb_device_descriptor desc;
	libusb_device* dev = libusb_get_device(devh);
	int result = libusb_get_device_descriptor(dev, &desc);
	if (result < 0) {
		if (result == LIBUSB_ERROR_PIPE) {
			std::cerr << "control message unsupported" << std::endl;
		} else {
			show_libusb_error(result);
		}
		return result;
	}
	*version = desc.bcdDevice;
	return 0;
}

inline int check_api() {
	uint16_t version;
	int result = get_api(&version);
	if (result < 0) {
		return result;
	}

	if (version < UBERTOOTH_API_VERSION) {
		std::cerr << "Ubertooth API version "
		          << ((version>>8)&0xFF) << "." << (version&0xFF)
				  << " found, libubertooth "
				  << VERSION
				  << " requires "
				  << ((version>>8)&0xFF) << "." << (version&0xFF)
				  << std::endl;
		std::cerr << "Please upgrade to latest released firmware." << std::endl;
		std::cerr << "See: https://github.com/greatscottgadgets/ubertooth/wiki/Firmware" << std::endl;
		stop();
		return -1;
	}
	else if (version > UBERTOOTH_API_VERSION) {
		std::cerr << "Ubertooth API version "
		          << ((version>>8)&0xFF) << "." << (version&0xFF)
		          << " found, newer than that supported by libubertooth ("
		          << ((UBERTOOTH_API_VERSION>>8)&0xFF) << "." << (UBERTOOTH_API_VERSION&0xFF)
		          << ")." << std::endl;
		std::cerr << "Things will still work, but you might want to update your host tools." << std::endl;
	}
	return 0;
}
