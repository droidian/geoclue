noinst_PROGRAMS = where-am-i

where_am_i_SOURCES = where-am-i.c
where_am_i_CFLAGS = $(GEOCLUE_CFLAGS)   		\
		    $(WARN_CFLAGS)			\
		    -DLOCALEDIR="\"$(datadir)/locale\""
where_am_i_LDADD = $(GEOCLUE_LIBS)
