/*
 * Geoclue
 * geoclue-geocode.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GEOCLUE_GEOCODE_H
#define _GEOCLUE_GEOCODE_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-address-details.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_GEOCODE (geoclue_geocode_get_type ())
#define GEOCLUE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GEOCODE, GeoclueGeocode))
#define GEOCLUE_IS_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_GEOCODE))

#define GEOCLUE_GEOCODE_INTERFACE_NAME "org.freedesktop.Geoclue.Geocode"

typedef struct _GeoclueGeocode {
	GeoclueProvider provider;
} GeoclueGeocode;

typedef struct _GeoclueGeocodeClass {
	GeoclueProviderClass provider_class;
} GeoclueGeocodeClass;

GType geoclue_geocode_get_type (void);

GeoclueGeocode *geoclue_geocode_new (const char *service,
				     const char *path);

GeocluePositionFields 
geoclue_geocode_address_to_position (GeoclueGeocode   *geocode,
				     GHashTable       *details,
				     double           *latitude,
				     double           *longitude,
				     double           *altitude,
				     GeoclueAccuracy **accuracy,
				     GError          **error);
	
G_END_DECLS

#endif
