libexec_PROGRAMS = geoclue-localnet

BUILT_SOURCES = \
	geoclue-localnet-glue.h

geoclue_localnet_SOURCES = \
	$(BUILT_SOURCES) \
	geoclue-localnet.c     


CLEANFILES = \
	$(BUILT_SOURCES) \
	stamp-geoclue-localnet-glue.h

EXTRA_DIST =         \
	$(BUILT_SOURCES)

geoclue_localnet_CFLAGS = \
	-I$(top_srcdir)     \
	-I$(top_builddir)   \
	$(GEOCLUE_CFLAGS)

geoclue_localnet_LDADD = \
	$(GEOCLUE_LIBS)    \
	$(top_builddir)/geoclue/libgeoclue.la


providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-localnet.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Localnet.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

%-glue.h: stamp-%-glue.h
	@true

stamp-geoclue-localnet-glue.h: geoclue-localnet.xml
	$(DBUS_BINDING_TOOL) --prefix=geoclue_localnet --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
