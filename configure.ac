AC_INIT(geoclue/position.c)

PACKAGE=geoclue

VERSION=0.2

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_SUBST(DBUS_BINDING_TOOL)


AM_CONFIG_HEADER(config.h)


PKG_CHECK_MODULES(YAHOO, libsoup-2.2 libxml-2.0 dbus-glib-1 gtk+-2.0)
AC_SUBST(YAHOO_CFLAGS)
AC_SUBST(YAHOO_LIBS)




PKG_CHECK_MODULES(DEPS,  dbus-glib-1 gtk+-2.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


DBUS_SERVICES_DIR=`pkg-config dbus-1 --variable=exec_prefix`/share/dbus-1/services
AC_SUBST(DBUS_SERVICES_DIR)


AC_OUTPUT(Makefile 
	geoclue/Makefile 
	example/Makefile
	geoclue.pc
	backend/Makefile
	backend/geoclue_master/Makefile
	backend/map_yahoo/Makefile
	backend/position_manual/Makefile
	backend/geocode_yahoo/Makefile
	backend/position_hostip/Makefile
)
