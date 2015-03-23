##
## This file is part of the libjaylink project.
##
## Copyright (C) 2014-2015 Marc Schink <jaylink-dev@marcschink.de>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

AC_PREREQ([2.69])

AC_INIT([libjaylink], [0.1.0], [BUG-REPORT-ADDRESS])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Enable additional compiler warnings via -Wall and -Wextra. Use hidden
# visibility for all non-static symbols by default with -fvisibility=hidden.
CFLAGS="$CFLAGS -Wall -Wextra -Werror -fvisibility=hidden"

# Checks for programs.
AC_PROG_CC

# Automake >= 1.12 requires AM_PROG_AR when using options -Wall and -Werror.
# To be compatible with older versions of Automake use AM_PROG_AR if it's
# defined only. This line must occur before LT_INIT.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Initialize libtool.
LT_INIT

# Initialize pkg-config.
PKG_PROG_PKG_CONFIG

# Checks for libraries.

# Check for libusb-1.0 which is always needed.
PKG_CHECK_MODULES([libusb], [libusb-1.0 >= 1.0.9],
	[CFLAGS="$CFLAGS $libusb_CFLAGS"; LIBS="$LIBS $libusb_LIBS"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.

# Disable progress and informational output of libtool.
AC_SUBST(AM_LIBTOOLFLAGS, '--silent')

JAYLINK_LIB_LDFLAGS=""

case $host_os in
mingw*)
	JAYLINK_LIB_LDFLAGS+="-no-undefined"
esac

AC_SUBST([JAYLINK_LIB_LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([libjaylink/Makefile])
AC_CONFIG_FILES([libjaylink.pc])

AC_OUTPUT
