noinst_PROGRAMS = \
	position-example \
	geocode-example \
	common-example

common_example_SOURCES = common-example.c
position_example_SOURCES = position-example.c
geocode_example_SOURCES = geocode-example.c

AM_CFLAGS = $(GEOCLUE_CFLAGS)
LDADD = \
	$(GEOCLUE_LIBS) \
    $(top_builddir)/geoclue/libgeoclue-provider.la
