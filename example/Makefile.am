bin_PROGRAMS = geoclue_position_example geoclue_map_example  geoclue_map_layout_example

geoclue_map_example_SOURCES = geoclue_map_example.c
geoclue_map_layout_example_SOURCES = geoclue_map_layout_example.c
geoclue_position_example_SOURCES = geoclue_position_example.c

AM_CPPFLAGS = $(DEPS_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(DEPS_LIBS) $(top_builddir)/geoclue/libgeoclue.la

