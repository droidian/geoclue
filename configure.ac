AC_PREREQ(2.59)
AC_INIT(geoclue, 0.10, http://geoclue.freedesktop.org)

AC_CONFIG_SRCDIR(geoclue/gc-iface-geoclue.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([1.9 foreign])
GEOCLUE_VERSION=0.10

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

PKG_CHECK_MODULES(MASTER, [
		  gconf-2.0
])
AC_SUBST(MASTER_LIBS)
AC_SUBST(MASTER_CFLAGS)

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

DBUS_SERVICES_DIR="${datadir}/dbus-1/services"
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for D-Bus is])

CFLAGS="$CFLAGS -g -Wall -Werror -Wno-format"



PKG_CHECK_MODULES(GTK, [
		  gtk+-2.0
], HAVE_GTK=yes, HAVE_GTK=no)
AM_CONDITIONAL(HAVE_GTK, test "x$HAVE_GTK" = "xyes")
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

CONNECTIVITY="None"

PKG_CHECK_MODULES(CONIC, [
		  conic
], HAVE_CONIC=yes, HAVE_CONIC=no)
if test "x$HAVE_CONIC" = "xyes"; then
	CONNECTIVITY="Maemo LibConIC"
	CONNECTIVITY_LIBS=${CONIC_LIBS}
	CONNECTIVITY_CFLAGS=${CONIC_CFLAGS}
	AC_DEFINE(HAVE_CONIC, 1, [define if libconic is installed])
fi

PKG_CHECK_MODULES(NETWORK_MANAGER, [
		  NetworkManager
], HAVE_NETWORK_MANAGER=yes, HAVE_NETWORK_MANAGER=no)
if test "x$HAVE_NETWORK_MANAGER" = "xyes"; then
	CONNECTIVITY="Network Manager"
	CONNECTIVITY_LIBS=${NETWORK_MANAGER_LIBS}
	CONNECTIVITY_CFLAGS=${NETWORK_MANAGER_CFLAGS}
	AC_DEFINE(HAVE_NETWORK_MANAGER, 1, [define if Network Manager is installed])
fi

AC_SUBST(CONNECTIVITY_LIBS)
AC_SUBST(CONNECTIVITY_CFLAGS)


PROVIDER_SUBDIRS="example hostip geonames manual plazes localnet yahoo"

PKG_CHECK_MODULES(GSMLOC, [
		  gammu >= 1.12
], HAVE_GSMLOC=yes, HAVE_GSMLOC=no)
if test "x$HAVE_GSMLOC" = "xyes"; then
	PROVIDER_SUBDIRS="$PROVIDER_SUBDIRS gsmloc"
else
	NO_BUILD_PROVIDERS="$NO_BUILD_PROVIDERS gsmloc"
fi
AC_SUBST(GSMLOC_LIBS)
AC_SUBST(GSMLOC_CFLAGS)


PKG_CHECK_MODULES(GYPSY, [
		  gypsy
], HAVE_GYPSY=yes,HAVE_GYPSY=no)
if test "x$HAVE_GYPSY" = "xyes"; then
	PROVIDER_SUBDIRS="$PROVIDER_SUBDIRS gypsy"
else
	NO_BUILD_PROVIDERS="$NO_BUILD_PROVIDERS gypsy"
fi
AC_SUBST(GYPSY_LIBS)
AC_SUBST(GYPSY_CFLAGS)

AC_CHECK_LIB(gps, gps_open,[libgps=yes] )
if test "x$libgps" = xyes; then
	PROVIDER_SUBDIRS="$PROVIDER_SUBDIRS gpsd"
    GPSD_LIBS="-lgps"
else
	NO_BUILD_PROVIDERS="$NO_BUILD_PROVIDERS gpsd"
fi
AC_SUBST(GPSD_LIBS)
AC_SUBST(GPSD_CFLAGS)

AC_SUBST(PROVIDER_SUBDIRS)

AC_CONFIG_FILES([
geoclue.pc
Makefile
docs/Makefile
docs/reference/Makefile
docs/tools/Makefile
interfaces/Makefile
geoclue/Makefile
example/Makefile
test/Makefile
providers/Makefile
providers/example/Makefile
providers/gypsy/Makefile
providers/gpsd/Makefile
providers/hostip/Makefile
providers/geonames/Makefile
providers/manual/Makefile
providers/plazes/Makefile
providers/localnet/Makefile
providers/yahoo/Makefile
providers/gsmloc/Makefile
src/Makefile
])

AC_OUTPUT

echo ""
echo "Geoclue ${VERSION} has been configured as follows: "
echo "---------------------------------------------------"
echo "Source code location:   ${srcdir}"
echo "Compiler:               ${CC}"
echo "Network connectivity:   ${CONNECTIVITY}"
echo "Providers:              ${PROVIDER_SUBDIRS}"
echo "Excluded providers:    ${NO_BUILD_PROVIDERS}"
echo ""
