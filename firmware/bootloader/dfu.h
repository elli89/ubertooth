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

#pragma once

#include <stdint.h>

#include <usbstruct.h>

#include "flash.h"
#include "dfu_interface.h"

/* Descriptor Types */
#define DESC_DFU_FUNCTIONAL         0x21

/* Hack to clean up the namespace pollution from lpc17.h */
#undef Status

class DFU {
public:
    static const uint32_t detach_timeout_ms = 15000;
    static const uint32_t transfer_size = 0x100;

    enum Attribute {
        WILL_DETACH            = (1 << 3),
        MANIFESTATION_TOLERANT = (1 << 2),
        CAN_UPLOAD             = (1 << 1),
        CAN_DNLOAD             = (1 << 0),
    };

    DFU(Flash& flash);

    bool request_handler(TSetupPacket *pSetup, uint32_t *piLen, uint8_t **ppbData);

    bool in_dfu_mode() const;

    bool dfu_virgin() const;

private:

    Flash& flash;
    DfuStatus status;
    DfuState state;
    bool virginity;

    void set_state(const DfuState new_state);
    DfuState get_state() const;

    void set_status(const DfuStatus new_status);
    DfuStatus get_status() const;

    bool error(const DfuStatus new_status);

    uint8_t get_status_string_id() const;
    uint32_t get_poll_timeout() const;

    bool request_detach(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_dnload(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_upload(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_getstatus(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_clrstatus(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_getstate(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_abort(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
};
