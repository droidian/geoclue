/*
 * Geoclue
 * geoclue-geocode.c - Client API for accessing GcIfaceGeocode
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <geoclue/geoclue-geocode.h>
#include <geoclue/geoclue-marshal.h>

#include "gc-iface-geocode-bindings.h"

typedef struct _GeoclueGeocodePrivate {
	int dummy;
} GeoclueGeocodePrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_GEOCODE, GeoclueGeocodePrivate))

G_DEFINE_TYPE (GeoclueGeocode, geoclue_geocode, GEOCLUE_TYPE_PROVIDER);

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (geoclue_geocode_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	G_OBJECT_CLASS (geoclue_geocode_parent_class)->dispose (object);
}

static void
geoclue_geocode_class_init (GeoclueGeocodeClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;

	g_type_class_add_private (klass, sizeof (GeoclueGeocodePrivate));
}

static void
geoclue_geocode_init (GeoclueGeocode *geocode)
{
}

GeoclueGeocode *
geoclue_geocode_new (const char *service,
		     const char *path)
{
	return g_object_new (GEOCLUE_TYPE_GEOCODE,
			     "service", service,
			     "path", path,
			     "interface", GEOCLUE_GEOCODE_INTERFACE_NAME,
			     NULL);
}

GeocluePositionFields
geoclue_geocode_address_to_position (GeoclueGeocode   *geocode,
				     GHashTable       *details,
				     double           *latitude,
				     double           *longitude,
				     double           *altitude,
				     GeoclueAccuracy **accuracy,
				     GError          **error)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (geocode);
	int fields;
	double la, lo, al;
	GeoclueAccuracy *acc;
	
	if (!org_freedesktop_Geoclue_Geocode_address_to_position (provider->proxy,
								  details, &fields,
								  &la, &lo, &al,
								  &acc, error)){
		return GEOCLUE_POSITION_FIELDS_NONE;
	}

	if (latitude != NULL && (fields & GEOCLUE_POSITION_FIELDS_LATITUDE)) {
		*latitude = la;
	}

	if (longitude != NULL && (fields & GEOCLUE_POSITION_FIELDS_LONGITUDE)) {
		*longitude = lo;
	}

	if (altitude != NULL && (fields & GEOCLUE_POSITION_FIELDS_ALTITUDE)) {
		*altitude = al;
	}

	if (accuracy != NULL) {
		*accuracy = acc;
	}

	return fields;
}
