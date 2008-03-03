/*
 * Geoclue
 * master-example.c - Example using the Master client API
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>
#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-position.h>

static void
provider_changed_cb (GeoclueMasterClient *client,
                     char *iface,
                     char *name,
                     char *description, 
                     gpointer userdata)
{
	g_print ("%s provider changed: %s (%s)\n", iface, name, description);
}


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


int
main (int    argc,
      char **argv)
{
	GeoclueMaster *master;
	GeoclueMasterClient *client;
	GeocluePosition *position;
	GError *error = NULL;
	char *path;
	GeocluePositionFields fields;
	int timestamp;
	double lat = 0.0, lon = 0.0, alt = 0.0;
	GeoclueAccuracy *accuracy;
	GMainLoop *mainloop;

	g_type_init ();
	
	master = geoclue_master_get_default ();
	client = geoclue_master_create_client (master, &path, &error);

	g_signal_connect (G_OBJECT (client), "provider-changed",
	                  G_CALLBACK (provider_changed_cb), NULL);
	
	if (!geoclue_master_client_set_requirements (client, 
	                                             GEOCLUE_ACCURACY_LEVEL_NONE,
	                                             0,
	                                             FALSE,
	                                             GEOCLUE_RESOURCE_FLAGS_NETWORK,
	                                             NULL)){
		g_printerr ("set_requirements failed");
	}
	
	position = geoclue_position_new (GEOCLUE_MASTER_DBUS_SERVICE, path);
	
	fields = geoclue_position_get_position (position,  &timestamp,
						&lat, &lon, &alt,
						&accuracy, &error);
	if (error != NULL) {
		g_warning ("Error: %s", error->message);
		return 0;
	}
	g_print ("lat - %.2f lon - %.2f alt - %.2f\n", lat, lon, alt);
	
	g_signal_connect (G_OBJECT (position), "position-changed",
			  G_CALLBACK (position_changed_cb), NULL);
	
	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);
	
	return 0;
}
