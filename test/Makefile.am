if HAVE_GTK

noinst_PROGRAMS = geoclue-test-gui

geoclue_test_gui_LDADD = \
	$(GTK_LIBS) \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la

geoclue_test_gui_CFLAGS= \
	-DGEOCLUE_PROVIDERS_DIR=\""$(datadir)/geoclue-providers"\" \
	$(GTK_CFLAGS) \
	$(GEOCLUE_CFLAGS) \
	-I$(top_builddir) -I$(top_srcdir)

geoclue_test_gui_SOURCES = \
	geoclue-test-gui.c

endif
