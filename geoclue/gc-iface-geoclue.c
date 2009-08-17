/*
 * Geoclue
 * gc-iface-geoclue.c - GInterface for org.freedesktop.Geoclue
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

#include <geoclue/gc-iface-geoclue.h>

enum {
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static gboolean gc_iface_geoclue_get_provider_info (GcIfaceGeoclue  *gc,
						    gchar          **name,
						    gchar          **description,
						    GError         **error);
static gboolean gc_iface_geoclue_get_status (GcIfaceGeoclue *gc,
					     GeoclueStatus  *status,
					     GError        **error);
static gboolean gc_iface_geoclue_set_options (GcIfaceGeoclue *gc,
                                              GHashTable     *options,
                                              GError        **error);
static void gc_iface_geoclue_add_reference (GcIfaceGeoclue *gc,
                                            DBusGMethodInvocation *context);
static void gc_iface_geoclue_remove_reference (GcIfaceGeoclue *gc,
                                               DBusGMethodInvocation *context);

#include "gc-iface-geoclue-glue.h"


static void
gc_iface_geoclue_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
						G_OBJECT_CLASS_TYPE (klass),
						G_SIGNAL_RUN_LAST, 
						G_STRUCT_OFFSET (GcIfaceGeoclueClass, status_changed),
						NULL, NULL,
						g_cclosure_marshal_VOID__INT,
						G_TYPE_NONE, 1,
						G_TYPE_INT);
	dbus_g_object_type_install_info (gc_iface_geoclue_get_type (),
					 &dbus_glib_gc_iface_geoclue_object_info);
}

GType
gc_iface_geoclue_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceGeoclueClass),
			gc_iface_geoclue_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceGeoclue", &info, 0);
	}

	return type;
}

static gboolean
gc_iface_geoclue_get_provider_info (GcIfaceGeoclue  *gc,
				    gchar          **name,
				    gchar          **description,
				    GError         **error)
{
	return GC_IFACE_GEOCLUE_GET_CLASS (gc)->get_provider_info (gc, 
								   name,
								   description,
								   error);
}

static gboolean
gc_iface_geoclue_get_status (GcIfaceGeoclue *gc,
			     GeoclueStatus  *status,
			     GError        **error)
{
	return GC_IFACE_GEOCLUE_GET_CLASS (gc)->get_status (gc, status,
							    error);
}

static gboolean
gc_iface_geoclue_set_options (GcIfaceGeoclue *gc,
                              GHashTable     *options,
                              GError        **error)
{
        return GC_IFACE_GEOCLUE_GET_CLASS (gc)->set_options (gc, options, 
                                                             error);
}

static void 
gc_iface_geoclue_add_reference (GcIfaceGeoclue *gc,
                                DBusGMethodInvocation *context)
{
	GC_IFACE_GEOCLUE_GET_CLASS (gc)->add_reference (gc, context);
}
static void 
gc_iface_geoclue_remove_reference (GcIfaceGeoclue *gc,
                                   DBusGMethodInvocation *context)
{
	GC_IFACE_GEOCLUE_GET_CLASS (gc)->remove_reference (gc, context);
}

void
gc_iface_geoclue_emit_status_changed (GcIfaceGeoclue *gc,
				      GeoclueStatus   status)
{
	g_signal_emit (gc, signals[STATUS_CHANGED], 0, status);
}
