lib_LTLIBRARIES = libgeoclue-backend.la

BUILT_SOURCES =			\
	gc-iface-geoclue-glue.h

libgeoclue_backend_la_SOURCES = \
	$(BUILT_SOURCES)	\
	gc-iface-geoclue.c
libgeoclue_backend_la_CFLAGS =	\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)
	
EXTRA_DIST =			\
	$(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES) 	\
	stamp-gc-iface-geoclue-glue.h

%-glue.h: stamp-%-glue.h
	@true
stamp-gc-iface-geoclue-glue.h: ../interfaces/gc-iface-geoclue.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_geoclue --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
