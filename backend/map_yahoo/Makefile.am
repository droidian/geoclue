bin_PROGRAMS = geoclue-map-server-yahoo

geoclue_map_server_yahoo_SOURCES = geoclue_map_server_yahoo.c \
				geomapserver_signal_marshal.c	



AM_CPPFLAGS = $(YAHOO_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(YAHOO_LIBS)


BUILT_SOURCES = \
	geomapserver_signal_marshal.c	\
	geomapserver_signal_marshal.h	\
	geomapserver_server_glue.h 

CLEANFILES = \
	geomapserver_signal_marshal.c	\
	geomapserver_signal_marshal.h	\
	geomapserver_server_glue.h 

geomapserver_server_glue.h:../../geoclue/map_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geomapserver --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 


geomapserver_signal_marshal.h:../../geoclue/map_signal_marshal.list
	glib-genmarshal --prefix _geomapserver --header  $< > $@ 
geomapserver_signal_marshal.c:../../geoclue/map_signal_marshal.list
	glib-genmarshal --prefix _geomapserver --body  $< > $@ 

