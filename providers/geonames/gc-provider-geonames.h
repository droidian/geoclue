/*
 * Geoclue
 * gc-provider-geonames.h - A Geocode/ReverseGeocode provider for geonames.org
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 */

#ifndef _GC_PROVIDER_GEONAMES
#define _GC_PROVIDER_GEONAMES

#include <glib-object.h>
#include <geoclue/gc-web-service.h>

G_BEGIN_DECLS


#define GC_TYPE_PROVIDER_GEONAMES (gc_provider_geonames_get_type ())

#define GC_PROVIDER_GEONAMES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_PROVIDER_GEONAMES, GcProviderGeonames))
#define GC_PROVIDER_GEONAMES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_PROVIDER_GEONAMES, GcProviderGeonamesClass))
#define GC_IS_PROVIDER_GEONAMES(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_PROVIDER_GEONAMES))
#define GC_IS_PROVIDER_GEONAMES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_PROVIDER_GEONAMES))

typedef struct _GcProviderGeonames {
	GcProvider parent;
	GMainLoop *loop;
	
	GcWebService *place_geocoder;
	GcWebService *postalcode_geocoder;
	
	GcWebService *rev_street_geocoder;
	GcWebService *rev_place_geocoder;
} GcProviderGeonames;

typedef struct _GcProviderGeonamesClass {
	GcProviderClass parent_class;
} GcProviderGeonamesClass;

GType gc_provider_geonames_get_type (void);

G_END_DECLS

#endif
