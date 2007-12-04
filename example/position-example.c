/*
 * Geoclue
 * position-example.c - Example using the Position client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 */

#include <glib.h>
#include <geoclue/geoclue-position.h>

int main (int argc, char** argv)
{
	gchar *service, *path;
	GeocluePosition *pos = NULL;
	GeocluePositionFields fields;
	int timestamp;
	double lat, lon;
	GeoclueAccuracy *accuracy = NULL;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc != 2) {
		g_printerr ("Usage:\n  position-example <provider_name>\n");
		return 1;
	}
	g_print ("Using provider '%s'", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	
	/* Create new GeocluePosition */
	pos = geoclue_position_new (service, path);
	g_free (service);
	g_free (path);
	if (pos == NULL) {
		g_printerr ("Error while creating GeocluePosition object.\n");
		return 1;
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
		g_free (pos);
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
	g_free (pos);
	return 0;
	
}
