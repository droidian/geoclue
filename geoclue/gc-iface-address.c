/*
 * Geoclue
 * gc-iface-address.c - GInterface for org.freedesktop.Address
 * 
 * Author: Iain Holmes <iain@openedhand.com>
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
gc_iface_address_get_address (GcIfaceAddress *gc,
			      int            *timestamp,
			      GHashTable    **address,
			      int            *accuracy_level,
			      double         *horizontal_accuracy,
			      double         *vertical_accuracy,
			      GError        **error);
#include "gc-iface-address-glue.h"

static void
gc_iface_address_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	signals[ADDRESS_CHANGED] = g_signal_new ("status-changed",
						 G_OBJECT_CLASS_TYPE (klass),
						 G_SIGNAL_RUN_LAST, 0,
						 NULL, NULL,
						 geoclue_marshal_VOID__INT_POINTER_INT_DOUBLE_DOUBLE,
						 G_TYPE_NONE, 5,
						 G_TYPE_INT, G_TYPE_POINTER, 
						 G_TYPE_INT,
						 G_TYPE_DOUBLE, G_TYPE_DOUBLE);
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
gc_iface_address_get_address (GcIfaceAddress *gc,
			      int            *timestamp,
			      GHashTable    **address,
			      int            *accuracy_level,
			      double         *horizontal_accuracy,
			      double         *vertical_accuracy,
			      GError        **error)
{
	return GC_IFACE_ADDRESS_GET_CLASS (gc)->get_address 
		(gc, timestamp, address,
		 (GeoclueAccuracy *) accuracy_level, 
		 horizontal_accuracy, vertical_accuracy, error);
}
