AC_PREREQ(2.59)
AC_INIT(geoclue, 0.1, http://geoclue.freedesktop.org)

AC_CONFIG_SRCDIR(geoclue/gc-iface-geoclue.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([1.9 foreign])
GEOCLUE_VERSION=0.1

AC_PROG_CC
AC_ISC_POSIX

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL
AM_PROG_LIBTOOL

PKG_CHECK_MODULES(GEOCLUE, [
		  glib-2.0
		  dbus-glib-1 >= 0.60
])
AC_SUBST(GEOCLUE_LIBS)
AC_SUBST(GEOCLUE_CFLAGS)

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

CFLAGS="$CFLAGS -g -Wall -Werror -Wno-format"

AC_CONFIG_FILES([
Makefile
interfaces/Makefile
geoclue/Makefile
])

AC_OUTPUT
