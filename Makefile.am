SUBDIRS = interfaces geoclue providers src example test docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue.pc

DISTCLEANFILES = \
        geoclue.pc

EXTRA_DIST = \
        geoclue.pc.in

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
