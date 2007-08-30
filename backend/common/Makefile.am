EXTRA_DIST = geoclue_web_service.h geoclue_position_error.h

noinst_LIBRARIES = libgeoclue_web_service.a libgeoclue_position_error.a

libgeoclue_web_service_a_SOURCES = geoclue_web_service.c
libgeoclue_web_service_a_CFLAGS = $(DBUS_CFLAGS) $(HTTPXML_CFLAGS) $(GOBJECT_CFLAGS) $(CONIC_CFLAGS) -I$(top_srcdir)


libgeoclue_position_error_a_SOURCES = geoclue_position_error.c
libgeoclue_position_error_a_CFLAGS = $(GLIB_CFLAGS)
