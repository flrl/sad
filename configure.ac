AC_PREREQ([2.69])
AC_INIT([sad], [0.0], [whatever])

AC_CONFIG_SRCDIR([engine/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AC_PROG_CC_C99

AC_CHECK_HEADERS([stdio.h])

dnl look for SDL
AM_PATH_SDL2([2.0.4], , AC_MSG_ERROR([SDL2 not found]))

dnl look for libjansson
PKG_CHECK_MODULES([JANSSON], [jansson], , AC_MSG_ERROR([libjansson not found]))

AM_INIT_AUTOMAKE([foreign serial-tests subdir-objects -Wall -Werror -Wno-portability])
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
