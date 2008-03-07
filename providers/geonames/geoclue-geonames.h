/*
 * Geoclue
 * geoclue-geonames.h - A Geocode/ReverseGeocode provider for geonames.org
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
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
