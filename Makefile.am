ALWAYS_DIRS = interfaces geoclue src providers example docs

SUBDIRS = $(ALWAYS_DIRS) @BUILD_TESTS@
DIST_SUBDIRS = $(ALWAYS_DIRS) test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue.pc

DISTCLEANFILES = \
        geoclue.pc

EXTRA_DIST = \
        geoclue.pc.in

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
