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

#ifndef LIBJAYLINK_LIBJAYLINK_INTERNAL_H
#define LIBJAYLINK_LIBJAYLINK_INTERNAL_H

#include <stdarg.h>
#include <stdint.h>
#include <libusb.h>

struct jaylink_context {
	struct libusb_context *usb_ctx;

	/*
	 * List of allocated device instances. Used to prevent multiple device
	 * instances for the same J-Link device.
	 */
	struct list *devs;

	/* Current log level. */
	int log_level;
};

struct jaylink_device {
	struct jaylink_context *ctx;

	int refcnt;

	struct libusb_device *usb_dev;

	/* Indicates if the device is a J-Link OB device. */
	int onboard_device;

	uint8_t usb_address;

	/*
	 * Serial number of the device. This number is for enumeration purpose
	 * only and can differ from the real serial number of the device.
	 */
	uint32_t serial_number;
};

/*--- device.c --------------------------------------------------------------*/

struct jaylink_device *device_allocate(struct jaylink_context *ctx);

/*--- discovery.c -----------------------------------------------------------*/

ssize_t discovery_get_device_list(struct jaylink_context *ctx,
			 struct jaylink_device ***list);

/*--- list.c ----------------------------------------------------------------*/

struct list {
	void *data;
	struct list *next;
};

typedef int (*list_compare_callback)(const void *a, const void *b);

struct list *list_prepend(struct list *list, void *data);

struct list *list_remove(struct list *list, const void *data);

struct list *list_find_custom(struct list *list, list_compare_callback cb,
		const void *cb_data);

void list_free(struct list *list);

/*--- log.c -----------------------------------------------------------------*/

void log_err(struct jaylink_context *ctx, const char *format, ...);
void log_warn(struct jaylink_context *ctx, const char *format, ...);
void log_info(struct jaylink_context *ctx, const char *format, ...);
void log_dbg(struct jaylink_context *ctx, const char *format, ...);

#endif /* LIBJAYLINK_LIBJAYLINK_INTERNAL_H */
