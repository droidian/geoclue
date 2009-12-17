/*
 * Geoclue
 * geoclue-geocode.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
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

typedef void (*GeoclueGeocodeCallback) (GeoclueGeocode       *geocode,
					GeocluePositionFields fields,
					double                latitude,
					double                longitude,
					double                altitude,
					GeoclueAccuracy      *accuracy,
					GError               *error,
					gpointer              userdata);

void geoclue_geocode_address_to_position_async (GeoclueGeocode         *geocode,
						GHashTable             *details,
						GeoclueGeocodeCallback  callback,
						gpointer                userdata);


G_END_DECLS

#endif
