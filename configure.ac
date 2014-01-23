AC_PREREQ([2.63])

AC_INIT([geoclue],
        [2.1.0],
        [http://bugs.freedesktop.org/enter_bug.cgi?product=GeoClue],
        [geoclue],
        [http://www.freedesktop.org/wiki/Software/GeoClue])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 dist-xz no-dist-gzip tar-ustar foreign -Wall -Werror -Wno-portability])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AM_PROG_AR
AC_CHECK_TOOL([DESKTOP_FILE_VALIDATE], [desktop-file-validate], [/bin/true])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Pkg-config
PKG_PROG_PKG_CONFIG([0.22])

# i18 support
IT_PROG_INTLTOOL(0.40.0)

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The name of the gettext domain])

GLIB_MIN_VERSION=2.34.0
JSON_GLIB_MIN_VERSION=0.14
GEOIP_MIN_VERSION=1.5.1

AC_CHECK_LIB([m],[sin])

dnl
dnl Documentation
dnl
GTK_DOC_CHECK(1.0)

PKG_CHECK_MODULES(GEOCLUE, [
  glib-2.0 >= $GLIB_MIN_VERSION
  gio-2.0 >= $GLIB_MIN_VERSION
  gio-unix-2.0 >= $GLIB_MIN_VERSION
  json-glib-1.0 >= $JSON_GLIB_MIN_VERSION
  libsoup-2.4
])

# Geoip server
AC_ARG_ENABLE(geoip-server,
              AS_HELP_STRING([--enable-geoip-server=yes|no|auto], [Build geoip server]),
              [enable_geoip_server=$enableval],
              [enable_geoip_server=auto])
case "x$enable_geoip_server" in
  xyes)
    PKG_CHECK_MODULES(GEOIP_SERVER,
		      gio-2.0 >= $GLIB_MIN_VERSION
                      json-glib-1.0 >= $JSON_GLIB_MIN_VERSION
                      libsoup-2.4
                      geoip >= GEOIP_MIN_VERSION)
    build_geoip_server="yes"
    ;;
  xauto)
    PKG_CHECK_MODULES(GEOIP_SERVER,
		      gio-2.0 >= $GLIB_MIN_VERSION
                      json-glib-1.0 >= $JSON_GLIB_MIN_VERSION
                      libsoup-2.4
                      geoip >= GEOIP_MIN_VERSION,
                      [build_geoip_server=yes],
                      [build_geoip_server=no])
    ;;
  xno)
    build_geoip_server="xno"
    ;;
  x*)
    AC_MSG_ERROR([Unknown --enable-geoip-server argument: $enable_geoip_server])
    ;;
esac
if test "x$build_geoip_server" = "xyes"; then
  # Requires for the database update
  PKG_CHECK_MODULES(GEOIP_DBUPDATE,
		    gio-2.0 >= $GLIB_MIN_VERSION
                    libsoup-2.4)
  GEOIP_DATABASE_PATH=`$PKG_CONFIG --variable=databasedir geoip`
  AC_SUBST(GEOIP_DATABASE_PATH)
  AC_DEFINE([BUILD_GEOIP_SERVER], [1], [Build geoip server?])
else
  AC_DEFINE([BUILD_GEOIP_SERVER], [0], [Build geoip server?])
fi
AM_CONDITIONAL([BUILD_GEOIP_SERVER], [test "x$build_geoip_server" = "xyes"])

# Demo agent
AC_ARG_ENABLE(demo-agent,
              AS_HELP_STRING([--enable-demo-agent=yes|no],
                             [Build demo agent (default=no)]),
              [enable_demo_agent=$enableval],
              [enable_demo_agent=no])
if test "x$enable_demo_agent" = "xyes"; then
    PKG_CHECK_MODULES(DEMO_AGENT, [
                      glib-2.0 >= $GLIB_MIN_VERSION
                      gio-2.0 >= $GLIB_MIN_VERSION
                      gio-unix-2.0 >= $GLIB_MIN_VERSION
                      libnotify])
else
    AC_DEFINE([BUILD_DEMO_AGENT], [0], [Build demo agent?])
fi
AM_CONDITIONAL([BUILD_DEMO_AGENT], [test "x$enable_demo_agent" = "xyes"])

GNOME_COMPILE_WARNINGS([maximum])

dnl Require agent?
AC_ARG_ENABLE(require-auth,
	      AS_HELP_STRING([--enable-require-auth=yes|no],
                             [Require apps to be authorized by agent (default=no)]),
              [require_auth=$enableval],
              [require_auth=no])
if test "x$require_auth" = "xyes"; then
    AC_DEFINE([REQUIRE_AUTH], [1], [Require apps to be authorized by agent?])
else
    AC_DEFINE([REQUIRE_AUTH], [0], [Require apps to be authorized by agent?])
fi

dnl Debugging
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[enable debugging]),,
        enable_debug=no)
if test "x$enable_debug" = "xyes"; then
        CFLAGS="$CFLAGS -O0 -ggdb3"
fi

# DBus system directory
AC_ARG_WITH(dbus-sys-dir, AS_HELP_STRING([--with-dbus-sys-dir=DIR], [where D-BUS system.d directory is]))
if test -n "$with_dbus_sys_dir" ; then
    DBUS_SYS_DIR="$with_dbus_sys_dir"
else
    DBUS_SYS_DIR="${sysconfdir}/dbus-1/system.d"
fi
AC_SUBST(DBUS_SYS_DIR)

AC_ARG_WITH(dbus-service-user,
	    AS_HELP_STRING([--with-dbus-service-user=USER],
                           [The USER (existing) as which the service will run (default: root)]),
	    dbus_srv_user="$with_dbus_service_user",
	    dbus_srv_user="root")
AC_SUBST(dbus_srv_user)

# gdbus-codegen
if test x$cross_compiling != xyes; then
	GDBUS_CODEGEN=`$PKG_CONFIG --variable gdbus_codegen gio-2.0`
else
	AC_PATH_PROG([GDBUS_CODEGEN],[gdbus-codegen])
fi
AC_SUBST([GDBUS_CODEGEN])

if test "x$GDBUS_CODEGEN" = "x"; then
	ifelse([$2],,[AC_MSG_ERROR([gdbus-codegen not found.])],[$2])
else
	ifelse([$1],,[:],[$1])
fi

# systemd
AC_ARG_WITH([systemdsystemunitdir],
            AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
            [],
            [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test "x$with_systemdsystemunitdir" != "xno"; then
  AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
fi
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$systemdsystemunitdir"])

AC_PATH_PROG([REALPATH],[realpath])

AC_PATH_PROG([SETCAP],[setcap])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/geoclue-2.0.pc
  src/agent/Makefile
  src/geoip-server/Makefile
  po/Makefile.in
  data/org.freedesktop.GeoClue2.conf
  data/Makefile
  demo/Makefile
  docs/Makefile
  docs/version.xml
])

AC_OUTPUT

dnl ==========================================================================
AC_MSG_NOTICE([

        GeoClue $VERSION
        =================

        prefix:                   ${prefix}
        c compiler:               ${CC} ${CFLAGS}
        systemdsystemunitdir:     ${systemdsystemunitdir}
])
