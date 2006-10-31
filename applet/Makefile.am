#FIX ME  remove hard coded paths

bin_PROGRAMS = geoclue-applet
bindir = /usr/lib/gnome-panel

geoclue_applet_SOURCES = geoclue_applet.c


AM_CPPFLAGS = $(APPLET_CFLAGS) $(DEPS_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(APPLET_LIBS) $(DEPS_LIBS) $(top_builddir)/geoclue/libgeoclue.la


EXTRA_DIST = \
geoclue.png \
GNOME_geoclue.server




pixdir = /usr/share/pixmaps
pix_DATA = geoclue.png


bonobodir = /usr/lib/bonobo/servers
bonobo_DATA = GNOME_geoclue.server

