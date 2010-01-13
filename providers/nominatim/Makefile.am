libexec_PROGRAMS = \
	geoclue-nominatim

NOINST_H_FILES = 		\
	geoclue-nominatim.h

geoclue_nominatim_SOURCES = 	\
	$(NOINST_H_FILES)	\
	geoclue-nominatim.c

geoclue_nominatim_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS)

geoclue_nominatim_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-nominatim.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Nominatim.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
