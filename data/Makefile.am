servicedir       = $(datadir)/dbus-1/system-services
service_in_files = org.freedesktop.GeoClue2.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< | \
	sed -e "s|\@dbus_srv_user\@|$(dbus_srv_user)|" > $@

# DBus Service policy file
dbusservicedir = $(DBUS_SYS_DIR)
dbusservice_DATA = org.freedesktop.GeoClue2.conf

DISTCLEANFILES = $(service_DATA)
EXTRA_DIST = $(service_in_files)
