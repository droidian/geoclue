/*
 * Geoclue
 * geoclue-reverse-geocode.h - 
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

#ifndef _GEOCLUE_REVERSE_GEOCODE_H
#define _GEOCLUE_REVERSE_GEOCODE_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-types.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_REVERSE_GEOCODE (geoclue_reverse_geocode_get_type ())
#define GEOCLUE_REVERSE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_REVERSE_GEOCODE, GeoclueReverseGeocode))
#define GEOCLUE_IS_REVERSE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_GEOCODE_REVERSE))

#define GEOCLUE_REVERSE_GEOCODE_INTERFACE_NAME "org.freedesktop.Geoclue.ReverseGeocode"

typedef struct _GeoclueReverseGeocode {
	GeoclueProvider provider;
} GeoclueReverseGeocode;

typedef struct _GeoclueReverseGeocodeClass {
	GeoclueProviderClass provider_class;
} GeoclueReverseGeocodeClass;

GType geoclue_reverse_geocode_get_type (void);

GeoclueReverseGeocode *geoclue_reverse_geocode_new (const char *service,
						    const char *path);

gboolean 
geoclue_reverse_geocode_position_to_address (GeoclueReverseGeocode   *revgeocode,
					     double                   latitude,
					     double                   longitude,
					     GeoclueAccuracy         *position_accuracy,
					     GHashTable             **details,
					     GeoclueAccuracy        **address_accuracy,
					     GError                 **error);

typedef void (*GeoclueReverseGeocodeCallback) (GeoclueReverseGeocode *revgeocode,
					       GHashTable            *details,
					       GeoclueAccuracy       *accuracy,
					       GError                *error,
					       gpointer               userdata);

void geoclue_reverse_geocode_position_to_address_async (GeoclueReverseGeocode        *revgeocode,
							double                        latitude,
							double                        longitude,
							GeoclueAccuracy              *accuracy,
							GeoclueReverseGeocodeCallback callback,
							gpointer                      userdata);


G_END_DECLS

#endif
