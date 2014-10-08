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

#ifndef LIBJAYLINK_LIBJAYLINK_H
#define LIBJAYLINK_LIBJAYLINK_H

#include <stdlib.h>
#include <stdint.h>

enum jaylink_error {
	JAYLINK_OK = 0,
	JAYLINK_ERR = -1,
	JAYLINK_ERR_MALLOC = -2,
	JAYLINK_ERR_ARG = -3,
	JAYLINK_ERR_TIMEOUT = -4
};

enum jaylink_log_level {
	JAYLINK_LOG_LEVEL_NONE = 0,
	JAYLINK_LOG_LEVEL_ERROR = 1,
	JAYLINK_LOG_LEVEL_WARNING = 2,
	JAYLINK_LOG_LEVEL_INFO = 3,
	JAYLINK_LOG_LEVEL_DEBUG = 4
};

/** Device capabilities. */
enum jaylink_device_capability {
	/** Device supports retrieval of the hardware version. */
	JAYLINK_DEV_CAP_GET_HW_VERSION = 1,
	/** Device supports adaptive clocking. */
	JAYLINK_DEV_CAP_ADAPTIVE_CLOCKING = 3,
	/** Device supports retrieval of free memory size. */
	JAYLINK_DEV_CAP_GET_FREE_MEMORY = 11,
	/** Device supports retrieval of extended capabilities. */
	JAYLINK_DEV_CAP_GET_EXT_CAPS = 31
};

/** Device hardware types. */
enum jaylink_hardware_type {
	/** J-Link BASE. */
	JAYLINK_HW_TYPE_BASE = 0
};

/** Device hardware version. */
struct jaylink_hardware_version {
	/**
	 * Hardware type.
	 *
	 * See #jaylink_hardware_type for a description of the hardware types.
	 */
	uint8_t type;
	/** Major version. */
	uint8_t major;
	/** Minor version. */
	uint8_t minor;
	/** Revision number. */
	uint8_t revision;
};

/** Device hardware status. */
struct jaylink_hardware_status {
	/** Target reference voltage in mV. */
	uint16_t target_voltage;
	/** TCK pin state. */
	uint8_t tck;
	/** TDI pin state. */
	uint8_t tdi;
	/** TDO pin state. */
	uint8_t tdo;
	/** TMS pin state. */
	uint8_t tms;
	/** TRES pin state. */
	uint8_t tres;
	/** TRST pin state. */
	uint8_t trst;
};

/** Target interface speed value for adaptive clocking. */
#define JAYLINK_SPEED_ADAPTIVE_CLOCKING 0xffff

/** Number of bytes required to store device capabilities. */
#define JAYLINK_DEV_CAPS_SIZE		4

/** Number of bytes required to store extended device capabilities. */
#define JAYLINK_DEV_EXT_CAPS_SIZE	32

struct jaylink_context;
struct jaylink_device;
struct jaylink_device_handle;

int jaylink_init(struct jaylink_context **ctx);
void jaylink_exit(struct jaylink_context *ctx);

const char *jaylink_strerror(int error_code);
const char *jaylink_strerror_name(int error_code);

int jaylink_log_set_level(struct jaylink_context *ctx, int level);
int jaylink_log_get_level(const struct jaylink_context *ctx);

ssize_t jaylink_get_device_list(struct jaylink_context *ctx,
		struct jaylink_device ***list);

void jaylink_free_device_list(struct jaylink_device **list, int unref_devices);

int jaylink_device_get_serial_number(const struct jaylink_device *dev,
		uint32_t *serial_number);

int jaylink_device_get_usb_address(const struct jaylink_device *dev);

struct jaylink_device *jaylink_ref_device(struct jaylink_device *dev);
void jaylink_unref_device(struct jaylink_device *dev);

int jaylink_open(struct jaylink_device *dev,
		struct jaylink_device_handle **devh);

void jaylink_close(struct jaylink_device_handle *devh);

int jaylink_get_firmware_version(struct jaylink_device_handle *devh,
		char **version);

int jaylink_get_hardware_version(struct jaylink_device_handle *devh,
		struct jaylink_hardware_version *version);

int jaylink_get_hardware_status(struct jaylink_device_handle *devh,
		struct jaylink_hardware_status *status);

int jaylink_get_caps(struct jaylink_device_handle *devh, uint8_t *caps);
int jaylink_get_extended_caps(struct jaylink_device_handle *devh,
		uint8_t *caps);

int jaylink_get_free_memory(struct jaylink_device_handle *devh, uint32_t *size);

int jaylink_set_speed(struct jaylink_device_handle *devh, uint16_t speed);

int jaylink_has_cap(const uint8_t *caps, uint32_t cap);

#endif /* LIBJAYLINK_LIBJAYLINK_H */
