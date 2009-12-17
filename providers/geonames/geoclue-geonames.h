/*
 * Geoclue
 * geoclue-geonames.h - A Geocode/ReverseGeocode provider for geonames.org
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
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

#ifndef _GEOCLUE_GEONAMES
#define _GEOCLUE_GEONAMES

#include <glib-object.h>
#include <geoclue/gc-web-service.h>

G_BEGIN_DECLS


#define GEOCLUE_TYPE_GEONAMES (geoclue_geonames_get_type ())

#define GEOCLUE_GEONAMES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GEONAMES, GeoclueGeonames))
#define GEOCLUE_GEONAMES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_TYPE_GEONAMES, GeoclueGeonamesClass))
#define GEOCLUE_IS_GEONAMES(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_GEONAMES))
#define GEOCLUE_IS_GEONAMES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_TYPE_GEONAMES))

typedef struct _GeoclueGeonames {
	GcProvider parent;
	GMainLoop *loop;
	
	GcWebService *place_geocoder;
	GcWebService *postalcode_geocoder;
	
	GcWebService *rev_street_geocoder;
	GcWebService *rev_place_geocoder;
} GeoclueGeonames;

typedef struct _GeoclueGeonamesClass {
	GcProviderClass parent_class;
} GeoclueGeonamesClass;

GType geoclue_geonames_get_type (void);

G_END_DECLS

#endif
