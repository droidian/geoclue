libexec_PROGRAMS = \
	gc_provider_geonames

gc_provider_geonames_SOURCES = \
	gc-provider-geonames.c

gc_provider_geonames_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS)

gc_provider_geonames_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue-provider.la 
