/*
 * Geoclue
 * gc-iface-velocity.c - GInterface for org.freedesktop.Geoclue.Velocity
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
#include <geoclue/gc-iface-velocity.h>
#include <geoclue/geoclue-marshal.h>

enum {
	VELOCITY_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static gboolean 
gc_iface_velocity_get_velocity (GcIfaceVelocity       *velocity,
				int                   *fields,
				int                   *timestamp,
				double                *latitude,
				double                *longitude,
				double                *altitude,
				GError               **error);

#include "gc-iface-velocity-glue.h"

static void
gc_iface_velocity_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	signals[VELOCITY_CHANGED] = g_signal_new ("velocity-changed",
						  G_OBJECT_CLASS_TYPE (klass),
						  G_SIGNAL_RUN_LAST, 0,
						  NULL, NULL,
						  geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE,
						  G_TYPE_NONE, 5,
						  G_TYPE_INT,
						  G_TYPE_INT,
						  G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE);
	dbus_g_object_type_install_info (gc_iface_velocity_get_type (),
					 &dbus_glib_gc_iface_velocity_object_info);
}

GType
gc_iface_velocity_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceVelocityClass),
			gc_iface_velocity_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceVelocity", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_velocity_get_velocity (GcIfaceVelocity *gc,
				int             *fields,
				int             *timestamp,
				double          *speed,
				double          *direction,
				double          *climb,
				GError         **error)
{
	return GC_IFACE_VELOCITY_GET_CLASS (gc)->get_velocity 
		(gc, (GeoclueVelocityFields *) fields, timestamp,
		 speed, direction, climb, error);
}

void
gc_iface_velocity_emit_velocity_changed (GcIfaceVelocity      *gc,
					 GeoclueVelocityFields fields,
					 int                   timestamp,
					 double                speed,
					 double                direction,
					 double                climb)
{
	g_signal_emit (gc, signals[VELOCITY_CHANGED], 0, fields, timestamp,
		       speed, direction, climb);
}
