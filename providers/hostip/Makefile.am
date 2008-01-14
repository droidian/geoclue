libexec_PROGRAMS =	\
	geoclue-hostip

geoclue_hostip_SOURCES = \
	geoclue-hostip.c

geoclue_hostip_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS)

geoclue_hostip_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-hostip.provider
