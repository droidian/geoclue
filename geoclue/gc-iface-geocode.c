/*
 * Geoclue
 * gc-iface-geocode.c - GInterface for org.freedesktop.Geocode
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

/**
 * SECTION:geoclue-geocode
 * @short_description: Geoclue Geocoding client API
 *
 * #GeoclueGeocode contains Geocoding-related methods. 
 * It is part of the Geoclue public C client API which uses D-Bus 
 * to communicate with the actual provider.
 * 
 * After a #GeoclueGeocode is created with geoclue_geocode_new(), the 
 * geoclue_geocode_address_to_position() and geoclue_geocode_address_to_position_async() 
 * methods can be used to obtain the position (coordinates) of a given address. 
 * 
 * Address #GHashTable keys are defined in 
 * <ulink url="geoclue-types.html">geoclue-types.h</ulink>. See also 
 * convenience functions in 
 * <ulink url="geoclue-address-details.html">geoclue-address-details.h</ulink>.
 */

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
