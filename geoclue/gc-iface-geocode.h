/*
 * Geoclue
 * gc-iface-geocode.h - GInterface for org.freedesktop.Geocode
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

	gboolean (*freeform_address_to_position) (GcIfaceGeocode        *gc,
	                                          const char            *address,
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
