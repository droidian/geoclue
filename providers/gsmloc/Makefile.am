libexec_PROGRAMS =	\
	geoclue-gsmloc

geoclue_gsmloc_SOURCES = \
	geoclue-gsmloc.c

geoclue_gsmloc_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS) \
	$(GSMLOC_CFLAGS)

geoclue_gsmloc_LDADD = \
	$(GEOCLUE_LIBS) \
	$(GSMLOC_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-gsmloc.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Gsmloc.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
