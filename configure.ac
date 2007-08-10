AC_INIT(geoclue/position.c)

PACKAGE=geoclue

VERSION=0.6

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_SUBST(DBUS_BINDING_TOOL)


AM_CONFIG_HEADER(config.h)


AM_GCONF_SOURCE_2
AC_PATH_PROG(GCONFTOOL, gconftool-2)


              
AC_ARG_ENABLE(dev,
              [AC_HELP_STRING([--enable-dev],
                              [Only compile client libraries disable backends and applets])],
              [enable_dev=yes],
              [enable_dev=no])

AC_ARG_ENABLE(panel-applet,
              [AC_HELP_STRING([--enable-panel-applet],
                              [Compile the panel applet])],
              [enable_panel_applet=yes],
              [enable_panel_applet=no])




PKG_CHECK_MODULES(DEPS,  gconf-2.0 dbus-glib-1 gtk+-2.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


DBUS_SERVICES_DIR=`pkg-config dbus-1 --variable=exec_prefix`/share/dbus-1/services
AC_SUBST(DBUS_SERVICES_DIR)



if test "x$enable_dev" = "xno"
then




dnl Check for GPSD headers

AC_CHECK_HEADERS([gps.h], [ac_cv_gps_h=yes])
if test "x$ac_cv_gps_h" != xyes; then
    AC_MSG_ERROR([Install gpsd Debian package or its source-code equivalent])
fi
AC_CHECK_LIB(gps, gps_open,[ac_cv_libgps=yes] )
if test "x$ac_cv_libgps" != xyes; then
    AC_MSG_ERROR([Install gpsd Debian package or its source-code equivalent])
fi

dnl check for libgpsbt and libgpsmgr
AC_CHECK_LIB(gpsmgr, gpsmgr_is_gpsd_running)
AC_CHECK_LIB(gpsbt, gpsbt_start)

dnl check for libconic
PKG_CHECK_MODULES(CONIC, conic, [have_libconic=yes], [have_libconic=no])
AC_SUBST(CONIC_CFLAGS)
AC_SUBST(CONIC_LIBS)
if test "x$have_libconic" = "xyes"; then
    AC_DEFINE(HAVE_LIBCONIC, 1, [define if libconic is installed])
fi


PKG_CHECK_MODULES(HTTPXML, libsoup-2.2 libxml-2.0)
AC_SUBST(HTTPXML_CFLAGS)
AC_SUBST(HTTPXML_LIBS)

APPLET_CFLAGS=
APPLET_LIBS=


if test "x$enable_panel_applet" = "xyes"
then
PKG_CHECK_MODULES(APPLET, libpanelapplet-2.0)
EXTRA_SUBDIRS="applet"
AC_OUTPUT(	
	applet/Makefile
)
fi

AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)

PKG_CHECK_MODULES(YAHOO, gconf-2.0 libsoup-2.2 libxml-2.0 dbus-glib-1 gtk+-2.0)
AC_SUBST(YAHOO_CFLAGS)
AC_SUBST(YAHOO_LIBS)



dnl substitute subdirs into makefiles
EXTRA_SUBDIRS="${EXTRA_SUBDIRS} backend"



AC_OUTPUT(	
	backend/Makefile
	backend/map_yahoo/Makefile
	backend/geocode_yahoo/Makefile
	backend/position_manual/Makefile
	backend/position_hostip/Makefile
	backend/position_plazes/Makefile
	backend/position_gpsd/Makefile
)

fi

dnl substitute subdirs into makefiles
AC_SUBST(EXTRA_SUBDIRS)


AC_OUTPUT(Makefile 
	geoclue/Makefile 
	example/Makefile
	data/Makefile
    doxygen/Makefile
    doxygen/doxygen.conf
	geoclue.pc
)

