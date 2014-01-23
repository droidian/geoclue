noinst_PROGRAMS = where-am-i

where_am_i_SOURCES = where-am-i.c
where_am_i_CFLAGS = $(GEOCLUE_CFLAGS)   		\
		    $(WARN_CFLAGS)			\
		    -DLOCALEDIR="\"$(datadir)/locale\""
where_am_i_LDADD = $(GEOCLUE_LIBS)

if BUILD_DEMO_AGENT
noinst_PROGRAMS += agent
endif # BUILD_DEMO_AGENT

interface_data = $(top_builddir)/src/agent/geoclue-agent-interface.xml
dbus_built_sources = geoclue-agent-interface.c geoclue-agent-interface.h
geoclue-agent-interface.c: geoclue-agent-interface.h
geoclue-agent-interface.h: Makefile.am $(interface_data)
	gdbus-codegen						\
		--interface-prefix org.freedesktop.GeoClue2.	\
		--c-namespace GClue				\
		--generate-c-code geoclue-agent-interface	\
		$(interface_data)

BUILT_SOURCES = $(dbus_built_sources) \
		$(NULL)

agent_SOURCES = $(BUILT_SOURCES)       \
		gclue-service-agent.h  \
		gclue-service-agent.c  \
    		agent.c     	       \
		$(NULL)
agent_CFLAGS = $(DEMO_AGENT_CFLAGS)   		   \
	       $(WARN_CFLAGS)			   \
 	       -DLOCALEDIR="\"$(datadir)/locale\"" \
	       -DABS_SRCDIR=\""$(abs_srcdir)"\"
agent_LDADD = $(DEMO_AGENT_LIBS)

-include $(top_srcdir)/git.mk
