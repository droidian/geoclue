libexec_PROGRAMS = geoclue-master

geoclue_master_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(srcdir)		\
	-I$(top_builddir)	\
	-DGEOCLUE_PROVIDERS_DIR=\""$(datadir)/geoclue-providers"\" \
	$(GEOCLUE_CFLAGS) \
	$(MASTER_CFLAGS) \
	$(CONNECTIVITY_CFLAGS)

geoclue_master_LDADD =		\
	$(top_builddir)/geoclue/libgeoclue.la	\
	$(GEOCLUE_LIBS) \
	$(MASTER_LIBS)  \
	$(CONNECTIVITY_LIBS)

NOINST_H_FILES =		\
	main.h			\
	master.h		\
	master-provider.h		\
	client.h		\
	connectivity.h		\
	connectivity-networkmanager.h	\
	connectivity-conic.h

geoclue_master_SOURCES =	\
	$(NOINST_H_FILES)	\
	client.c		\
	main.c			\
	master.c		\
	master-provider.c	\
	connectivity.c		\
	connectivity-networkmanager.c	\
	connectivity-conic.c

BUILT_SOURCES =			\
	gc-iface-master-glue.h	\
	gc-iface-master-client-glue.h

%-glue.h: stamp-%-glue.h
	@true
stamp-gc-iface-master-glue.h: ../interfaces/gc-iface-master.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_master --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F)	\
	&& echo timestamp > $(@F)

stamp-gc-iface-master-client-glue.h: ../interfaces/gc-iface-master-client.xml
	$(DBUS_BINDING_TOOL) --prefix=gc_iface_master_client --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F)	\
	&& echo timestamp > $(@F)

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Geoclue.Master.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST =			\
	$(service_in_files)	\
	$(BUILT_SOURCES)

CLEANFILES = 				\
	$(BUILT_SOURCES) 		\
	stamp-gc-iface-master-glue.h	\
	stamp-gc-iface-master-client-glue.h

DISTCLEANFILES = \
	$(service_DATA)
