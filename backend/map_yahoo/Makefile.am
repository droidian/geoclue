bin_PROGRAMS = geoclue-map-server-yahoo

geoclue_map_server_yahoo_SOURCES = geoclue_map_server_yahoo.c \
				geoclue_map_signal_marshal.c	

service_in_files = org.freedesktop.geoclue.map.yahoo.service.in
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
		geoclue_map_server_yahoo.h \
		map_yahoo.desc \
		$(service_in_files)


BUILT_SOURCES = \
	geoclue_map_signal_marshal.c	\
	geoclue_map_signal_marshal.h	\
	geoclue_map_server_glue.h \
		$(service_files)

CLEANFILES = \
	geoclue_map_signal_marshal.c	\
	geoclue_map_signal_marshal.h	\
	geoclue_map_server_glue.h \
		$(service_files)

backendregistrationdir = $(datadir)/geoclue/backend
backendregistration_DATA = \
	map_yahoo.desc



geoclue_map_server_glue.h:../../geoclue/map_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_map --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 


geoclue_map_signal_marshal.h:../../geoclue/map_signal_marshal.list
	glib-genmarshal --prefix _geoclue_map --header  $< > $@ 
geoclue_map_signal_marshal.c:../../geoclue/map_signal_marshal.list
	glib-genmarshal --prefix _geoclue_map --body  $< > $@ 

