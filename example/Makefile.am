noinst_PROGRAMS = \
	position-example \
	address-example \
	geocode-example \
	revgeocode-example \
	common-example

common_example_SOURCES = common-example.c
position_example_SOURCES = position-example.c
address_example_SOURCES = address-example.c
geocode_example_SOURCES = geocode-example.c
revgeocode_example_SOURCES = revgeocode-example.c

AM_CFLAGS = $(GEOCLUE_CFLAGS)
LDADD = \
	$(GEOCLUE_LIBS) \
    $(top_builddir)/geoclue/libgeoclue.la
