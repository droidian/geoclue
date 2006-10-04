AC_INIT(geoclue/geoclue.c)

PACKAGE=geoclue

VERSION=0.1

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_SUBST(DBUS_BINDING_TOOL)


AM_CONFIG_HEADER(config.h)



AC_ARG_ENABLE(yahoo-backend, AS_HELP_STRING([--enable-yahoo-backend],[enable yahoo backend]), enable_yahoo_backend=yes, enable_yahoo_backend=no)


if test $enable_yahoo_backend = yes; then
	AC_OUTPUT(	yahoobackend/Makefile )
	PKG_CHECK_MODULES(YAHOO, libsoup-2.2 libxml-2.0 dbus-glib-1 geomap gtk+-2.0)
	AC_SUBST(YAHOO_CFLAGS)
	AC_SUBST(YAHOO_LIBS)
	YAHOOBACKEND=yahoobackend
fi

AC_SUBST(YAHOOBACKEND)



PKG_CHECK_MODULES(DEPS,  dbus-glib-1)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)



AC_OUTPUT(Makefile 
	geoclue/Makefile 
	example/Makefile
	geoclue.pc 
)
