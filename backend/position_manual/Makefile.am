bin_PROGRAMS = geoclue-position-server-yahoo

geoclue_position_server_yahoo_SOURCES = geoclue_position_server_yahoo.c \
				geoclue_position_signal_marshal.c


AM_CPPFLAGS = $(YAHOO_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(YAHOO_LIBS) ../../geoclue/libgeoclue.la


BUILT_SOURCES = \
	geoclue_position_signal_marshal.c	\
	geoclue_position_signal_marshal.h	\
	geoclue_position_server_glue.h 

CLEANFILES = \
	geoclue_position_signal_marshal.c	\
	geoclue_position_signal_marshal.h	\
	geoclue_position_server_glue.h 


geoclue_position_server_glue.h:../../geoclue/position_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_position --mode=glib-server  $< |  sed 's;\data = user_data;\data = (DBusGAsyncData\*)user_data;g' > $@ 


geoclue_position_signal_marshal.h:../../geoclue/position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_position --header  $< > $@ 
geoclue_position_signal_marshal.c:../../geoclue/position_signal_marshal.list
	glib-genmarshal --prefix _geoclue_position --body  $< > $@ 

