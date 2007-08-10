bin_PROGRAMS = geoclue-position-server-gpsd

geoclue_position_server_gpsd_SOURCES = geoclue_position_server_gpsd.c \
				geoclue_position_signal_marshal.c \
				../geoclue_position_error.c


service_in_files = org.freedesktop.geoclue.position.gpsd.service.in
service_files = $(service_in_files:.service.in=.service)


# D-Bus service descriptor installation
servicedir = $(DBUS_SERVICES_DIR)
service_DATA = $(service_files)
execdir = @bindir@
$(service_DATA): $(service_in_files) Makefile
	for f in $(service_in_files) ; do \
		sed -e 's;@execdir@;$(execdir);' $< > $@ ;\
	done


AM_CPPFLAGS = $(YAHOO_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(YAHOO_LIBS) ../../geoclue/libgeoclue.la -lgps

EXTRA_DIST = \
		geoclue_position_server_gpsd.h \
		position_gpsd.desc \
		$(service_in_files)

BUILT_SOURCES = \
	geoclue_position_signal_marshal.c	\
	geoclue_position_signal_marshal.h	\
	geoclue_position_server_glue.h \
		$(service_files)

CLEANFILES = \
	geoclue_position_signal_marshal.c	\
	geoclue_position_signal_marshal.h	\
	geoclue_position_server_glue.h \
		$(service_files)

backendregistrationdir = $(datadir)/geoclue/backend
backendregistration_DATA = \
	position_gpsd.desc


geoclue_position_server_glue.h:../../geoclue/position_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_position --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 


geoclue_position_signal_marshal.h:../../geoclue/position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_position --header  $< > $@ 
geoclue_position_signal_marshal.c:../../geoclue/position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_position --body  $< > $@ 

