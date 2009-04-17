/*
 * Geoclue
 * position-example.c - Example using the Position client API 
 *                      (asynchronous method call)
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

/* NOTE: provider options are not used in this example  */

#include <glib.h>
#include <geoclue/geoclue-position.h>

static void
position_callback (GeocluePosition      *pos,
		   GeocluePositionFields fields,
		   int                   timestamp,
		   double                latitude,
		   double                longitude,
		   double                altitude,
		   GeoclueAccuracy      *accuracy,
		   GError               *error,
		   gpointer              userdata)
{
	if (error) {
		g_printerr ("Error getting position: %s\n", error->message);
		g_error_free (error);
		g_object_unref (pos);
	} else {
		if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
		    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
			GeoclueAccuracyLevel level;
			
			geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
			g_print ("Current position (accuracy %d):\n", level);
			g_print ("\t%f, %f\n", latitude, longitude);
		} else {
			g_print ("Current position not available.\n");
		}
	}
	g_main_loop_quit ((GMainLoop *)userdata);
}

int main (int argc, char** argv)
{
	gchar *service, *path;
	GMainLoop *mainloop;
	GeocluePosition *pos = NULL;
	
	if (argc < 2 || argc % 2 != 0) {
		g_printerr ("Usage:\n  position-example <provider_name>");
		return 1;
	}
	
	g_type_init();
	mainloop = g_main_loop_new (NULL, FALSE);
	
	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	
	pos = geoclue_position_new (service, path);
	g_free (service);
	g_free (path);
	if (pos == NULL) {
		g_printerr ("Error while creating GeocluePosition object.\n");
		return 1;
	}
	
	geoclue_position_get_position_async (pos, 
	                                     (GeocluePositionCallback) position_callback, 
	                                     mainloop);
	g_print ("Asynchronous call made, going to main loop now...\n");
	g_main_loop_run (mainloop);
	
	g_main_loop_unref (mainloop);
	g_object_unref (pos);
	
	return 0;
}
