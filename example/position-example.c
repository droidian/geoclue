/*
 * Geoclue
 * position-example.c - Example using the Position client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
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
#include <geoclue/geoclue-position.h>

static void
position_changed_cb (GeocluePosition      *position,
		     GeocluePositionFields fields,
		     int                   timestamp,
		     double                latitude,
		     double                longitude,
		     double                altitude,
		     GeoclueAccuracy      *accuracy,
		     gpointer              userdata)
{
	if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
	    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
		
		GeoclueAccuracyLevel level;
		double horiz_acc;
		
		geoclue_accuracy_get_details (accuracy, &level, &horiz_acc, NULL);
		g_print ("Current position:\n");
		g_print ("\t%f, %f\n", latitude, longitude);
		g_print ("\tAccuracy level %d (%.0f meters)\n", level, horiz_acc);
		
	} else {
		g_print ("Latitude and longitude not available.\n");
	}
}

static void
unset_and_free_gvalue (gpointer val)
{
        g_value_unset (val);
        g_free (val);
}

static GHashTable *
parse_options (int    argc,
               char **argv)
{
        GHashTable *options;
        int i;

        options = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         NULL, unset_and_free_gvalue);

        for (i = 2; i < argc; i += 2) {
                GValue *val = g_new0(GValue, 1);
                g_value_init (val, G_TYPE_STRING);
                g_value_set_string(val, argv[i + 1]);
                g_hash_table_insert (options, argv[i], val);
        }

        return options;
}

int main (int argc, char** argv)
{
	gchar *service, *path;
	GeocluePosition *pos = NULL;
	GeocluePositionFields fields;
	int timestamp;
	double lat, lon;
	GeoclueAccuracy *accuracy = NULL;
	GMainLoop *mainloop;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc < 2 || argc % 2 != 0) {
		g_printerr ("Usage:\n  position-example <provider_name> [option value]\n");
		return 1;
	}

	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	mainloop = g_main_loop_new (NULL, FALSE);
	
	/* Create new GeocluePosition */
	pos = geoclue_position_new (service, path);
	if (pos == NULL) {
		g_printerr ("Error while creating GeocluePosition object.\n");
		return 1;
	}

	g_free (service);
	g_free (path);
	
        if (argc > 2) {
                GHashTable *options;

                options = parse_options (argc, argv);
                if (!geoclue_provider_set_options (GEOCLUE_PROVIDER (pos), options, &error)) {
                        g_printerr ("Error setting options: %s\n", 
                                    error->message);
                        g_error_free (error);
                        error = NULL;
                }
                g_hash_table_destroy (options);
        }
	
	/* Query current position. We're not interested in altitude 
	   this time, so leave it NULL. Same can be done with all other
	   arguments that aren't interesting to the client */
	fields = geoclue_position_get_position (pos, &timestamp, 
	                                        &lat, &lon, NULL, 
	                                        &accuracy, &error);
	if (error) {
		g_printerr ("Error getting position: %s\n", error->message);
		g_error_free (error);
		g_object_unref (pos);
		return 1;
	}
	
	/* Print out coordinates if they are valid */
	if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
	    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
		
		GeoclueAccuracyLevel level;
		double horiz_acc;
		
		geoclue_accuracy_get_details (accuracy, &level, &horiz_acc, NULL);
		g_print ("Current position:\n");
		g_print ("\t%f, %f\n", lat, lon);
		g_print ("\tAccuracy level %d (%.0f meters)\n", level, horiz_acc);
		
	} else {
		g_print ("Latitude and longitude not available.\n");
	}

	geoclue_accuracy_free (accuracy);

	g_signal_connect (G_OBJECT (pos), "position-changed",
			  G_CALLBACK (position_changed_cb), NULL);

	g_main_loop_run (mainloop);
	return 0;
	
}
