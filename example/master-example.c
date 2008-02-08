/*
 * Geoclue
 * master-example.c - Example using the Master client API
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-position.h>

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

	g_type_init ();

	/*set the accuracy we want */
	accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY, 0, 0);
	
	master = geoclue_master_get_default ();
	client = geoclue_master_create_client (master, &path, &error);
	if (!geoclue_master_client_set_requirements (client, 
	                                             accuracy,
	                                             0,
	                                             FALSE,
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
	return 0;
}
