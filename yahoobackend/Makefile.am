bin_PROGRAMS = geoclueserver-yahoo

geoclueserver_yahoo_SOURCES = geoclueserver-yahoo.c \
	geoclueserver_signal_marshal.c	

AM_CPPFLAGS = $(YAHOO_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(YAHOO_LIBS)


BUILT_SOURCES = \
	geoclueserver_signal_marshal.c	\
	geoclueserver_signal_marshal.h	\
	geoclueserver_server_glue.h 

CLEANFILES = \
	geoclueserver_signal_marshal.c	\
	geoclueserver_signal_marshal.h	\
	geoclueserver_server_glue.h 


geoclueserver_server_glue.h:../geoclue/geoclue_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclueserver --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 


geoclueserver_signal_marshal.h:../geoclue/geoclue_signal_marshal.list
	glib-genmarshal --prefix _geoclueserver --header  $< > $@ 
geoclueserver_signal_marshal.c:../geoclue/geoclue_signal_marshal.list
	glib-genmarshal --prefix _geoclueserver --body  $< > $@ 
