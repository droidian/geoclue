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

GTK_DOC_CHECK(1.0)
AC_CHECK_PROGS(XSLT, xsltproc)

AC_ARG_ENABLE(system-bus,
	      [AC_HELP_STRING([--enable-system-bus],
			      [Use the system bus instead of session bus])],
	      enable_system_bus="$enableval",
	      enable_system_bus=no)

AM_CONDITIONAL(USE_SYSTEM_BUS, test x$enable_system_bus = xyes)
if test x$enable_system_bus = xyes; then
	AC_DEFINE(GEOCLUE_DBUS_BUS, DBUS_BUS_SYSTEM, Use the system bus)
else
	AC_DEFINE(GEOCLUE_DBUS_BUS, DBUS_BUS_SESSION, Use the session bus)
fi

PKG_CHECK_MODULES(GEOCLUE, [
		  glib-2.0
		  gobject-2.0
		  dbus-glib-1 >= 0.60
		  libxml-2.0
])
AC_SUBST(GEOCLUE_LIBS)
AC_SUBST(GEOCLUE_CFLAGS)

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

CFLAGS="$CFLAGS -g -Wall -Werror -Wno-format"

PROVIDER_SUBDIRS="example hostip geonames"

PKG_CHECK_MODULES(GYPSY, [
		  gypsy
], HAVE_GYPSY=yes,HAVE_GYPSY=no)
if test "x$HAVE_GYPSY" = "xyes"; then
	PROVIDER_SUBDIRS="$PROVIDER_SUBDIRS gypsy"
fi
AC_SUBST(GYPSY_LIBS)
AC_SUBST(GYPSY_CFLAGS)

AC_SUBST(PROVIDER_SUBDIRS)

AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
docs/tools/Makefile
interfaces/Makefile
geoclue/Makefile
example/Makefile
providers/Makefile
providers/example/Makefile
providers/gypsy/Makefile
providers/hostip/Makefile
providers/geonames/Makefile
])

AC_OUTPUT

echo ""
echo "Geoclue ${VERSION} has been configured as follows: "
echo "---------------------------------------------------"
echo "Source code location:   ${srcdir}"
echo "Compiler:               ${CC}"
echo "Providers:              ${PROVIDER_SUBDIRS}"
echo ""
