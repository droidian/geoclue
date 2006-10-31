schema_files = geoclue.schemas

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = $(schema_files)

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_srcdir)/data/$(schema_DATA)
	pkill -HUP gconfd-2 > /dev/null 2>&1 || true
endif

uninstall-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $(top_srcdir)/data/$(schema_DATA)
	pkill -HUP gconfd-2 > /dev/null 2>&1 || true
endif

EXTRA_DIST = $(schema_DATA)
