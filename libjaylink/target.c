/*
 * This file is part of the libjaylink project.
 *
 * Copyright (C) 2014-2015 Marc Schink <jaylink-dev@marcschink.de>
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

/** @cond PRIVATE */
#define CMD_SET_SPEED		0x05
#define CMD_SET_TARGET_POWER	0x08
#define CMD_SELECT_TIF		0xc7
#define CMD_CLEAR_RESET		0xdc
#define CMD_SET_RESET		0xdd

#define TIF_GET_SELECTED	0xfe
#define TIF_GET_AVAILABLE	0xff
/** @endcond */

/**
 * Set the target interface speed.
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
JAYLINK_API int jaylink_set_speed(struct jaylink_device_handle *devh,
		uint16_t speed)
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

/**
 * Select the target interface.
 *
 * @note This function must only be used if the device has the
 * 	 #JAYLINK_DEV_CAP_SELECT_TIF capability.
 *
 * @param[in,out] devh Device handle.
 * @param[in] interface Target interface to select. See
 * 			#jaylink_target_interface for valid values.
 *
 * @return The previously selected target interface on success, or a negative
 * 	   error code on failure.
 *
 * @see jaylink_get_caps() to retrieve device capabilities.
 */
JAYLINK_API int jaylink_select_interface(struct jaylink_device_handle *devh,
		uint8_t interface)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[4];
	uint32_t tmp;

	if (!devh)
		return JAYLINK_ERR_ARG;

	if (interface > JAYLINK_TIF_MAX)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 2, 4, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_SELECT_TIF;
	buf[1] = interface;

	ret = transport_write(devh, buf, 2);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 4);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	tmp = buffer_get_u32(buf, 0);

	if (tmp > JAYLINK_TIF_MAX) {
		log_err(ctx, "Invalid target interface: %u.", tmp);
		return JAYLINK_ERR;
	}

	return tmp;
}

/**
 * Retrieve the available target interfaces.
 *
 * The target interfaces are stored in a 32-bit bit field where each individual
 * bit represents a target interface. A set bit indicates an available target
 * interface. See #jaylink_target_interface for a description of the target
 * interfaces and their bit positions.
 *
 * @note This function must only be used if the device has the
 * 	 #JAYLINK_DEV_CAP_SELECT_TIF capability.
 *
 * @param[in,out] devh Device handle.
 * @param[out] interfaces Target interfaces on success, and undefined on
 * 			  failure.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 *
 * @see jaylink_get_caps() to retrieve device capabilities.
 * @see jaylink_select_interface() to select a target interface.
 */
JAYLINK_API int jaylink_get_available_interfaces(
		struct jaylink_device_handle *devh, uint32_t *interfaces)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[4];

	if (!devh || !interfaces)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 2, 4, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_SELECT_TIF;
	buf[1] = TIF_GET_AVAILABLE;

	ret = transport_write(devh, buf, 2);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 4);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	*interfaces = buffer_get_u32(buf, 0);

	return JAYLINK_OK;
}

/**
 * Retrieve the selected target interface.
 *
 * @note This function must only be used if the device has the
 * 	 #JAYLINK_DEV_CAP_SELECT_TIF capability.
 *
 * @param[in,out] devh Device handle.
 *
 * @return The currently selected target interface on success, or a negative
 * 	   error code on failure.
 *
 * @see jaylink_get_caps() to retrieve device capabilities.
 */
JAYLINK_API int jaylink_get_selected_interface(
		struct jaylink_device_handle *devh)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[4];
	uint32_t tmp;

	if (!devh)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write_read(devh, 2, 4, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write_read() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_SELECT_TIF;
	buf[1] = TIF_GET_SELECTED;

	ret = transport_write(devh, buf, 2);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	ret = transport_read(devh, buf, 4);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_read() failed: %i.", ret);
		return ret;
	}

	tmp = buffer_get_u32(buf, 0);

	if (tmp > JAYLINK_TIF_MAX) {
		log_err(ctx, "Invalid target interface: %u.", tmp);
		return JAYLINK_ERR;
	}

	return tmp;
}

/**
 * Clear the target reset signal.
 *
 * @param[in,out] devh Device handle.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 */
JAYLINK_API int jaylink_clear_reset(struct jaylink_device_handle *devh)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[1];

	if (!devh)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write(devh, 1, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_CLEAR_RESET;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	return JAYLINK_OK;
}

/**
 * Set the target reset signal.
 *
 * @param[in,out] devh Device handle.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_TIMEOUT A timeout occurred.
 * @retval JAYLINK_ERR Other error conditions.
 */
JAYLINK_API int jaylink_set_reset(struct jaylink_device_handle *devh)
{
	int ret;
	struct jaylink_context *ctx;
	uint8_t buf[1];

	if (!devh)
		return JAYLINK_ERR_ARG;

	ctx = devh->dev->ctx;
	ret = transport_start_write(devh, 1, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_start_write() failed: %i.", ret);
		return ret;
	}

	buf[0] = CMD_SET_RESET;

	ret = transport_write(devh, buf, 1);

	if (ret != JAYLINK_OK) {
		log_err(ctx, "transport_write() failed: %i.", ret);
		return ret;
	}

	return JAYLINK_OK;
}

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
JAYLINK_API int jaylink_set_target_power(struct jaylink_device_handle *devh,
		int enable)
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
