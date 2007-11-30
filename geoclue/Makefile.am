lib_LTLIBRARIES = libgeoclue-provider.la

BUILT_SOURCES =			\
	geoclue-marshal.c	\
	geoclue-marshal.h	\
	gc-iface-address-glue.h	\
	gc-iface-geoclue-glue.h \
	gc-iface-geocode-glue.h	\
	gc-iface-position-bindings.h	\
	gc-iface-position-glue.h	\
	gc-iface-reverse-geocode-glue.h	\
	gc-iface-velocity-glue.h

libgeoclue_provider_la_SOURCES = \
	$(BUILT_SOURCES)	\
	geoclue-accuracy.c	\
	geoclue-error.c		\
	geoclue-position.c	\
	geoclue-provider.c	\
	gc-provider.c		\
	gc-web-service.c	\
	gc-iface-address.c	\
	gc-iface-geoclue.c      \
	gc-iface-geocode.c	\
	gc-iface-position.c	\
	gc-iface-reverse-geocode.c	\
	gc-iface-velocity.c

libgeoclue_provider_la_CFLAGS =	\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)
	
EXTRA_DIST =			\
	geoclue-marshal.list	\
	$(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES) 	\
	stamp-gc-iface-address-glue.h	\
	stamp-gc-iface-geoclue-glue.h	\
	stamp-gc-iface-geocode-glue.h	\
	stamp-gc-iface-position-glue.h	\
	stamp-gc-iface-reverse-geocode-glue.h	\
	stamp-gc-iface-velocity-glue.h

geoclue-marshal.h: geoclue-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=geoclue_marshal > $@
geoclue-marshal.c: geoclue-marshal.list geoclue-marshal.h $(GLIB_GENMARSHAL)
	echo "#include \"geoclue-marshal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) --prefix=geoclue_marshal $(srcdir)/geoclue-marshal.list --body >> $@

%-glue.h: stamp-%-glue.h
	@true

stamp-gc-iface-address-glue.h: ../interfaces/gc-iface-address.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_address --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

stamp-gc-iface-geoclue-glue.h: ../interfaces/gc-iface-geoclue.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_geoclue --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

stamp-gc-iface-geocode-glue.h: ../interfaces/gc-iface-geocode.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_geocode --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

stamp-gc-iface-position-glue.h: ../interfaces/gc-iface-position.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_position --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

stamp-gc-iface-reverse-geocode-glue.h: ../interfaces/gc-iface-reverse-geocode.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_reverse_geocode --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

stamp-gc-iface-velocity-glue.h: ../interfaces/gc-iface-velocity.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_velocity --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

%-bindings.h: stamp-%-bindings.h
	@true
stamp-%-bindings.h: ../interfaces/%.xml
	$(DBUS_BINDING_TOOL) --mode=glib-client --prefix=geoclue $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
