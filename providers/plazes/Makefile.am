libexec_PROGRAMS =	\
	geoclue-plazes

geoclue_plazes_SOURCES = \
	geoclue-plazes.c

geoclue_plazes_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src \
	$(GEOCLUE_CFLAGS)

geoclue_plazes_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/src/libconnectivity.la \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-plazes.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Plazes.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
