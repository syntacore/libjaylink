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

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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

struct jaylink_device_handle {
	struct jaylink_device *dev;

	struct libusb_device_handle *usb_devh;

	/* USB interface number of the device. */
	uint8_t interface_number;

	/* USB interface IN endpoint of the device. */
	uint8_t endpoint_in;

	/* USB interface OUT endpoint of the device. */
	uint8_t endpoint_out;

	/*
	 * Buffer for write and read operations.
	 *
	 * Note that write and read operations are always processed
	 * consecutively and therefore the same buffer can be used for both.
	 */
	uint8_t *buffer;

	/*
	 * Number of bytes left to be received from the device for the read
	 * operation.
	 */
	uint16_t read_length;

	/* Number of bytes available in the buffer to be read. */
	uint16_t bytes_available;

	/* Current read position in the buffer. */
	uint16_t read_pos;

	/*
	 * Number of bytes left to be written before the write operation will
	 * be performed.
	 */
	uint16_t write_length;

	/*
	 * Current write position in the buffer. This is equivalent to the
	 * number of bytes in the buffer and used for write operations only.
	 */
	uint16_t write_pos;
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

/*--- transport.c -----------------------------------------------------------*/

int transport_open(struct jaylink_device_handle *devh);
int transport_close(struct jaylink_device_handle *devh);
int transport_start_write_read(struct jaylink_device_handle *devh,
		uint16_t write_length, uint16_t read_length, int has_command);
int transport_start_write(struct jaylink_device_handle *devh, uint16_t length,
		int has_command);
int transport_start_read(struct jaylink_device_handle *devh, uint16_t length);
int transport_write(struct jaylink_device_handle *devh, const uint8_t *buffer,
		uint16_t length);
int transport_read(struct jaylink_device_handle *devh, uint8_t *buffer,
		uint16_t length);

#endif /* LIBJAYLINK_LIBJAYLINK_INTERNAL_H */
