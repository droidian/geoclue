include $(top_srcdir)/Makefile.decl

BUILT_GIRSOURCES =

AM_CFLAGS = \
	-I$(top_srcdir) \
	$(GEOIP_SERVER_CFLAGS) \
	$(COMMON_CFLAGS) \
	$(WARN_CFLAGS)  \
	$(DISABLE_DEPRECATED) \
	-DGEOIP_DATABASE_PATH=\""$(GEOIP_DATABASE_PATH)"\" \
	-DTEST_SRCDIR=\""$(top_srcdir)/src/test-data/"\" \
	-DBUILDDIR=\""$(builddir)/"\"

bin_PROGRAMS = geoip-lookup geoip-update

TEST_PROGS += test-geoipformat
noinst_PROGRAMS = $(TEST_PROGS)

geoip_lookup_SOURCES = geoip-lookup.c geoip-server.h

geoip_lookup_LDADD = $(GEOIP_SERVER_LIBS)
geoip_update_LDADD = $(GEOIP_DBUPDATE_LIBS)
test_geoipformat_LDADD = $(GEOCLUE_LIBS)

EXTRA_DIST = API-Documentation.txt

MAINTAINERCLEANFILES = Makefile.in

-include $(top_srcdir)/git.mk
