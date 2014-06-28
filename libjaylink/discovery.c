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
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <libusb.h>

#include "libjaylink.h"
#include "libjaylink-internal.h"

/* USB Vendor ID (VID) of SEGGER products. */
#define USB_VENDOR_ID			0x1366

/* USB Product ID (PID) of J-Link devices with USB address 0. */
#define USB_PRODUCT_ID_BASE		0x0101

/* USB Product ID (PID) of J-Link OB devices. */
#define USB_PRODUCT_ID_OB		0x0105

/* Maximum length of the USB string descriptor for the serial number. */
#define USB_SERIAL_NUMBER_LENGTH	12

/*
 * Maximum number of digits in a serial number. Note that the serial number of
 * J-Link devices consists of at most 9 digits but user defined serial numbers
 * are allowed with up to 10 digits.
 */
#define MAX_SERIAL_NUMBER_DIGITS	10

static struct jaylink_device **allocate_device_list(size_t length)
{
	struct jaylink_device **list;

	list = malloc(sizeof(struct jaylink_device *) * (length + 1));

	if (!list)
		return NULL;

	list[length] = NULL;

	return list;
}

static int parse_serial_number(const char *str, uint32_t *serial_number)
{
	size_t length;

	length = strlen(str);

	/*
	 * Skip the first digits which are not part of a valid serial number.
	 * This is necessary because some devices erroneously use random digits
	 * instead of zeros for padding.
	 */
	if (length > MAX_SERIAL_NUMBER_DIGITS)
		str = str + (length - MAX_SERIAL_NUMBER_DIGITS);

	if (sscanf(str, "%" SCNu32, serial_number) != 1)
		return 0;

	return 1;
}

static int compare_devices(const void *a, const void *b)
{
	const struct jaylink_device *dev;
	const struct libusb_device *usb_dev;

	dev = a;
	usb_dev = b;

	if (dev->usb_dev == usb_dev)
		return 0;

	return 1;
}

static struct jaylink_device *find_device(const struct jaylink_context *ctx,
		const struct libusb_device *usb_dev)
{
	struct list *item;

	item = list_find_custom(ctx->devs, &compare_devices, usb_dev);

	if (item)
		return item->data;

	return NULL;
}

static struct jaylink_device *probe_device(struct jaylink_context *ctx,
		struct libusb_device *usb_dev)
{
	int ret;
	struct libusb_device_descriptor desc;
	struct libusb_device_handle *usb_devh;
	struct jaylink_device *dev;
	char buf[USB_SERIAL_NUMBER_LENGTH + 1];
	uint8_t usb_address;
	uint32_t serial_number;
	int onboard_device;

	if (libusb_get_device_descriptor(usb_dev, &desc) < 0)
		return NULL;

	/* Check for USB Vendor ID (VID) of SEGGER. */
	if (desc.idVendor != USB_VENDOR_ID)
		return NULL;

	/* Check for USB Product ID (PID) of J-Link devices. */
	if (desc.idProduct < USB_PRODUCT_ID_BASE)
		return NULL;

	if (desc.idProduct > USB_PRODUCT_ID_OB)
		return NULL;

	/*
	 * Search for an already allocated device instance for this J-Link
	 * device and if found return a reference to it.
	 */
	dev = find_device(ctx, usb_dev);

	if (dev)
		return jaylink_ref_device(dev);

	/*
	 * J-Link OB devices always have the USB address 0. The USB address of
	 * all other J-Link devices depends on their USB Product ID (PID).
	 */
	if (desc.idProduct == USB_PRODUCT_ID_OB) {
		onboard_device = 1;
		usb_address = 0;
	} else {
		onboard_device = 0;
		usb_address = desc.idProduct - USB_PRODUCT_ID_BASE;
	}

	/* Open the device to be able to retrieve its serial number. */
	if (libusb_open(usb_dev, &usb_devh) < 0)
		return NULL;

	ret = libusb_get_string_descriptor_ascii(usb_devh, desc.iSerialNumber,
		(unsigned char *)buf, USB_SERIAL_NUMBER_LENGTH + 1);

	libusb_close(usb_devh);

	if (ret < 0)
		return NULL;

	if (!parse_serial_number(buf, &serial_number))
		return NULL;

	dev = device_allocate(ctx);

	if (!dev)
		return NULL;

	dev->usb_dev = libusb_ref_device(usb_dev);
	dev->onboard_device = onboard_device;
	dev->usb_address = usb_address;
	dev->serial_number = serial_number;

	return dev;
}

ssize_t discovery_get_device_list(struct jaylink_context *ctx,
		struct jaylink_device ***list)
{
	ssize_t ret;
	struct libusb_device **usb_devs;
	struct jaylink_device **devs;
	struct jaylink_device *dev;
	size_t num_usb_devs;
	size_t num_devs;
	size_t i;

	ret = libusb_get_device_list(ctx->usb_ctx, &usb_devs);

	if (ret < 0)
		return JAYLINK_ERR;

	num_usb_devs = ret;

	/*
	 * Allocate a device list with the length of the number of all found
	 * USB devices because they all are possible J-Link devices.
	 */
	devs = allocate_device_list(num_usb_devs);

	if (!devs) {
		libusb_free_device_list(usb_devs, 1);
		return JAYLINK_ERR_MALLOC;
	}

	num_devs = 0;

	for (i = 0; i < num_usb_devs; i++) {
		dev = probe_device(ctx, usb_devs[i]);

		if (dev) {
			devs[num_devs] = dev;
			num_devs++;
		}
	}

	devs[num_devs] = NULL;

	libusb_free_device_list(usb_devs, 1);
	*list = devs;

	return num_devs;
}
