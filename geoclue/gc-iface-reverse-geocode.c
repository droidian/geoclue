/*
 * Geoclue
 * gc-iface-reverse-geocode.c - GInterface for org.freedesktop.ReverseGeocode
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

#include <glib.h>

#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-accuracy.h>
#include <geoclue/gc-iface-reverse-geocode.h>

static gboolean 
gc_iface_reverse_geocode_position_to_address (GcIfaceReverseGeocode  *gc,
					      double                  latitude,
					      double                  longitude,
					      GeoclueAccuracy        *position_accuracy,
					      GHashTable            **address,
					      GeoclueAccuracy       **address_accuracy,
					      GError                **error);
#include "gc-iface-reverse-geocode-glue.h"

static void
gc_iface_reverse_geocode_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	dbus_g_object_type_install_info (gc_iface_reverse_geocode_get_type (),
					 &dbus_glib_gc_iface_reverse_geocode_object_info);
}

GType
gc_iface_reverse_geocode_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceReverseGeocodeClass),
			gc_iface_reverse_geocode_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceReverseGeocode", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_reverse_geocode_position_to_address (GcIfaceReverseGeocode  *gc,
					      double                  latitude,
					      double                  longitude,
					      GeoclueAccuracy        *position_accuracy,
					      GHashTable            **address,
					      GeoclueAccuracy       **address_accuracy,
					      GError                **error)
{
	return GC_IFACE_REVERSE_GEOCODE_GET_CLASS (gc)->position_to_address 
		(gc, latitude, longitude, position_accuracy, 
		 address, address_accuracy, error);
}
