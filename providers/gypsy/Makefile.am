libexec_PROGRAMS = geoclue-gypsy

geoclue_gypsy_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GEOCLUE_CFLAGS)	\
	$(GYPSY_CFLAGS)

geoclue_gypsy_LDADD =		\
	$(GEOCLUE_LIBS)		\
	$(GYPSY_LIBS)		\
	$(top_builddir)/geoclue/libgeoclue-provider.la

geoclue_gypsy_SOURCES =		\
	geoclue-gypsy.c
