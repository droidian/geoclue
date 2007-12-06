/*
 * Geoclue
 * gc-iface-reverse_geocode.h - GInterface for org.freedesktop.Reverse_Geocode
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GC_IFACE_REVERSE_GEOCODE_H
#define _GC_IFACE_REVERSE_GEOCODE_H

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GC_TYPE_IFACE_REVERSE_GEOCODE (gc_iface_reverse_geocode_get_type ())
#define GC_IFACE_REVERSE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_IFACE_REVERSE_GEOCODE, GcIfaceReverseGeocode))
#define GC_IFACE_REVERSE_GEOCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_IFACE_REVERSE_GEOCODE, GcIfaceReverseGeocodeClass))
#define GC_IS_IFACE_REVERSE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_IFACE_REVERSE_GEOCODE))
#define GC_IS_IFACE_REVERSE_GEOCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_IFACE_REVERSE_GEOCODE))
#define GC_IFACE_REVERSE_GEOCODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GC_TYPE_IFACE_REVERSE_GEOCODE, GcIfaceReverseGeocodeClass))

typedef struct _GcIfaceReverseGeocode GcIfaceReverseGeocode; /* Dummy typedef */
typedef struct _GcIfaceReverseGeocodeClass GcIfaceReverseGeocodeClass;

struct _GcIfaceReverseGeocodeClass {
	GTypeInterface base_iface;

	/* vtable */
	gboolean (*position_to_address) (GcIfaceReverseGeocode  *gc,
					 double                  latitude,
					 double                  longitude,
					 GHashTable            **address,
					 GError                **error);
};

GType gc_iface_reverse_geocode_get_type (void);

G_END_DECLS

#endif
