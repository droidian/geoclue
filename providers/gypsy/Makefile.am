libexec_PROGRAMS = geoclue-gypsy

geoclue_gypsy_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)	\
	$(GYPSY_CFLAGS)

geoclue_gypsy_LDADD =		\
	$(GEOCLUE_LIBS)		\
	$(GYPSY_LIBS)		\
	$(top_builddir)/geoclue/libgeoclue.la

geoclue_gypsy_SOURCES =		\
	geoclue-gypsy.c

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-gypsy.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Gypsy.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
