libexec_PROGRAMS = \
	geoclue-geonames

geoclue_geonames_SOURCES = \
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
