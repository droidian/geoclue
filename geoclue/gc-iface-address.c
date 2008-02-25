/*
 * Geoclue
 * gc-iface-address.c - GInterface for org.freedesktop.Address
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <glib.h>

#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-marshal.h>
#include <geoclue/gc-iface-address.h>

enum {
	ADDRESS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static gboolean 
gc_iface_address_get_address (GcIfaceAddress   *gc,
			      int              *timestamp,
			      GHashTable      **address,
			      GeoclueAccuracy **accuracy,
			      GError          **error);
#include "gc-iface-address-glue.h"

static void
gc_iface_address_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	signals[ADDRESS_CHANGED] = g_signal_new ("address-changed",
						 G_OBJECT_CLASS_TYPE (klass),
						 G_SIGNAL_RUN_LAST, 0,
						 NULL, NULL,
						 geoclue_marshal_VOID__INT_POINTER_BOXED,
						 G_TYPE_NONE, 3,
						 G_TYPE_INT, 
						 dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING),
						 dbus_g_type_get_collection ("GPtrArray", GEOCLUE_ACCURACY_TYPE));
	dbus_g_object_type_install_info (gc_iface_address_get_type (),
					 &dbus_glib_gc_iface_address_object_info);
}

GType
gc_iface_address_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfaceAddressClass),
			gc_iface_address_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfaceAddress", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_address_get_address (GcIfaceAddress   *gc,
			      int              *timestamp,
			      GHashTable      **address,
			      GeoclueAccuracy **accuracy,
			      GError          **error)
{
	return GC_IFACE_ADDRESS_GET_CLASS (gc)->get_address 
		(gc, timestamp, address, accuracy, error);
}

void
gc_iface_address_emit_address_changed (GcIfaceAddress  *gc,
				       int              timestamp,
				       GHashTable      *address,
				       GeoclueAccuracy *accuracy)
{
	g_signal_emit (gc, signals[ADDRESS_CHANGED], 0, timestamp,
		       address, accuracy);
}
		       
