/*
 * Geoclue
 * geoclue-example.c - Example provider which doesn't do anything.
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

static void
print_option (gpointer key,
              gpointer value,
              gpointer data)
{
	if (G_VALUE_TYPE (value) == G_TYPE_STRING)
		g_print ("   %s - %s\n", key, g_value_get_string (value));
	else
		g_print ("   %s - %d\n", key, g_value_get_int (value));
}

static gboolean
set_options (GcIfaceGeoclue *gc,
             GHashTable     *options,
             GError        **error)
{
        g_print ("Options received---\n");
        g_hash_table_foreach (options, print_option, NULL);
        return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueExample *example = GEOCLUE_EXAMPLE (provider);
	
	g_main_loop_quit (example->loop);
}

static void
geoclue_example_class_init (GeoclueExampleClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *) klass;

	p_class->get_status = get_status;
        p_class->set_options = set_options;
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
	*timestamp = time (NULL);

	/* We're not emitting location details here because we don't want
	   geoclue to accidently use this as a source */
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

	accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
					 0.0, 0.0);
	
	gc_iface_position_emit_position_changed 
		(GC_IFACE_POSITION (example), 
		 GEOCLUE_POSITION_FIELDS_NONE,
		 time (NULL), 0.0, 0.0, 0.0, accuracy);

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
