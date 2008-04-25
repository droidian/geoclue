libexec_PROGRAMS = \
	geoclue-geonames

NOINST_H_FILES = 		\
	geoclue-geonames.h

geoclue_geonames_SOURCES = 	\
	$(NOINST_H_FILES)	\
	geoclue-geonames.c

geoclue_geonames_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS)

geoclue_geonames_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-geonames.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Geonames.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
