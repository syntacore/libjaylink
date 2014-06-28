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
