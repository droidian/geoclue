/*
 * Geoclue
 * geoclue-geocode.c - Client API for accessing GcIfaceGeocode
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-geocode
 * @short_description: Geoclue geocode client API
 *
 * #GeoclueGeocode contains geocoding methods. 
 * It is part of the Geoclue public client API which uses the D-Bus 
 * API to communicate with the actual provider.
 * 
 * After a #GeoclueGeocode is created with geoclue_geocode_new(), the 
 * geoclue_geocode_address_to_position() method can be used to 
 * get the position of a known address.
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

/**
 * geoclue_geocode_new:
 * @service: D-Bus service name
 * @path: D-Bus path name
 *
 * Creates a #GeoclueGeocode with given D-Bus service name and path.
 * 
 * Return value: Pointer to a new #GeoclueGeocode
 */
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

/**
 * geoclue_geocode_address_to_position:
 * @geocode: A #GeoclueGeocode object
 * @details: Hashtable with address data
 * @latitude: Pointer to returned latitude in degrees
 * @longitude: Pointer to returned longitude in degrees
 * @altitude: Pointer to returned altitude in meters
 * @accuracy: Pointer to returned #GeoclueAccuracy
 * @error: Pointer to returned #Gerror
 * 
 * Geocodes given address to a position. @accuracy is a rough estimate 
 * of the accuracy of the returned position.
 * 
 * If the caller is not interested in some values, the pointers can be 
 * left %NULL.
 * 
 * Return value: A #GeocluePositionFields bitfield representing the 
 * validity of the position values.
 */
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
