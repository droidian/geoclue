/*
 * Geoclue
 * geoclue-reverse-geocode.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GEOCLUE_REVERSE_GEOCODE_H
#define _GEOCLUE_REVERSE_GEOCODE_H

#include <geoclue/geoclue-provider.h>
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
geoclue_reverse_geocode_position_to_address (GeoclueReverseGeocode   *geocode,
					     double                   latitude,
					     double                   longitude,
					     GHashTable             **details,
					     GError                 **error);
	
G_END_DECLS

#endif
