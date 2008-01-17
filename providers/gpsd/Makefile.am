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
