bin_PROGRAMS = geoclue-position-server-gypsy

geoclue_position_server_gypsy_SOURCES = \
	geoclue_position_server_gypsy.c \
	geoclue_position_signal_marshal.c

service_in_files = org.freedesktop.geoclue.position.gypsy.service.in
service_files = $(service_in_files:.service.in=.service)

# D-Bus service descriptor installation
servicedir = $(DBUS_SERVICES_DIR)
service_DATA = $(service_files)
execdir = @bindir@
$(service_DATA): $(service_in_files) Makefile
	for f in $(service_in_files) ; do \
		sed -e 's;@execdir@;$(execdir);' $< > $@ ;\
	done

LDADD = ../common/libgeoclue_position_error.a
AM_CPPFLAGS = $(GYPSY_CFLAGS)
AM_LDFLAGS = $(GYPSY_LIBS) \
             ../../geoclue/libgeoclue.la

EXTRA_DIST = \
	geoclue_position_server_gypsy.h \
	position_gypsy.desc \
	$(service_in_files)

BUILT_SOURCES = \
	geoclue_position_signal_marshal.c \
	geoclue_position_signal_marshal.h \
	geoclue_position_server_glue.h \
	$(service_files)

CLEANFILES = \
	geoclue_position_signal_marshal.c \
	geoclue_position_signal_marshal.h \
	geoclue_position_server_glue.h \
	$(service_files)

backendregistrationdir = $(datadir)/geoclue/backend
backendregistration_DATA = position_gypsy.desc

geoclue_position_server_glue.h:../../geoclue/position_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_posgypsy --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 

geoclue_position_signal_marshal.h:../../geoclue/position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_posgypsy --header  $< > $@ 
geoclue_position_signal_marshal.c:../../geoclue/position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_posgypsy --body  $< > $@ 
