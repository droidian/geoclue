include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

SUBDIRS = public-api agent geocode-glib

interfacedir = $(datadir)/dbus-1/interfaces
interface_DATA = org.freedesktop.GeoClue2.xml

dbus_built_sources = geoclue-interface.c 	\
		     geoclue-interface.h 	\
		     wpa_supplicant-interface.c \
		     wpa_supplicant-interface.h
geoclue-interface.c: geoclue-interface.h
geoclue-interface.h: Makefile.am $(interface_DATA)
	$(AM_V_GEN)$(GDBUS_CODEGEN) \
		--interface-prefix org.freedesktop.GeoClue2.	\
		--c-namespace GClueDBus				\
		--generate-c-code geoclue-interface		\
    		--generate-docbook=docs				\
		$(srcdir)/$(interface_DATA)

wpa_supplicant-interface.c: wpa_supplicant-interface.h
wpa_supplicant-interface.h: Makefile.am fi.w1.wpa_supplicant1.xml
	$(AM_V_GEN)$(GDBUS_CODEGEN) \
		--interface-prefix fi.w1			\
		--annotate "fi.w1.wpa_supplicant1"		\
		org.gtk.GDBus.C.Name WPA_Supplicant		\
		--annotate "fi.w1.wpa_supplicant1.Interface"	\
		org.gtk.GDBus.C.Name WPA_Interface		\
		--annotate "fi.w1.wpa_supplicant1.BSS"		\
		org.gtk.GDBus.C.Name WPA_BSS			\
		--annotate "fi.w1.wpa_supplicant1.BSS:SSID"	\
		org.gtk.GDBus.C.ForceGVariant whatever		\
		--annotate "fi.w1.wpa_supplicant1.BSS:BSSID"	\
		org.gtk.GDBus.C.ForceGVariant whatever		\
		--annotate "fi.w1.wpa_supplicant1.Interface::BSSAdded"	 \
		org.gtk.GDBus.C.Name BSS_Added				 \
		--annotate "fi.w1.wpa_supplicant1.Interface::BSSRemoved" \
		org.gtk.GDBus.C.Name BSS_Removed			 \
		--generate-c-code wpa_supplicant-interface 	\
		$(srcdir)/fi.w1.wpa_supplicant1.xml

gclue-marshal.c: gclue-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=gclue_marshal $(srcdir)/gclue-marshal.list --header --body > gclue-marshal.c

gclue-marshal.h: gclue-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=gclue_marshal $(srcdir)/gclue-marshal.list --header > gclue-marshal.h

BUILT_SOURCES = 		\
	$(dbus_built_sources)	\
	gclue-marshal.c 	\
	gclue-marshal.h		\
	$(NULL)

noinst_LTLIBRARIES = libgeoclue.la

AM_CPPFLAGS = $(GEOCLUE_CFLAGS) 		     	  \
	      $(ModemManager_CFLAGS)			  \
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
	gclue-3g-tower.h 	 \
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
	gclue-mozilla.h		 \
	gclue-mozilla.c		 \
	gclue-location.h	 \
	gclue-location.c	 \
	$(NULL)

if BUILD_MODEM_SOURCE
libgeoclue_la_SOURCES += gclue-modem.c	       \
			 gclue-modem.h	       \
			 gclue-modem-manager.c \
			 gclue-modem-manager.h
endif

if BUILD_3G_SOURCE
libgeoclue_la_SOURCES += gclue-3g.c gclue-3g.h
endif

if BUILD_CDMA_SOURCE
libgeoclue_la_SOURCES += gclue-cdma.c gclue-cdma.h
endif

if BUILD_MODEM_GPS_SOURCE
libgeoclue_la_SOURCES += gclue-modem-gps.c gclue-modem-gps.h
endif

libgeoclue_la_LIBADD = \
	$(GEOCLUE_LIBS)			\
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
EXTRA_DIST = $(interface_DATA) 	       \
	     fi.w1.wpa_supplicant1.xml \
	     gclue-marshal.list

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-$(GEOCLUE_API_VERSION).pc

DISTCLEANFILES = $(pkgconfig_DATA)

-include $(top_srcdir)/git.mk
