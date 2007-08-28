EXTRA_DIST = geoclue_position_error.h \
             geoclue_position_error.c \
             geoclue_web_service.h \
             geoclue_web_service.c


geoclue_web_service_CFLAGS = $(DBUS_CFLAGS) $(HTTPXML_CFLAGS) $(GOBJECT_CFLAGS) $(CONIC_CFLAGS) -I$(top_srcdir)
geoclue_position_error_CFLAGS = $(GLIB_CFLAGS)

all: geoclue_web_service.o geoclue_position_error.o

geoclue_position_error.o: geoclue_position_error.c
	$(CC) $(geoclue_position_error_CFLAGS) -c geoclue_position_error.c

geoclue_web_service.o: geoclue_web_service.c
	$(CC) $(geoclue_web_service_CFLAGS) -c geoclue_web_service.c

clean:
	rm *.o
