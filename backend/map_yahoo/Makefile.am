bin_PROGRAMS = geoclue-map-server-yahoo

geoclue_map_server_yahoo_SOURCES = geoclue_map_server_yahoo.c \
				geoclue_map_signal_marshal.c	



AM_CPPFLAGS = $(YAHOO_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(YAHOO_LIBS) 


BUILT_SOURCES = \
	geoclue_map_signal_marshal.c	\
	geoclue_map_signal_marshal.h	\
	geoclue_map_server_glue.h 

CLEANFILES = \
	geoclue_map_signal_marshal.c	\
	geoclue_map_signal_marshal.h	\
	geoclue_map_server_glue.h 

geoclue_map_server_glue.h:../../geoclue/map_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_map --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 


geoclue_map_signal_marshal.h:../../geoclue/map_signal_marshal.list
	glib-genmarshal --prefix _geoclue_map --header  $< > $@ 
geoclue_map_signal_marshal.c:../../geoclue/map_signal_marshal.list
	glib-genmarshal --prefix _geoclue_map --body  $< > $@ 

