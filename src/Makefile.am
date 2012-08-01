libexec_PROGRAMS = geoclue-master
noinst_LTLIBRARIES = libconnectivity.la
noinst_PROGRAMS = test-connectivity

AM_CFLAGS =			\
	-I$(top_srcdir)		\
	-I$(srcdir)		\
	-I$(top_builddir)	\
	-DGEOCLUE_PROVIDERS_DIR=\""$(datadir)/geoclue-providers"\" \
	$(GEOCLUE_CFLAGS) \
	$(MASTER_CFLAGS) \
	$(CONNECTIVITY_CFLAGS)

geoclue_master_LDADD =				\
	$(top_builddir)/geoclue/libgeoclue.la	\
	libconnectivity.la			\
	$(GEOCLUE_LIBS)				\
	$(MASTER_LIBS)

NOINST_H_FILES =		\
	main.h			\
	master.h		\
	master-provider.h	\
	client.h

libconnectivity_la_SOURCES =		\
	connectivity.h			\
	connectivity-networkmanager.h	\
	connectivity-conic.h		\
	connectivity-connman.h		\
	connectivity.c			\
	connectivity-networkmanager.c	\
	connectivity-conic.c		\
	connectivity-connman.c

libconnectivity_la_LIBADD = $(CONNECTIVITY_LIBS)

test_connectivity_SOURCES = test-connectivity.c
test_connectivity_LDADD = libconnectivity.la $(GEOCLUE_LIBS)

geoclue_master_SOURCES =	\
	$(NOINST_H_FILES)	\
	client.c		\
	main.c			\
	master.c		\
	master-provider.c

BUILT_SOURCES =			\
	gc-iface-master-glue.h	\
	gc-iface-master-client-glue.h

%-glue.h: stamp-%-glue.h
	@true
stamp-gc-iface-master-glue.h: ../interfaces/gc-iface-master.xml
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --prefix=gc_iface_master --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F)	\
	&& echo timestamp > $(@F)

stamp-gc-iface-master-client-glue.h: ../interfaces/gc-iface-master-client.xml
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --prefix=gc_iface_master_client --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F)	\
	&& echo timestamp > $(@F)

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Master.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = geoclue

gsettings_SCHEMAS = org.freedesktop.Geoclue.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST =			\
	$(service_in_files)	\
	$(gsettings_SCHEMAS)	\
	$(convert_DATA)

CLEANFILES = 				\
	$(BUILT_SOURCES) 		\
	stamp-gc-iface-master-glue.h	\
	stamp-gc-iface-master-client-glue.h

DISTCLEANFILES = \
	$(service_DATA)
