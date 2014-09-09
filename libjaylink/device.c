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

#include <stdlib.h>
#include <libusb.h>

#include "libjaylink.h"
#include "libjaylink-internal.h"

#define CMD_GET_VERSION 0x01

struct jaylink_device *device_allocate(struct jaylink_context *ctx)
{
	struct jaylink_device *dev;
	struct list *list;

	dev = malloc(sizeof(struct jaylink_device));

	if (!dev)
		return NULL;

	list = list_prepend(ctx->devs, dev);

	if (!list) {
		free(dev);
		return NULL;
	}

	ctx->devs = list;

	dev->ctx = ctx;
	dev->refcnt = 1;
	dev->usb_dev = NULL;

	return dev;
}

static struct jaylink_device_handle *allocate_device_handle(
		struct jaylink_device *dev)
{
	struct jaylink_device_handle *devh;

	devh = malloc(sizeof(struct jaylink_device_handle));

	if (!devh)
		return NULL;

	devh->dev = jaylink_ref_device(dev);

	return devh;
}

static void free_device_handle(struct jaylink_device_handle *devh)
{
	jaylink_unref_device(devh->dev);
	free(devh);
}

ssize_t jaylink_get_device_list(struct jaylink_context *ctx,
		struct jaylink_device ***list)
{
	if (!ctx || !list)
		return JAYLINK_ERR_ARG;

	return discovery_get_device_list(ctx, list);
}

void jaylink_free_device_list(struct jaylink_device **list, int unref_devices)
{
	size_t i;

	if (!list)
		return;

	if (unref_devices) {
		i = 0;

		while (list[i]) {
			jaylink_unref_device(list[i]);
			i++;
		}
	}

	free(list);
}

int jaylink_device_get_serial_number(const struct jaylink_device *dev,
		uint32_t *serial_number)
{
	if (!dev || !serial_number)
		return JAYLINK_ERR_ARG;

	*serial_number = dev->serial_number;

	return JAYLINK_OK;
}

int jaylink_device_get_usb_address(const struct jaylink_device *dev)
{
	if (!dev)
		return JAYLINK_ERR_ARG;

	return dev->usb_address;
}

struct jaylink_device *jaylink_ref_device(struct jaylink_device *dev)
{
	if (!dev)
		return NULL;

	dev->refcnt++;

	return dev;
}

void jaylink_unref_device(struct jaylink_device *dev)
{
	if (!dev)
		return;

	dev->refcnt--;

	if (dev->refcnt == 0) {
		dev->ctx->devs = list_remove(dev->ctx->devs, dev);

		if (dev->usb_dev)
			libusb_unref_device(dev->usb_dev);

		free(dev);
	}
}

int jaylink_open(struct jaylink_device *dev,
		struct jaylink_device_handle **devh)
{
	int ret;
	struct jaylink_device_handle *handle;

	if (!dev || !devh)
		return JAYLINK_ERR_ARG;

	handle = allocate_device_handle(dev);

	if (!handle) {
		log_err(dev->ctx, "Device handle malloc failed.");
		return JAYLINK_ERR_MALLOC;
	}

	ret = transport_open(handle);

	if (ret < 0) {
		free_device_handle(handle);
		return ret;
	}

	*devh = handle;

	return JAYLINK_OK;
}

void jaylink_close(struct jaylink_device_handle *devh)
{
	if (!devh)
		return;

	transport_close(devh);
	free_device_handle(devh);
}

/**
 * Retrieve the firmware version of a device.
 *
 * @param[in,out] devh Device handle.
 * @param[out] version Newly allocated string which contains the firmware
 * 		       version, and undefined if the device returns no
 * 		       firmware version or on failure. The string is
 * 		       null-terminated and must be free'd by the caller.
 *
 * @return The length of the newly allocated firmware version string including
 *	   trailing null-terminator or 0 if the device returns no firmware
 * 	   version or any #jaylink_error error code on failure.
 */
int jaylink_get_firmware_version(struct jaylink_device_handle *devh,
		char **version)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[2];
	uint16_t length;
	char *tmp;

	if (!devh || !version)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 1, 2, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_GET_VERSION;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 2);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	length = buffer_get_u16(buf, 0);

	if (!length)
		return 0;

	ret = transport_start_read(devh, length);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_read() failed: %i.", ret);
		return ret;
	}

	tmp = malloc(length);

	if (!tmp) {
		log_err(ctx, "Firmware version string malloc failed.");
		return JAYLINK_ERR_MALLOC;
	}

	ret = transport_read(devh, (uint8_t *)tmp, length);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		free(tmp);
		return ret;
	}

	/* Last byte is reserved for null-terminator. */
	tmp[length - 1] = 0;
	*version = tmp;

	return length;
}
