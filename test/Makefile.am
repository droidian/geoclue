if HAVE_GTK

bin_PROGRAMS = geoclue-test-gui

geoclue_test_gui_LDADD = \
	$(GTK_LIBS) \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la

geoclue_test_gui_CFLAGS= \
	$(GTK_CFLAGS) \
	$(GEOCLUE_CFLAGS)

geoclue_test_gui_SOURCES = \
	geoclue-test-gui.c

endif
