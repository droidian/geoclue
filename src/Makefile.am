include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

SUBDIRS = public-api agent geocode-glib

interfacedir = $(datadir)/dbus-1/interfaces
interface_DATA = org.freedesktop.GeoClue2.xml

dbus_built_sources = geoclue-interface.c geoclue-interface.h
geoclue-interface.c: geoclue-interface.h
geoclue-interface.h: Makefile.am $(interface_DATA)
	$(AM_V_GEN)$(GDBUS_CODEGEN) \
		--interface-prefix org.freedesktop.GeoClue2.	\
		--c-namespace GClue				\
		--generate-c-code geoclue-interface		\
    		--generate-docbook=docs				\
		$(srcdir)/$(interface_DATA)

BUILT_SOURCES = 		\
	$(dbus_built_sources)	\
	$(NULL)

noinst_LTLIBRARIES = libgeoclue.la

AM_CPPFLAGS = $(GEOCLUE_CFLAGS) 		     	  \
	      $(ModemManager_CFLAGS)			  \
	      $(NetworkManager_CFLAGS)			  \
	      $(WARN_CFLAGS)			     	  \
	      -DLOCALEDIR="\"$(datadir)/locale\"" 	  \
	      -DG_LOG_DOMAIN=\""Geoclue"\"	     	  \
	      -DABS_TOP_SRCDIR=\""$(abs_top_srcdir)"\" 	  \
	      -DSYSCONFDIR=\""$(sysconfdir)"\"		  \
	      -I$(srcdir)/agent				  \
	      -I$(builddir)/agent			  \
	      -I$(srcdir)/public-api			  \
	      -I$(builddir)/public-api

libgeoclue_la_SOURCES =	 	 \
	$(BUILT_SOURCES) 	 \
	gclue-client-info.h 	 \
	gclue-client-info.c 	 \
	gclue-config.h 	 	 \
	gclue-config.c 	 	 \
	gclue-error.h 	 	 \
	gclue-error.c 	 	 \
	gclue-location-source.h	 \
	gclue-location-source.c	 \
	gclue-locator.h 	 \
	gclue-locator.c 	 \
	gclue-service-manager.h  \
	gclue-service-manager.c  \
	gclue-service-client.h   \
	gclue-service-client.c   \
	gclue-service-location.h \
	gclue-service-location.c \
	gclue-web-source.c	 \
	gclue-web-source.h	 \
	gclue-wifi.h 		 \
	gclue-wifi.c		 \
	$(NULL)

if BUILD_MODEM_SOURCE
libgeoclue_la_SOURCES += gclue-modem-source.c gclue-modem-source.h
endif

if BUILD_3G_SOURCE
libgeoclue_la_SOURCES += gclue-3g.c gclue-3g.h
endif

if BUILD_MODEM_GPS_SOURCE
libgeoclue_la_SOURCES += gclue-modem-gps.c gclue-modem-gps.h
endif

libgeoclue_la_LIBADD = \
	$(GEOCLUE_LIBS)			\
	$(NetworkManager_LIBS)		\
	$(ModemManager_LIBS)		\
	$(LIBS)				\
	$(NULL)

geoclue_SOURCES =	 	  \
	gclue-main.c	 	  \
	$(NULL)
geoclue_LDADD = $(GEOCLUE_LIBS) 	  		    	\
		$(LIBS) 		  		    	\
		$(builddir)/libgeoclue.la		    	\
		$(builddir)/agent/libgeoclue-agent.la 		\
		$(builddir)/geocode-glib/libgeocode-glib.la 	\
		$(builddir)/public-api/libgeoclue-public-api.la

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(interface_DATA) \
	     test-data/fedora-geoip-results.json \
	     test-data/freegeoip-results.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-$(GEOCLUE_API_VERSION).pc

DISTCLEANFILES = $(pkgconfig_DATA)

-include $(top_srcdir)/git.mk
