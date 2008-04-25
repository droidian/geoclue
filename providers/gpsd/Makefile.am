libexec_PROGRAMS = geoclue-gpsd

geoclue_gpsd_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)	\
    $(GPSD_CFLAGS)

geoclue_gpsd_LDADD =		\
	$(GEOCLUE_LIBS)		\
	$(GPSD_LIBS)		\
	$(top_builddir)/geoclue/libgeoclue.la

geoclue_gpsd_SOURCES =		\
	geoclue-gpsd.c

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-gpsd.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Gpsd.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
