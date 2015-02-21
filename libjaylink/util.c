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

/**
 * @file
 *
 * Utility functions.
 */

/**
 * Check for a capability.
 *
 * The capabilities are expected to be stored in a bit array consisting of one
 * or more bytes where each individual bit represents a capability. The first
 * bit of this array is the least significant bit of the first byte and the
 * following bits are sequentially numbered in order of increasing bit
 * significance and byte index. A set bit indicates a supported capability.
 *
 * @param[in] caps Buffer with capabilities.
 * @param[in] cap Bit position of the capability to check for.
 *
 * @return 1 if the capability is supported, 0 otherwise.
 */
JAYLINK_API int jaylink_has_cap(const uint8_t *caps, uint32_t cap)
{
	if (!caps)
		return 0;

	if (caps[cap / 8] & (1 << (cap % 8)))
		return 1;

	return 0;
}
