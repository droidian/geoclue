libexec_PROGRAMS = geoclue-example

geoclue_example_CFLAGS = 	\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)

geoclue_example_LDADD =		\
	$(GEOCLUE_LIBS)		\
	$(top_builddir)/geoclue/libgeoclue.la

geoclue_example_SOURCES =	\
	geoclue-example.c

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-example.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Example.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	$(service_in_files)	\
	$(providers_DATA)

DISTCLEANFILES = \
	$(service_DATA)
