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

#include "libjaylink.h"
#include "libjaylink-internal.h"

/* USB interface number of J-Link devices. */
#define USB_INTERFACE_NUMBER		0

/* USB interface number of J-Link OB devices. */
#define USB_INTERFACE_NUMBER_OB		2

int transport_open(struct jaylink_device_handle *devh)
{
	int ret;
	struct jaylink_device *dev;
	struct jaylink_context *ctx;
	struct libusb_device_handle *usb_devh;
	int interface_number;

	dev = devh->dev;
	ctx = dev->ctx;

	if (dev->onboard_device)
		interface_number = USB_INTERFACE_NUMBER_OB;
	else
		interface_number = USB_INTERFACE_NUMBER;

	log_dbg(ctx, "Trying to open device (bus:address = %03u:%03u).",
		libusb_get_bus_number(dev->usb_dev),
		libusb_get_device_address(dev->usb_dev));

	ret = libusb_open(dev->usb_dev, &usb_devh);

	if (ret < 0) {
		log_err(ctx, "Failed to open device: %s.",
			libusb_error_name(ret));
		return JAYLINK_ERR;
	}

	ret = libusb_claim_interface(usb_devh, interface_number);

	if (ret < 0) {
		log_err(ctx, "Failed to claim interface: %s.",
			libusb_error_name(ret));
		libusb_close(usb_devh);
		return JAYLINK_ERR;
	}

	log_dbg(ctx, "Device opened successfully.");

	devh->usb_devh = usb_devh;

	return JAYLINK_OK;
}

void transport_close(struct jaylink_device_handle *devh)
{
	int ret;
	struct jaylink_device *dev;
	struct jaylink_context *ctx;
	int interface_number;

	dev = devh->dev;
	ctx = dev->ctx;

	if (dev->onboard_device)
		interface_number = USB_INTERFACE_NUMBER_OB;
	else
		interface_number = USB_INTERFACE_NUMBER;

	log_dbg(ctx, "Closing device (bus:address = %03u:%03u).",
		libusb_get_bus_number(dev->usb_dev),
		libusb_get_device_address(dev->usb_dev));

	ret = libusb_release_interface(devh->usb_devh, interface_number);

	if (ret < 0)
		log_warn(ctx, "Failed to release interface: %s.",
			libusb_error_name(ret));

	libusb_close(devh->usb_devh);
}
