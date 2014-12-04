/*
 * This file is part of the libjaylink project.
 *
 * Copyright (C) 2015 Marc Schink <jaylink-dev@marcschink.de>
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

#include <stdint.h>

#include "libjaylink.h"
#include "libjaylink-internal.h"

/**
 * @file
 *
 * Emulator communication (EMUCOM).
 */

/** @cond PRIVATE */
#define CMD_EMUCOM			0xee

#define EMUCOM_CMD_WRITE		0x01

/**
 * Error code indicating that the EMUCOM channel is not supported by the
 * device.
 */
#define EMUCOM_ERR_NOT_SUPPORTED	0x80000001
/** @endcond */

/**
 * Write to an EMUCOM channel.
 *
 * @note This function must only be used if the device has the
 * 	 #JAYLINK_DEV_CAP_EMUCOM capability.
 *
 * @param[in,out] devh Device handle.
 * @param[in] channel Channel to write data to.
 * @param[in] buffer Buffer to write data from.
 * @param[in,out] length Number of bytes to write. On success, the value gets
 * 			 updated with the actual number of bytes written. The
 * 			 value is undefined on failure.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR_PROTO Protocol violation.
 * @retval JAYLINK_ERR_DEV Unspecified device error.
 * @retval JAYLINK_ERR_DEV_NOT_SUPPORTED Channel is not supported by the device.
 * @retval JAYLINK_ERR Other error conditions.
 */
JAYLINK_API int jaylink_emucom_write(struct jaylink_device_handle *devh,
		uint32_t channel, const uint8_t *buffer, uint32_t *length)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[10];
	uint32_t tmp;
	int32_t dummy;

	if (!devh || !buffer || !length)
		return JAYLINK_ERR_ARG;

	if (!*length)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write(devh, 10, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_EMUCOM;
	buf[1] = EMUCOM_CMD_WRITE;

	buffer_set_u32(buf, channel, 2);
	buffer_set_u32(buf, *length, 6);

	ret = transport_write(devh, buf, 10);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_start_write_read(devh, *length, 4, 0);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_read() failed: %i.", ret);
		return ret;
	}

	ret = transport_write(devh, buffer, *length);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_read() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 4);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	tmp = buffer_get_u32(buf, 0);

	if (tmp == EMUCOM_ERR_NOT_SUPPORTED) {
		log_err(ctx, "Channel 0x%x is not supported by the device.",
			channel);
		return JAYLINK_ERR_DEV_NOT_SUPPORTED;
	}

	dummy = tmp;

	if (dummy < 0) {
		log_err(ctx, "Failed to write to channel 0x%x.", channel);
		return JAYLINK_ERR_DEV;
	}

	if (tmp > *length) {
		log_err(ctx, "Only %u bytes were supposed to be written, but "
			"the device reported %u written bytes.", *length, tmp);
		return JAYLINK_ERR_PROTO;
	}

	*length = tmp;

	return JAYLINK_OK;
}
