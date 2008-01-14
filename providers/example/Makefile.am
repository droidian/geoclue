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
