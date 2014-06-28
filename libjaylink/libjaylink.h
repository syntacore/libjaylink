/*
 * This file is part of the libjaylink project.
 *
 * Copyright (C) 2014 Marc Schink <jaylink-dev@marcschink.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBJAYLINK_LIBJAYLINK_H
#define LIBJAYLINK_LIBJAYLINK_H

#include <stdlib.h>
#include <stdint.h>

enum jaylink_error {
	JAYLINK_OK = 0,
	JAYLINK_ERR = -1,
	JAYLINK_ERR_MALLOC = -2,
	JAYLINK_ERR_ARG = -3
};

struct jaylink_context;
struct jaylink_device;

int jaylink_init(struct jaylink_context **ctx);
void jaylink_exit(struct jaylink_context *ctx);

ssize_t jaylink_get_device_list(struct jaylink_context *ctx,
		struct jaylink_device ***list);

void jaylink_free_device_list(struct jaylink_device **list, int unref_devices);

int jaylink_device_get_serial_number(const struct jaylink_device *dev,
		uint32_t *serial_number);

int jaylink_device_get_usb_address(const struct jaylink_device *dev);

struct jaylink_device *jaylink_ref_device(struct jaylink_device *dev);
void jaylink_unref_device(struct jaylink_device *dev);

#endif /* LIBJAYLINK_LIBJAYLINK_H */
