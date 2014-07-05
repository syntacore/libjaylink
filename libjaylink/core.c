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

int jaylink_init(struct jaylink_context **ctx)
{
	struct jaylink_context *context;

	if (!ctx)
		return JAYLINK_ERR_ARG;

	context = malloc(sizeof(struct jaylink_context));

	if (!context)
		return JAYLINK_ERR_MALLOC;

	if (libusb_init(&context->usb_ctx) < 0) {
		free(context);
		return JAYLINK_ERR;
	}

	context->devs = NULL;

	/* Show error and warning messages by default. */
	context->log_level = JAYLINK_LOG_LEVEL_WARNING;

	*ctx = context;

	return JAYLINK_OK;
}

void jaylink_exit(struct jaylink_context *ctx)
{
	if (!ctx)
		return;

	list_free(ctx->devs);
	libusb_exit(ctx->usb_ctx);
	free(ctx);
}
