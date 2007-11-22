/*
 * Geoclue
 * gc-iface-geocode.c - GInterface for org.freedesktop.Geocode
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <glib.h>

#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-accuracy.h>
#include <geoclue/gc-iface-geocode.h>

static gboolean 
gc_iface_geocode_address_to_position (GcIfaceGeocode   *gc,
				      GHashTable       *address,
				      int              *fields,
				      double           *latitude,
				      double           *longitude,
				      double           *altitude,
				      GeoclueAccuracy **accuracy,
				      GError          **error);
#include "gc-iface-geocode-glue.h"

static void
gc_iface_geocode_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	dbus_g_object_type_install_info (gc_iface_geocode_get_type (),
					 &dbus_glib_gc_iface_geocode_object_info);
}

GType
gc_iface_geocode_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceGeocodeClass),
			gc_iface_geocode_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceGeocode", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_geocode_address_to_position (GcIfaceGeocode   *gc,
				      GHashTable       *address,
				      int              *fields,
				      double           *latitude,
				      double           *longitude,
				      double           *altitude,
				      GeoclueAccuracy **accuracy,
				      GError          **error)
{
	return GC_IFACE_GEOCODE_GET_CLASS (gc)->address_to_position 
		(gc, address, (GeocluePositionFields *) fields,
		 latitude, longitude, altitude, accuracy, error);
}
