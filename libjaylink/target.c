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

#include <stdint.h>

#include "libjaylink.h"
#include "libjaylink-internal.h"

/**
 * @file
 *
 * Target related functions.
 */

#define CMD_SET_TARGET_POWER	0x08

/**
 * Enable or disable the target power supply.
 *
 * If enabled, the target is supplied with 5 V from pin 19 of the 20-pin
 * JTAG / SWD connector.
 *
 * @note This function must only be used if the device has the
 * 	 #JAYLINK_DEV_CAP_SET_TARGET_POWER capability.
 *
 * @param devh Device handle.
 * @param enable Determines whether to enable or disable the target power
 * 		 supply.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 *
 * @see jaylink_get_caps() to retrieve device capabilities.
 */
int jaylink_set_target_power(struct jaylink_device_handle *devh, int enable)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[2];

	if (!devh)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write(devh, 2, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_wrte() failed: %i.", ret);
		return ret;
	}

	if (enable)
		enable = 1;

	buf[0] = CMD_SET_TARGET_POWER;
	buf[1] = enable;

	ret = transport_write(devh, buf, 2);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	return JAYLINK_OK;
}
