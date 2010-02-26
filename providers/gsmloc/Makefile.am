libexec_PROGRAMS =	\
	geoclue-gsmloc

nodist_geoclue_gsmloc_SOURCES = \
	ofono-marshal.c \
	ofono-marshal.h \
	ofono-manager-bindings.h \
	ofono-modem-bindings.h \
	ofono-network-registration-bindings.h \
	ofono-network-operator-bindings.h

BUILT_SOURCES = \
	$(nodist_geoclue_gsmloc_SOURCES)

geoclue_gsmloc_SOURCES = \
	mcc.h \
	geoclue-gsmloc.c \
	geoclue-gsmloc-ofono.c \
	geoclue-gsmloc-ofono.h


geoclue_gsmloc_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GEOCLUE_CFLAGS)

geoclue_gsmloc_LDADD = \
	$(GEOCLUE_LIBS) \
	$(top_builddir)/geoclue/libgeoclue.la 

providersdir = $(datadir)/geoclue-providers
providers_DATA = geoclue-gsmloc.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Providers.Gsmloc.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = 			\
	ofono-marshal.list	\
	ofono-manager.xml \
	ofono-modem.xml \
	ofono-network-operator.xml \
	ofono-network-registration.xml \
	$(service_in_files)	\
	$(providers_DATA)

CLEANFILES = \
	$(BUILT_SOURCES)

DISTCLEANFILES = \
	$(service_DATA)

%-bindings.h: stamp-%-bindings.h
	@true
stamp-%-bindings.h: %.xml
	$(DBUS_BINDING_TOOL) --mode=glib-client $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

ofono-marshal.h: ofono-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=ofono_marshal > $@
ofono-marshal.c: ofono-marshal.list ofono-marshal.h $(GLIB_GENMARSHAL)
	echo "#include \"ofono-marshal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) --prefix=ofono_marshal $(srcdir)/ofono-marshal.list --body >> $@
