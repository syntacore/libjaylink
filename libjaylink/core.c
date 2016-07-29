/*
 * This file is part of the libjaylink project.
 *
 * Copyright (C) 2014-2016 Marc Schink <jaylink-dev@marcschink.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <libusb.h>

#include "libjaylink.h"
#include "libjaylink-internal.h"

/**
 * @file
 *
 * Core library functions.
 */

/**
 * Initialize libjaylink.
 *
 * This function must be called before any other libjaylink function is called.
 *
 * @param[out] ctx Newly allocated libjaylink context on success, and undefined
 *                 on failure.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 * @retval JAYLINK_ERR_MALLOC Memory allocation error.
 * @retval JAYLINK_ERR Other error conditions.
 *
 * @since 0.1.0
 */
JAYLINK_API int jaylink_init(struct jaylink_context **ctx)
{
	int ret;
	struct jaylink_context *context;
#ifdef _WIN32
	WSADATA wsa_data;
#endif

	if (!ctx)
		return JAYLINK_ERR_ARG;

	context = malloc(sizeof(struct jaylink_context));

	if (!context)
		return JAYLINK_ERR_MALLOC;

	if (libusb_init(&context->usb_ctx) != LIBUSB_SUCCESS) {
		free(context);
		return JAYLINK_ERR;
	}

#ifdef _WIN32
	ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);

	if (ret != 0) {
		libusb_exit(context->usb_ctx);
		free(context);
		return JAYLINK_ERR;
	}

	if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
		libusb_exit(context->usb_ctx);
		free(context);
		return JAYLINK_ERR;
	}
#endif

	context->devs = NULL;
	context->discovered_devs = NULL;

	/* Show error and warning messages by default. */
	context->log_level = JAYLINK_LOG_LEVEL_WARNING;

	context->log_callback = &log_vprintf;
	context->log_callback_data = NULL;

	ret = jaylink_log_set_domain(context, JAYLINK_LOG_DOMAIN_DEFAULT);

	if (ret != JAYLINK_OK) {
#ifdef _WIN32
		WSACleanup();
#endif
		free(context);
		return ret;
	}

	*ctx = context;

	return JAYLINK_OK;
}

/**
 * Shutdown libjaylink.
 *
 * @param[in,out] ctx libjaylink context.
 *
 * @retval JAYLINK_OK Success.
 * @retval JAYLINK_ERR_ARG Invalid arguments.
 *
 * @since 0.1.0
 */
JAYLINK_API int jaylink_exit(struct jaylink_context *ctx)
{
	struct list *item;

	if (!ctx)
		return JAYLINK_ERR_ARG;

	item = ctx->discovered_devs;

	while (item) {
		jaylink_unref_device((struct jaylink_device *)item->data);
		item = item->next;
	}

	list_free(ctx->discovered_devs);
	list_free(ctx->devs);

	libusb_exit(ctx->usb_ctx);
#ifdef _WIN32
	WSACleanup();
#endif

	free(ctx);

	return JAYLINK_OK;
}
