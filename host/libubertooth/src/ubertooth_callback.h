/*
 * Copyright 2015 Hannes Ellinger
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

#ifndef __UBERTOOTH_CALLBACK_H__
#define __UBERTOOTH_CALLBACK_H__


#include "ubertooth_control.h"
#include "ubertooth.h"

typedef void (*rx_callback)(Ubertooth* ut, void* args);

void cb_afh_initial(Ubertooth* ut, void* args);
void cb_afh_monitor(Ubertooth* ut, void* args);
void cb_afh_r(Ubertooth* ut, void* args);
void cb_btle(Ubertooth* ut, void* args);
void cb_ego(Ubertooth* ut, void* args __attribute__((unused)));
void cb_rx(Ubertooth* ut, void* args);
void cb_scan(Ubertooth* ut, void* args);

#endif /* __UBERTOOTH_CALLBACK_H__ */
