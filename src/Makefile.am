include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

install-exec-hook:
	$(AM_V_GEN)$(SETCAP) cap_sys_ptrace+ep $(DESTDIR)$(libexecdir)/geoclue || \
	echo "Unable to setcap cap_sys_ptrace+ep $(libexecdir)/geoclue. " \
	     "Package systems may do this after installation."

SUBDIRS = agent

if BUILD_GEOIP_SERVER
SUBDIRS += geoip-server
endif # BUILD_GEOIP_SERVER

interfacedir = $(datadir)/geoclue-2.0
interface_DATA = geoclue-interface.xml

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

geoclue_SOURCES =	 	 \
	$(BUILT_SOURCES) 	 \
	gclue-main.c	 	 \
	gclue-client-info.h 	 \
	gclue-client-info.c 	 \
	gclue-config.h 	 	 \
	gclue-config.c 	 	 \
	gclue-error.h 	 	 \
	gclue-error.c 	 	 \
	gclue-ipclient.h 	 \
	gclue-ipclient.c 	 \
	gclue-locator.h 	 \
	gclue-locator.c 	 \
	gclue-service-manager.h  \
	gclue-service-manager.c  \
	gclue-service-client.h   \
	gclue-service-client.c   \
	gclue-service-location.h \
	gclue-service-location.c \
	geocode-location.c       \
	geocode-location.h       \
	$(NULL)

geoclue_CFLAGS = $(GEOCLUE_CFLAGS) 		     	  \
		 $(WARN_CFLAGS)			     	  \
		 -DLOCALEDIR="\"$(datadir)/locale\"" 	  \
		 -DG_LOG_DOMAIN=\""Geoclue"\"	     	  \
		 -DABS_TOP_SRCDIR=\""$(abs_top_srcdir)"\" \
    		 -DSYSCONFDIR=\""$(sysconfdir)"\"
geoclue_LDADD = $(GEOCLUE_LIBS) $(LIBS)

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(interface_DATA) \
	     test-data/fedora-geoip-results.json \
	     test-data/freegeoip-results.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-2.0.pc

GEOCODEGLIBDIR=$(top_srcdir)/../geocode-glib/geocode-glib/
GEOCODEGLIBFILES=geocode-location.c geocode-location.h
update-from-geocode-glib:
	FILES="$(GEOCODEGLIBFILES)" DIR="$(GEOCODEGLIBDIR)" $(srcdir)/update-from-geocode-glib.sh && changed=true ; \
	git commit -m "location: Update from geocode-glib" $(GEOCODEGLIBFILES)

-include $(top_srcdir)/git.mk
