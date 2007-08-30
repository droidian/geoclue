bin_PROGRAMS = geoclue-geocode-server-yahoo

geoclue_geocode_server_yahoo_SOURCES = geoclue_geocode_server_yahoo.c

service_in_files = org.freedesktop.geoclue.geocode.yahoo.service.in
service_files = $(service_in_files:.service.in=.service)


# D-Bus service descriptor installation
servicedir = $(DBUS_SERVICES_DIR)
service_DATA = $(service_files)
execdir = @bindir@
$(service_DATA): $(service_in_files) Makefile
	for f in $(service_in_files) ; do \
		sed -e 's;@execdir@;$(execdir);' $< > $@ ;\
	done



LDADD = ../common/libgeoclue_web_service.a
AM_CPPFLAGS = $(YAHOO_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(YAHOO_LIBS)

EXTRA_DIST = \
		geoclue_geocode_server_yahoo.h \
		geocode_yahoo.desc \
		$(service_in_files)

BUILT_SOURCES = \
	geoclue_geocode_server_glue.h \
		$(service_files)

CLEANFILES = \
	geoclue_geocode_server_glue.h \
		$(service_files)


backendregistrationdir = $(datadir)/geoclue/backend
backendregistration_DATA = \
	geocode_yahoo.desc



geoclue_geocode_server_glue.h:../../geoclue/geocode_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_geocode --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 

