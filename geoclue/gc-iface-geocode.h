/*
 * Geoclue
 * gc-iface-geocode.h - GInterface for org.freedesktop.Geocode
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GC_IFACE_GEOCODE_H
#define _GC_IFACE_GEOCODE_H

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GC_TYPE_IFACE_GEOCODE (gc_iface_geocode_get_type ())
#define GC_IFACE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_IFACE_GEOCODE, GcIfaceGeocode))
#define GC_IFACE_GEOCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_IFACE_GEOCODE, GcIfaceGeocodeClass))
#define GC_IS_IFACE_GEOCODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_IFACE_GEOCODE))
#define GC_IS_IFACE_GEOCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_IFACE_GEOCODE))
#define GC_IFACE_GEOCODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GC_TYPE_IFACE_GEOCODE, GcIfaceGeocodeClass))

typedef struct _GcIfaceGeocode GcIfaceGeocode; /* Dummy typedef */
typedef struct _GcIfaceGeocodeClass GcIfaceGeocodeClass;

struct _GcIfaceGeocodeClass {
	GTypeInterface base_iface;

	/* vtable */
	gboolean (*address_to_position) (GcIfaceGeocode        *gc,
					 GHashTable            *address,
					 GeocluePositionFields *fields,
					 double                *latitude,
					 double                *longitude,
					 double                *altitude,
					 GeoclueAccuracy      **accuracy,
					 GError               **error);
};

GType gc_iface_geocode_get_type (void);

G_END_DECLS

#endif
