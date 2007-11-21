/*
 * Geoclue
 * gc-iface-position.c - GInterface for org.freedesktop.Geoclue.Position
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <glib.h>

#include <dbus/dbus-glib.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/geoclue-marshal.h>

enum {
	POSITION_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static gboolean 
gc_iface_position_get_position (GcIfacePosition       *position,
				int                   *fields,
				int                   *timestamp,
				double                *latitude,
				double                *longitude,
				double                *altitude,
				int                   *accuracy_level,
				double                *horizontal_accuracy,
				double                *vertical_accuracy,
				GError               **error);

#include "gc-iface-position-glue.h"

static void
gc_iface_position_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}
	initialized = TRUE;
	
	signals[POSITION_CHANGED] = g_signal_new ("position-changed",
						  G_OBJECT_CLASS_TYPE (klass),
						  G_SIGNAL_RUN_LAST, 0,
						  NULL, NULL,
						  geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE_INT_DOUBLE_DOUBLE,
						  G_TYPE_NONE, 8,
						  G_TYPE_INT,
						  G_TYPE_INT,
						  G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE,
						  G_TYPE_INT,
						  G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE);
	dbus_g_object_type_install_info (gc_iface_position_get_type (),
					 &dbus_glib_gc_iface_position_object_info);
}

GType
gc_iface_position_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GcIfacePositionClass),
			gc_iface_position_base_init,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GcIfacePosition", &info, 0);
	}

	return type;
}

static gboolean 
gc_iface_position_get_position (GcIfacePosition *gc,
				int             *fields,
				int             *timestamp,
				double          *latitude,
				double          *longitude,
				double          *altitude,
				int             *accuracy_level,
				double          *horizontal_accuracy,
				double          *vertical_accuracy,
				GError         **error)
{
	return GC_IFACE_POSITION_GET_CLASS (gc)->get_position 
		(gc, (GeocluePositionFields *) fields, timestamp,
		 latitude, longitude, altitude,
		 (GeoclueAccuracy *) accuracy_level,
		 horizontal_accuracy, vertical_accuracy,
		 error);
}

void
gc_iface_position_changed (GcIfacePosition      *gc,
			   GeocluePositionFields fields,
			   int                   timestamp,
			   double                latitude,
			   double                longitude,
			   double                altitude)
{
	g_signal_emit (gc, signals[POSITION_CHANGED], timestamp,
		       latitude, longitude, altitude);
}
