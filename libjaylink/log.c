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

#include <stdio.h>
#include <stdarg.h>

#include "libjaylink.h"
#include "libjaylink-internal.h"

int jaylink_log_set_level(struct jaylink_context *ctx, int level)
{
	if (!ctx)
		return JAYLINK_ERR_ARG;

	if (level < JAYLINK_LOG_LEVEL_NONE || level > JAYLINK_LOG_LEVEL_DEBUG)
		return JAYLINK_ERR_ARG;

	ctx->log_level = level;

	return JAYLINK_OK;
}

int jaylink_log_get_level(const struct jaylink_context *ctx)
{
	if (!ctx)
		return JAYLINK_ERR_ARG;

	return ctx->log_level;
}

static void log_vprintf(struct jaylink_context *ctx, int level,
		const char *format, va_list args)
{
	/*
	 * Filter out messages with higher verbosity than the verbosity of the
	 * current log level.
	 */
	if (level > ctx->log_level)
		return;

	fprintf(stderr, "jaylink: ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
}

void log_err(struct jaylink_context *ctx, const char *format, ...)
{
	va_list args;

	if (!ctx)
		return;

	va_start(args, format);
	log_vprintf(ctx, JAYLINK_LOG_LEVEL_ERROR, format, args);
	va_end(args);
}

void log_warn(struct jaylink_context *ctx, const char *format, ...)
{
	va_list args;

	if (!ctx)
		return;

	va_start(args, format);
	log_vprintf(ctx, JAYLINK_LOG_LEVEL_WARNING, format, args);
	va_end(args);
}

void log_info(struct jaylink_context *ctx, const char *format, ...)
{
	va_list args;

	if (!ctx)
		return;

	va_start(args, format);
	log_vprintf(ctx, JAYLINK_LOG_LEVEL_INFO, format, args);
	va_end(args);
}

void log_dbg(struct jaylink_context *ctx, const char *format, ...)
{
	va_list args;

	if (!ctx)
		return;

	va_start(args, format);
	log_vprintf(ctx, JAYLINK_LOG_LEVEL_DEBUG, format, args);
	va_end(args);
}
