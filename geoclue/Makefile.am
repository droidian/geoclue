lib_LTLIBRARIES = libgeoclue.la

nodist_libgeoclue_la_SOURCES = \
	geoclue-marshal.c	\
	geoclue-marshal.h	\
	gc-iface-address-bindings.h	\
	gc-iface-address-glue.h	\
	gc-iface-geoclue-bindings.h	\
	gc-iface-geoclue-glue.h \
	gc-iface-geocode-bindings.h	\
	gc-iface-geocode-glue.h	\
	gc-iface-master-bindings.h	\
	gc-iface-master-client-bindings.h	\
	gc-iface-position-bindings.h	\
	gc-iface-position-glue.h	\
	gc-iface-reverse-geocode-bindings.h	\
	gc-iface-reverse-geocode-glue.h	\
	gc-iface-velocity-bindings.h	\
	gc-iface-velocity-glue.h

BUILT_SOURCES = \
	$(nodist_libgeoclue_la_SOURCES)	\
	geoclue-enum-types.h		\
	geoclue-enum-types.c

libgeoclue_la_SOURCES = 	\
	geoclue-accuracy.c	\
	geoclue-address.c	\
	geoclue-address-details.c	\
	geoclue-provider.c	\
	geoclue-error.c		\
	geoclue-geocode.c	\
	geoclue-master.c	\
	geoclue-master-client.c	\
	geoclue-position.c	\
	geoclue-reverse-geocode.c	\
	geoclue-types.c		\
	geoclue-velocity.c	\
	gc-provider.c		\
	gc-web-service.c	\
	gc-iface-address.c	\
	gc-iface-geoclue.c      \
	gc-iface-geocode.c	\
	gc-iface-position.c	\
	gc-iface-reverse-geocode.c	\
	gc-iface-velocity.c	\
	geoclue-enum-types.c

libgeoclue_la_LIBADD =	\
	$(GEOCLUE_LIBS)

libgeoclue_la_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)
	
geoclue_headers =		\
	gc-iface-address.h	\
	gc-iface-geoclue.h	\
	gc-iface-geocode.h	\
	gc-iface-position.h	\
	gc-iface-reverse-geocode.h	\
	gc-iface-velocity.h	\
	gc-provider.h		\
	gc-web-service.h	\
	geoclue-accuracy.h	\
	geoclue-address.h	\
	geoclue-address-details.h	\
	geoclue-provider.h	\
	geoclue-error.h		\
	geoclue-geocode.h	\
	geoclue-master.h	\
	geoclue-master-client.h	\
	geoclue-position.h	\
	geoclue-reverse-geocode.h	\
	geoclue-types.h		\
	geoclue-velocity.h	\
	geoclue-enum-types.h

libgeoclue_includedir = $(includedir)/geoclue
libgeoclue_include_HEADERS =	\
	$(geoclue_headers)

EXTRA_DIST =			\
	geoclue-marshal.list

CLEANFILES = $(BUILT_SOURCES) 	\
	stamp-gc-iface-address-glue.h	\
	stamp-gc-iface-geoclue-glue.h	\
	stamp-gc-iface-geocode-glue.h	\
	stamp-gc-iface-position-glue.h	\
	stamp-gc-iface-reverse-geocode-glue.h	\
	stamp-gc-iface-velocity-glue.h

DISTCLEANFILES = \
	$(nodist_libgeoclue_la_SOURCES)	\
	geoclue-enum-types.h		\
	geoclue-enum-types.c

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

geoclue_headers_to_scan_for_enums = geoclue-error.h
# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
geoclue_built_headers = geoclue-enum-types.h
geoclue_built_cfiles = geoclue-enum-types.c

geoclue-enum-types.h: $(geoclue_headers_to_scan_for_enums) Makefile
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __GEOCLUE_ENUM_TYPES_H__\n#define __GEOCLUE_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define GEOCLUE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __GEOCLUE_ENUM_TYPES_H__ */" \
		$(geoclue_headers_to_scan_for_enums)) > $@

geoclue-enum-types.c: $(geoclue_headers_to_scan_for_enums) Makefile geoclue-enum-types.h
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#include <geoclue-error.h>\n" \
			--fhead "#include \"geoclue-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(geoclue_headers_to_scan_for_enums)) > $@
