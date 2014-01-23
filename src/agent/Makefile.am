include $(top_srcdir)/Makefile.decl

interfacedir = $(datadir)/geoclue-2.0
interface_DATA = geoclue-agent-interface.xml

docs-org.freedesktop.GeoClue2.Agent.xml: Makefile.am $(interface_DATA)
	$(AM_V_GEN)$(GDBUS_CODEGEN) \
		--interface-prefix org.freedesktop.GeoClue2.	\
		--c-namespace GClue				\
    		--generate-docbook=docs				\
		$(srcdir)/$(interface_DATA)
noinst_DATA = docs-org.freedesktop.GeoClue2.Agent.xml

CLEANFILES = $(noinst_DATA)
EXTRA_DIST = $(interface_DATA)

-include $(top_srcdir)/git.mk
