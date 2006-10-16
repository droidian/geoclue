lib_LTLIBRARIES = libgeoclue.la
bin_PROGRAMS = geoclue-master

geoclue_master_SOURCES = geoclue_master.c \
						geoclue_master_signal_marshal.c	


service_in_files = org.foinse_project.geoclue.master.service.in
service_files = $(service_in_files:.service.in=.service)


# D-Bus service descriptor installation
servicedir = $(DBUS_SERVICES_DIR)
service_DATA = $(service_files)
execdir = @bindir@
$(service_DATA): $(service_in_files) Makefile
	for f in $(service_in_files) ; do \
		sed -e 's;@execdir@;$(execdir);' $< > $@ ;\
	done



AM_CPPFLAGS = $(DEPS_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(DEPS_LIBS)



EXTRA_DIST = \
		position_glue.xml \
		position_signal_marshal.list \
		map_glue.xml \
		map_signal_marshal.list \
		master_glue.xml \
		geocode_glue.xml \
		$(service_in_files)
		
		

libgeoclue_la_SOURCES = 	position.c \
  				position_signal_marshal.c \
				map.c \
				map_gtk.c \
				map_gtk_layout.c \
				map_signal_marshal.c \
				geocode.c 
 
libgeoclue_la_CFLAGS = $(DEPS_CFLAGS) -I$(top_srcdir)
libgeoclue_la_LIBADD = $(DEPS_LIBS)

libgeoclue_ladir=$(includedir)/geoclue/geoclue
libgeoclue_la_HEADERS = 	position.h \ 
				map.h \
				map_gtk.h \
				map_gtk_layout.h \
				geocode.h



BUILT_SOURCES = \
				position_client_glue.h \
				position_signal_marshal.h \
				position_signal_marshal.c \
				map_client_glue.h \
				map_signal_marshal.h \
				map_signal_marshal.c \
				geoclue_master_signal_marshal.c	\
				geoclue_master_server_glue.h \
				geoclue_master_client_glue.h \
				geocode_client_glue.h \
				$(service_files)

CLEANFILES =	\
				position_client_glue.h \
				position_signal_marshal.h \
				position_signal_marshal.c \
				map_client_glue.h \
				map_signal_marshal.h \
				map_signal_marshal.c \
				geoclue_master_signal_marshal.c	\
				geoclue_master_server_glue.h \
				geoclue_master_client_glue.h \
				geocode_client_glue.h \
				$(service_files)


dbusspecdir = $(datadir)/geoclue/dbus
dbusspec_DATA = \ 
		position_glue.xml \
		position_signal_marshal.list \
		map_glue.xml \
		map_signal_marshal.list \
		geocode_glue.xml 





position_client_glue.h:position_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_position --mode=glib-client $< > $@

position_signal_marshal.h:position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_position --header $< > $@ 
position_signal_marshal.c:position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_position --body $< > $@ 


map_client_glue.h:map_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_map --mode=glib-client $< > $@

map_signal_marshal.h:map_signal_marshal.list
	glib-genmarshal --prefix _geoclue_map --header $< > $@ 
map_signal_marshal.c:map_signal_marshal.list
	glib-genmarshal --prefix _geoclue_map --body $< > $@ 
	
	
	
	
geoclue_master_server_glue.h:master_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_master --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 
	
geoclue_master_client_glue.h:master_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_client --mode=glib-client  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 



geoclue_master_signal_marshal.h:master_signal_marshal.list
	glib-genmarshal --prefix _geoclue_master --header  $< > $@ 
geoclue_master_signal_marshal.c:master_signal_marshal.list
	glib-genmarshal --prefix _geoclue_master --body  $< > $@ 
	
geocode_client_glue.h:geocode_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_geocode --mode=glib-client $< > $@




