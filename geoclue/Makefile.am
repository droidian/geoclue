lib_LTLIBRARIES = libgeoclue.la

EXTRA_DIST = \
		geoclue_glue.xml \
		geoclue_signal_marshal.list


libgeoclue_la_SOURCES = 	geoclue.c \
						geoclue_signal_marshal.c

libgeoclue_la_CFLAGS = $(DEPS_CFLAGS) -I$(top_srcdir)
libgeoclue_la_LIBADD = $(DEPS_LIBS)

libgeoclue_ladir=$(includedir)/geoclue/geoclue
libgeoclue_la_HEADERS = 	geoclue.h 



BUILT_SOURCES = \
				geoclue_client_glue.h \
				geoclue_signal_marshal.h \
				geoclue_signal_marshal.c

CLEANFILES = 	\
				geoclue_client_glue.h \
				geoclue_signal_marshal.h \
				geoclue_signal_marshal.c


dbusspecdir = $(datadir)/geoclue/dbus
dbusspec_DATA = \ 
		geoclue_glue.xml \
		geoclue_signal_marshal.list



geoclue_client_glue.h:geoclue_glue.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue --mode=glib-client $< > $@

geoclue_signal_marshal.h:geoclue_signal_marshal.list
	glib-genmarshal --prefix _geoclue --header $< > $@ 
geoclue_signal_marshal.c:geoclue_signal_marshal.list
	glib-genmarshal --prefix _geoclue --body $< > $@ 
