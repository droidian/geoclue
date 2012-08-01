libexec_PROGRAMS = geoclue-localnet

noinst_DATA = \
	geoclue-localnet.xml

nodist_geoclue_localnet_SOURCES = \
	geoclue-localnet-glue.h

BUILT_SOURCES =			\
	$(nodist_geoclue_localnet_SOURCES)

geoclue_localnet_SOURCES = 	\
	geoclue-localnet.c     

geoclue_localnet_CFLAGS = \
	-I$(top_srcdir)     \
	-I$(top_builddir)   \
	-I$(top_srcdir)/src \
	$(GEOCLUE_CFLAGS)

geoclue_localnet_LDADD = \
	$(GEOCLUE_LIBS)    \
	$(top_builddir)/src/libconnectivity.la \
	$(top_builddir)/geoclue/libgeoclue.la


providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-localnet.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Localnet.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

CLEANFILES = \
	stamp-geoclue-localnet-glue.h

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)	\
	$(noinst_DATA)

DISTCLEANFILES = \
	$(service_DATA)		\
	$(nodist_geoclue_localnet_SOURCES)

%-glue.h: stamp-%-glue.h
	@true

stamp-geoclue-localnet-glue.h: geoclue-localnet.xml
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --prefix=geoclue_localnet --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
