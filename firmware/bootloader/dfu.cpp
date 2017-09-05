/*
 * Copyright 2010, 2011 Michael Ossmann
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "dfu.h"

#include <string.h>

struct memory_policy {
	static bool write_permitted(const uint32_t flash_address) {
		// Protect flash region containing bootloader.
		return (flash_address >= 0x4000);
	}
};

DFU::DFU(Flash& flash) :
	flash(flash),
	status(DfuStatus::OK),
	state(DfuState::DFU_IDLE),
	virginity(true) {
}

bool DFU::request_handler(TSetupPacket *pSetup, uint32_t *piLen, uint8_t **ppbData) {
	uint8_t* pbData = *ppbData;

	switch( (DfuCommand)pSetup->bRequest ) {
	case DfuCommand::DETACH:
		return request_detach(pSetup, piLen, pbData);

	case DfuCommand::DOWNLOAD:
		return request_dnload(pSetup, piLen, pbData);

	case DfuCommand::UPLOAD:
		return request_upload(pSetup, piLen, pbData);

	case DfuCommand::GET_STATUS:
		return request_getstatus(pSetup, piLen, pbData);

	case DfuCommand::CLEAR_STATUS:
		return request_clrstatus(pSetup, piLen, pbData);

	case DfuCommand::GET_STATE:
		return request_getstate(pSetup, piLen, pbData);

	case DfuCommand::ABORT:
		return request_abort(pSetup, piLen, pbData);

	default:
		return false;
	}
}

bool DFU::in_dfu_mode() const {
	return (get_state() >= DfuState::DFU_IDLE);
}

bool DFU::dfu_virgin() const {
	return virginity;
}

void DFU::set_state(const DfuState new_state) {
	state = new_state;
	virginity = false;
}

DfuState DFU::get_state() const {
	return state;
}

void DFU::set_status(const DfuStatus new_status) {
	status = new_status;
}

DfuStatus DFU::get_status() const {
	return status;
}

bool DFU::error(const DfuStatus new_status) {
	if( (get_state() != DfuState::APP_IDLE) &&
		(get_state() != DfuState::APP_DETACH) ) {
		 set_state(DfuState::DFU_ERROR);
	}
	set_status(new_status);
	return false;
}

uint8_t DFU::get_status_string_id() const {
	return 0;
}

uint32_t DFU::get_poll_timeout() const {
	return 20;  // milliseconds
}

bool DFU::request_detach(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* pbData) {
	if( (pSetup->wLength == 0) && (pSetup->wValue <= detach_timeout_ms) ) {
		// TODO: Check DFU vs. APP mode, and reboot device if in DFU mode?
		set_state(DfuState::APP_DETACH);
		return true;
	} else {
		return error(DfuStatus::ERR_UNKNOWN);
	}
}

bool DFU::request_dnload(TSetupPacket *pSetup, uint32_t *piLen, uint8_t *pbData) {
	if( pSetup->wLength == 0 ) {
		if( get_state() != DfuState::DFU_DOWNLOAD_IDLE ) {
			return error(DfuStatus::ERR_STALLEDPKT);
		}

		// End of transfer
		set_state(DfuState::DFU_MANIFEST_SYNC);

		return true;
	} else if( pSetup->wLength == transfer_size ) {
		if( (get_state() != DfuState::DFU_IDLE) && (get_state() != DfuState::DFU_DOWNLOAD_IDLE) ) {
			return error(DfuStatus::ERR_STALLEDPKT);
		}

		// TODO: Improve returned error values.
		const uint32_t length = pSetup->wLength;
		const uint32_t flash_address = pSetup->wValue * transfer_size;
		const uint32_t source_address = reinterpret_cast<uint32_t>(pbData);
		if( memory_policy::write_permitted(flash_address) ) {
			if( flash.write(flash_address, source_address, length) ) {
				set_state(DfuState::DFU_DOWNLOAD_SYNC);
			} else {
				return error(DfuStatus::ERR_PROG);
			}
			return true;
		} else {
			return error(DfuStatus::ERR_WRITE);
		}
	} else {
		return error(DfuStatus::ERR_UNKNOWN);
	}
}

bool DFU::request_upload(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* pbData) {
	if( pSetup->wLength == transfer_size ) {
		if( (get_state() != DfuState::DFU_IDLE) && (get_state() != DfuState::DFU_UPLOAD_IDLE) ) {
			return error(DfuStatus::ERR_STALLEDPKT);
		}

		const uint32_t length = pSetup->wLength;
		const uint32_t flash_address_start = pSetup->wValue * transfer_size;
		const uint32_t flash_address_end = flash_address_start + length;
		if( flash.valid_address(flash_address_start) && flash.valid_address(flash_address_end - 1) ) {
			memcpy(pbData, reinterpret_cast<const void*>(flash_address_start), length);
			*piLen = length;
			set_state(DfuState::DFU_IDLE);
			return true;
		} else {
			return error(DfuStatus::ERR_ADDRESS);
		}
	} else {
		return error(DfuStatus::ERR_UNKNOWN);
	}
}

bool DFU::request_getstatus(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* pbData) {
	if( (pSetup->wValue == 0) && (pSetup->wLength == 6) ) {
		switch( get_state() ) {
			case DfuState::DFU_DOWNLOAD_SYNC:
				set_state(DfuState::DFU_DOWNLOAD_IDLE);
				break;

			case DfuState::DFU_MANIFEST_SYNC:
				set_state(DfuState::DFU_IDLE);
				break;

			default:
				break;
		}

		pbData[0] = (uint8_t)get_status();
		pbData[1] = (get_poll_timeout() >>  0) & 0xFF;
		pbData[2] = (get_poll_timeout() >>  8) & 0xFF;
		pbData[3] = (get_poll_timeout() >> 16) & 0xFF;
		pbData[4] = (uint8_t)get_state();
		pbData[5] = get_status_string_id();
		*piLen = 6;
		return true;
	} else {
		return false;
	}
}

bool DFU::request_clrstatus(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* pbData) {
	if( (pSetup->wValue == 0) && (pSetup->wLength == 0) ) {
		if( get_state() == DfuState::DFU_ERROR ) {
			set_status(DfuStatus::OK);
			set_state(DfuState::DFU_IDLE);
			return true;
		} else {
			return error(DfuStatus::ERR_UNKNOWN);
		}
	} else {
		return error(DfuStatus::ERR_UNKNOWN);
	}
}

bool DFU::request_getstate(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* pbData) {
	if( (pSetup->wValue == 0) && (pSetup->wLength == 1) ) {
		pbData[0] = (uint8_t)get_state();
		*piLen = 1;
		return true;
	} else {
		return error(DfuStatus::ERR_UNKNOWN);
	}
}

bool DFU::request_abort(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* pbData) {
	if( (pSetup->wValue == 0) && (pSetup->wLength == 0) ) {
		if( get_state() != DfuState::DFU_ERROR ) {
			set_state(DfuState::DFU_IDLE);
			return true;
		} else {
			return error(DfuStatus::ERR_UNKNOWN);
		}
	} else {
		return error(DfuStatus::ERR_UNKNOWN);
	}
}
