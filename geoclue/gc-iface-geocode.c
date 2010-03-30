/*
 * Geoclue
 * gc-iface-geocode.c - GInterface for org.freedesktop.Geocode
 * 
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@linux.intel.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *           2010 Intel Corporation
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

/* This is a GInterface for implementing Geoclue Geocode providers */

#include <glib.h>

#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-accuracy.h>
#include <geoclue/gc-iface-geocode.h>

static gboolean 
gc_iface_geocode_address_to_position (GcIfaceGeocode   *gc,
				      GHashTable       *address,
				      int              *fields,
				      double           *latitude,
				      double           *longitude,
				      double           *altitude,
				      GeoclueAccuracy **accuracy,
				      GError          **error);

static gboolean
gc_iface_geocode_freeform_address_to_position (GcIfaceGeocode   *gc,
                                               const char       *address,
                                               int              *fields,
                                               double           *latitude,
                                               double           *longitude,
                                               double           *altitude,
                                               GeoclueAccuracy **accuracy,
                                               GError          **error);
#include "gc-iface-geocode-glue.h"

static void
gc_iface_geocode_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	dbus_g_object_type_install_info (gc_iface_geocode_get_type (),
					 &dbus_glib_gc_iface_geocode_object_info);
}

GType
gc_iface_geocode_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceGeocodeClass),
			gc_iface_geocode_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceGeocode", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_geocode_address_to_position (GcIfaceGeocode   *gc,
				      GHashTable       *address,
				      int              *fields,
				      double           *latitude,
				      double           *longitude,
				      double           *altitude,
				      GeoclueAccuracy **accuracy,
				      GError          **error)
{
	return GC_IFACE_GEOCODE_GET_CLASS (gc)->address_to_position 
		(gc, address, (GeocluePositionFields *) fields,
		 latitude, longitude, altitude, accuracy, error);
}

static gboolean
gc_iface_geocode_freeform_address_to_position (GcIfaceGeocode   *gc,
                                               const char       *address,
                                               int              *fields,
                                               double           *latitude,
                                               double           *longitude,
                                               double           *altitude,
                                               GeoclueAccuracy **accuracy,
                                               GError          **error)
{
	return GC_IFACE_GEOCODE_GET_CLASS (gc)->freeform_address_to_position
		(gc, address, (GeocluePositionFields *) fields,
		 latitude, longitude, altitude, accuracy, error);
}
