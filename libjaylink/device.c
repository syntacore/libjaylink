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

#define CMD_GET_VERSION		0x01
#define CMD_SET_SPEED		0x05
#define CMD_GET_HW_STATUS	0x07
#define CMD_GET_FREE_MEMORY	0xd4
#define CMD_GET_CAPS		0xe8
#define CMD_GET_EXT_CAPS	0xed

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

/**
 * Retrieve the hardware status of a device.
 *
 * @param[in,out] devh Device handle.
 * @param[out] status Hardware status on success, and undefined on failure.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 */
int jaylink_get_hardware_status(struct jaylink_device_handle *devh,
		struct jaylink_hardware_status *status)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[8];

	if (!devh || !status)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 1, 8, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_GET_HW_STATUS;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 8);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	status->target_voltage = buffer_get_u16(buf, 0);
	status->tck = buf[2];
	status->tdi = buf[3];
	status->tdo = buf[4];
	status->tms = buf[5];
	status->tres = buf[6];
	status->trst = buf[7];

	return JAYLINK_OK;
}

/**
 * Retrieve the capabilities of a device.
 *
 * The capabilities are stored in a 32-bit bit array consisting of
 * #JAYLINK_DEV_CAPS_SIZE bytes where each individual bit represents a
 * capability. The first bit of this array is the least significant bit of the
 * first byte and the following bits are sequentially numbered in order of
 * increasing bit significance and byte index. A set bit indicates a supported
 * capability. See #jaylink_device_capability for a description of the
 * capabilities and their bit positions.
 *
 * @param[in,out] devh Device handle.
 * @param[out] caps Buffer to store capabilities on success. Its value is
 * 		    undefined on failure. The size of the buffer must be large
 * 		    enough to contain at least #JAYLINK_DEV_CAPS_SIZE bytes.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 *
 * @see jaylink_get_extended_caps() to retrieve extended device capabilities.
 */
int jaylink_get_caps(struct jaylink_device_handle *devh, uint8_t *caps)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[1];

	if (!devh || !caps)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 1, JAYLINK_DEV_CAPS_SIZE, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_GET_CAPS;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, caps, JAYLINK_DEV_CAPS_SIZE);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	return JAYLINK_OK;
}

/**
 * Retrieve the extended capabilities of a device.
 *
 * The extended capabilities are stored in a 256-bit bit array consisting of
 * #JAYLINK_DEV_EXT_CAPS_SIZE bytes. See jaylink_get_caps() for a further
 * description of how the capabilities are represented in this bit array. For a
 * description of the capabilities and their bit positions, see
 * #jaylink_device_capability.
 *
 * @note This function must only be used if the device has the
 *	 #JAYLINK_DEV_CAP_GET_EXT_CAPS capability.
 *
 * @param[in,out] devh Device handle.
 * @param[out] caps Buffer to store capabilities on success. Its value is
 * 		    undefined on failure. The size of the buffer must be large
 * 		    enough to contain at least #JAYLINK_DEV_EXT_CAPS_SIZE bytes.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 *
 * @see jaylink_get_caps() to retrieve device capabilities.
 */
int jaylink_get_extended_caps(struct jaylink_device_handle *devh, uint8_t *caps)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[1];

	if (!devh || !caps)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 1, JAYLINK_DEV_EXT_CAPS_SIZE, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_GET_EXT_CAPS;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, caps, JAYLINK_DEV_EXT_CAPS_SIZE);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	return JAYLINK_OK;
}

/**
 * Retrieve the size of free memory of a device.
 *
 * @note This function must only be used if the device has the
 * 	 #JAYLINK_DEV_CAP_GET_FREE_MEMORY capability.
 *
 * @param[in,out] devh Device handle.
 * @param[out] size Size of free memory in bytes on success, and undefined on
 * 		    failure.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 *
 * @see jaylink_get_caps() to retrieve device capabilities.
 */
int jaylink_get_free_memory(struct jaylink_device_handle *devh, uint32_t *size)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[4];

	if (!devh || !size)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 1, 4, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_GET_FREE_MEMORY;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 4);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	*size = buffer_get_u32(buf, 0);

	return JAYLINK_OK;
}

/**
 * Set the target interface speed of a device.
 *
 * @param[in,out] devh Device handle.
 * @param[in] speed Speed in kHz or #JAYLINK_SPEED_ADAPTIVE_CLOCKING for
 * 		    adaptive clocking. Speed of 0 kHz is not allowed and
 * 		    adaptive clocking must only be used if the device has the
 * 		    #JAYLINK_DEV_CAP_ADAPTIVE_CLOCKING capability.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 */
int jaylink_set_speed(struct jaylink_device_handle *devh, uint16_t speed)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[3];

	if (!devh || !speed)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write(devh, 3, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_SET_SPEED;
	buffer_set_u16(buf, speed, 1);

	ret = transport_write(devh, buf, 3);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	return JAYLINK_OK;
}
