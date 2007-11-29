/*
 * Geoclue
 * gc-iface-reverse-geocode.c - GInterface for org.freedesktop.ReverseGeocode
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <glib.h>

#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-accuracy.h>
#include <geoclue/gc-iface-reverse-geocode.h>

static gboolean 
gc_iface_reverse_geocode_position_to_address (GcIfaceReverseGeocode  *gc,
					      double                  latitude,
					      double                  longitude,
					      GHashTable            **address,
					      GError                **error);
#include "gc-iface-reverse-geocode-glue.h"

static void
gc_iface_reverse_geocode_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	dbus_g_object_type_install_info (gc_iface_reverse_geocode_get_type (),
					 &dbus_glib_gc_iface_reverse_geocode_object_info);
}

GType
gc_iface_reverse_geocode_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceReverseGeocodeClass),
			gc_iface_reverse_geocode_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceReverseGeocode", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_reverse_geocode_position_to_address (GcIfaceReverseGeocode  *gc,
					      double                  latitude,
					      double                  longitude,
					      GHashTable            **address,
					      GError                **error)
{
	return GC_IFACE_REVERSE_GEOCODE_GET_CLASS (gc)->position_to_address 
		(gc, latitude, longitude, address, error);
}
