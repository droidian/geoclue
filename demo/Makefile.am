demodir = $(libexecdir)/geoclue-2.0/demos

demo_PROGRAMS = where-am-i

where_am_i_SOURCES = where-am-i.c
where_am_i_CFLAGS = $(GEOCLUE_CFLAGS)   		\
		    $(WARN_CFLAGS)			\
		    -DLOCALEDIR="\"$(datadir)/locale\""
where_am_i_LDADD = $(GEOCLUE_LIBS)

appsdir = $(datadir)/applications
apps_DATA = geoclue-where-am-i.desktop \
	    geoclue-demo-agent.desktop

@INTLTOOL_DESKTOP_RULE@

geoclue-where-am-i.desktop.in: geoclue-where-am-i.desktop.in.in
	$(AM_V_GEN) sed -e "s|[@]libexecdir@|$(libexecdir)|g" $< > $@

geoclue-demo-agent.desktop.in: geoclue-demo-agent.desktop.in.in
	$(AM_V_GEN) sed -e "s|[@]libexecdir@|$(libexecdir)|g" $< > $@

if BUILD_DEMO_AGENT
demo_PROGRAMS += agent
endif # BUILD_DEMO_AGENT

agent_SOURCES = $(BUILT_SOURCES)       \
		gclue-service-agent.h  \
		gclue-service-agent.c  \
    		agent.c     	       \
		$(NULL)
agent_CFLAGS = $(DEMO_AGENT_CFLAGS)   		   \
	       $(WARN_CFLAGS)			   \
 	       -DLOCALEDIR="\"$(datadir)/locale\"" \
	       -DABS_SRCDIR=\""$(abs_srcdir)"\"	   \
	       -I$(top_builddir)/src		   \
	       -I$(top_builddir)/src/agent	   \
	       -I$(top_builddir)/src/public-api
agent_LDADD = $(DEMO_AGENT_LIBS) \
	      $(top_builddir)/src/agent/libgeoclue-agent.la

-include $(top_srcdir)/git.mk

EXTRA_DIST = \
	geoclue-demo-agent.desktop.in.in \
        geoclue-where-am-i.desktop.in.in \
        $(NULL)

DISTCLEANFILES = \
	geoclue-demo-agent.desktop \
	geoclue-demo-agent.desktop.in \
        geoclue-where-am-i.desktop \
        geoclue-where-am-i.desktop.in \
        $(NULL)

