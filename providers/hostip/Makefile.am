libexec_PROGRAMS =	\
	gc_provider_hostip

gc_provider_hostip_SOURCES = \
	gc-provider-hostip.c

gc_provider_hostip_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS)

gc_provider_hostip_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue-provider.la 
