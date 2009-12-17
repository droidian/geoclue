libexec_PROGRAMS =	\
	geoclue-skyhook

geoclue_skyhook_SOURCES = \
	geoclue-skyhook.c

geoclue_skyhook_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS) \
	$(SKYHOOK_CFLAGS)

geoclue_skyhook_LDADD = \
	$(GEOCLUE_LIBS) \
	$(SKYHOOK_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-skyhook.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Skyhook.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
