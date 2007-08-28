geoclue_web_service_CFLAGS = $(DBUS_CFLAGS) $(HTTPXML_CFLAGS) $(GOBJECT_CFLAGS) $(CONIC_CFLAGS) -I$(top_srcdir)

all: geoclue_web_service.o

geoclue_web_service.o: geoclue_web_service.c
	$(CC) $(geoclue_web_service_CFLAGS) -c geoclue_web_service.c

clean:
	rm geoclue_web_service.o
