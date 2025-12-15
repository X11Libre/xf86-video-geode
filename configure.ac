#  Copyright 2005 Adam Jackson.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Process this file with autoconf to produce a configure script

# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([xlibre-xf86-video-geode],
        [2.18.1],
        [https://github.com/X11Libre/xf86-video-geode/issues],
        [xlibre-xf86-video-geode])

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz])
AM_MAINTAINER_MODE

# Initialize libtool
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Require xorg-macros: XORG_DEFAULT_OPTIONS
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.4 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.4)
XORG_DEFAULT_OPTIONS

# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AH_TOP([#include "xorg-server.h"])

# Define a configure option for an alternate input module directory
PKG_PROG_PKG_CONFIG([0.25])
AC_ARG_WITH(xorg-module-dir,
            AS_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory]),
            [moduledir="$withval"],
            [moduledir=`$PKG_CONFIG --variable=moduledir xorg-server`])
AC_SUBST([moduledir])

# Define a configure option to enable/disable lcd panel support
AC_ARG_ENABLE(geodegx-panel,
          AS_HELP_STRING([--disable-geodegx-panel],
                         [Disable support for flatpanels with the Geode GX]),
          [ ],
          [ PANEL_CPPFLAGS=-DPNL_SUP ])
AC_SUBST(PANEL_CPPFLAGS)

# Define a configure option to disable GX altogether
AC_ARG_ENABLE(geodegx,
          AS_HELP_STRING([--disable-geodegx],
                         [Disable support for the Geode GX and only build LX]),
          [geodegx=no],
          [geodegx=yes])

if test "x$geodegx" != "xno" ; then
    BUILD_GX=yes
    DEVICE_CPPFLAGS=-DHAVE_GX
fi
AM_CONDITIONAL(BUILD_GX, [test "x$BUILD_GX" = xyes])
AC_SUBST(DEVICE_CPPFLAGS)

# Define a configure option to enable/disable ztv
AC_ARG_ENABLE(ztv,
    AS_HELP_STRING([--enable-ztv],
        [Enable Video For Linux based ZTV driver (default: auto-detected)]),
        [ztv=$enableval],
        [ztv=auto])

# Check for Video4Linux Version 2 (V4L2) availability
AC_CHECK_HEADERS([linux/videodev2.h],[v4l2=yes],[v4l2=no])
if test "x$ztv" != "xno" ; then
    if test "x$v4l2" = "xno" ; then
        if test "x$ztv" = "xyes" ; then
            # User really wants ztv but V4L2 is not found
            AC_MSG_ERROR([ZTV driver requested, but videodev2.h not found.])
        fi
    else
        BUILD_ZTV=yes
    fi
fi
AM_CONDITIONAL(BUILD_ZTV, [test "x$BUILD_ZTV" = xyes])

# Check if GCC supports compiling in 32 bit mode for 64 bit computers
case $host_cpu in
    x86_64*|amd64*)
    if  test "x$GCC" = xyes ; then
        SAVE_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS -m32"
        AC_MSG_CHECKING([if $CC supports the -m32 Intel/AMD option])
        AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
int
main ()
{
  ;
  return 0;
}
                        ]])],
                        [m32_support=yes; M32_CFLAGS=-m32],
                        [m32_support=no])
        AC_MSG_RESULT([$m32_support])
        CFLAGS="$SAVE_CFLAGS"
    fi
    ;;
esac
AC_SUBST([M32_CFLAGS])

# Store the list of server defined optional extensions in REQUIRED_MODULES
XORG_DRIVER_CHECK_EXT(RANDR, randrproto)
XORG_DRIVER_CHECK_EXT(RENDER, renderproto)
XORG_DRIVER_CHECK_EXT(XV, videoproto)
XORG_DRIVER_CHECK_EXT(DPMSExtension, xextproto)

# Obtain compiler/linker options for the Geode driver dependencies
PKG_CHECK_MODULES(XORG, [xorg-server >= 1.18 xproto fontsproto $REQUIRED_MODULES])
PKG_CHECK_MODULES(XEXT, [xextproto >= 7.0.99.1],
                  HAVE_XEXTPROTO_71="yes"; AC_DEFINE(HAVE_XEXTPROTO_71, 1, [xextproto 7.1 available]),
                  HAVE_XEXTPROTO_71="no")

# Checks for libpciaccess support.
SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $XORG_CFLAGS"
AC_CHECK_DECL(XSERVER_LIBPCIACCESS,
              [XSERVER_LIBPCIACCESS=yes],[XSERVER_LIBPCIACCESS=no],
              [#include "xorg-server.h"])
CPPFLAGS="$SAVE_CPPFLAGS"

if test "x$XSERVER_LIBPCIACCESS" = xyes; then
    PKG_CHECK_MODULES([PCIACCESS], [pciaccess >= 0.8.0])
    XORG_CFLAGS="$XORG_CFLAGS $PCIACCESS_CFLAGS"
fi

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
