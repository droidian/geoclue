/*
 * Geoclue
 * geoclue-example.c - Example provider
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>

typedef struct {
	GcProvider parent;

	GMainLoop *loop;
} GeoclueExample;

typedef struct {
	GcProviderClass parent_class;
} GeoclueExampleClass;

#define GEOCLUE_TYPE_EXAMPLE (geoclue_example_get_type ())
#define GEOCLUE_EXAMPLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_EXAMPLE, GeoclueExample))

static void geoclue_example_position_init (GcIfacePositionClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueExample, geoclue_example, GC_TYPE_PROVIDER,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						geoclue_example_position_init))


static gboolean
get_status (GcIfaceGeoclue *gc,
	    GeoclueStatus  *status,
	    GError        **error)
{
	*status = GEOCLUE_STATUS_AVAILABLE;

	return TRUE;
}

static gboolean
shutdown (GcIfaceGeoclue *gc,
	  GError        **error)
{
	GeoclueExample *example = GEOCLUE_EXAMPLE (gc);

	g_main_loop_quit (example->loop);
	return TRUE;
}

static void
geoclue_example_class_init (GeoclueExampleClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *) klass;

	p_class->get_status = get_status;
	p_class->shutdown = shutdown;
}

static void
geoclue_example_init (GeoclueExample *example)
{
	gc_provider_set_details (GC_PROVIDER (example),
				 "org.freedesktop.Geoclue.Providers.Example",
				 "/org/freedesktop/Geoclue/Providers/Example",
				 "Example", "Example provider");
}

static gboolean
get_position (GcIfacePosition       *gc,
	      GeocluePositionFields *fields,
	      int                   *timestamp,
	      double                *latitude,
	      double                *longitude,
	      double                *altitude,
	      GeoclueAccuracy      **accuracy,
	      GError               **error)
{
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0.0, 0.0);
	return TRUE;
}

static void
geoclue_example_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}

static gboolean
emit_position_signal (gpointer data)
{
	GeoclueExample *example = data;
	GeoclueAccuracy *accuracy;

	g_print ("Emitting\n");
	/* FIXME: get the real accuracy */
	accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_DETAILED,
					 12.3, 0.0);
	
	gc_iface_position_emit_position_changed 
		(GC_IFACE_POSITION (example), 
		 GEOCLUE_POSITION_FIELDS_LATITUDE |
		 GEOCLUE_POSITION_FIELDS_LONGITUDE |
		 GEOCLUE_POSITION_FIELDS_ALTITUDE,
		 time (NULL), -5.0, 56.0, 23.5, accuracy);

	geoclue_accuracy_free (accuracy);

	return TRUE;
}

int
main (int    argc,
      char **argv)
{
	GeoclueExample *example;

	g_type_init ();

	example = g_object_new (GEOCLUE_TYPE_EXAMPLE, NULL);

	g_timeout_add (5000, emit_position_signal, example);

	example->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (example->loop);

	return 0;
}
