/*
 * Geoclue
 * master-example.c - Example using the Master client API
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 *           2008 OpenedHand Ltd
 */


/* This example shows typical GPS-like usage. Following params are 
 * given to geoclue_master_client_set_requirements():
 * 	min_accuracy = GEOCLUE_ACCURACY_LEVEL_DETAILED
 * 		We require the highest level of accuracy
 * 	min_time = 0
 * 		No limit on frequency of position-changed signals
 * 		(this is not actually implemented yet)
 * 	require_updates = TRUE
 * 		We need position-changed signals
 * 	allowed_resources = GEOCLUE_RESOURCE_ALL
 * 		Any available resource can be used
 * 
 * Geoclue master will try to select a suitable provider based on these
 * requirements -- currently only Gypsy and Gpsd providers fulfill 
 * the above requiremens. Gpsd-provider should work out-of-the-box as 
 * long as gpsd is running in the default port. Gypsy provider requires 
 * that you set device name in the options: see README for details.
 * 
 */

#include <string.h>

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-position.h>

static void
provider_changed_cb (GeoclueMasterClient *client,
                     char *iface,
                     char *name,
                     char *description, 
                     gpointer userdata)
{
	if (strlen (name) == 0) {
		g_print ("%s: No provider available\n", iface);
	} else {
		g_print ("%s: now using provider: %s\n", iface, name);
	}
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
		
		geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
		g_print ("got position (accuracy level %d):\n", level);
		g_print ("\t%f, %f\n", latitude, longitude);
		
	} else {
		g_print ("position emitted, but latitude and longitude are not valid.\n");
	}
}

int
main (int    argc,
      char **argv)
{
	GError *error = NULL;
	GMainLoop *mainloop;
	GeoclueMaster *master;
	GeoclueMasterClient *client;
	GeocluePosition *position;
	
	g_type_init ();
	
	master = geoclue_master_get_default ();
	client = geoclue_master_create_client (master, NULL, NULL);
	g_object_unref (master);
	
	g_signal_connect (G_OBJECT (client), "provider-changed",
	                  G_CALLBACK (provider_changed_cb), NULL);
	
	/* We want provider that has detailed accuracy and emits signals.
	 * The provider is allowed to use any resources available. */
	if (!geoclue_master_client_set_requirements (client, 
	                                             GEOCLUE_ACCURACY_LEVEL_DETAILED,
	                                             0, TRUE,
	                                             GEOCLUE_RESOURCE_ALL,
	                                             NULL)){
		g_printerr ("Setting requirements failed");
		g_object_unref (client);
		return 1;
	}
	
	position = geoclue_master_client_create_position (client, &error);
	if (!position) {
		g_warning ("Creating GeocluePosition failed: %s", error->message);
		g_error_free (error);
		g_object_unref (client);
		return 1;
	}
	
	g_signal_connect (G_OBJECT (position), "position-changed",
			  G_CALLBACK (position_changed_cb), NULL);
	
	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);
	
	g_main_loop_unref (mainloop);
	g_object_unref (client);
	g_object_unref (position);
	
	return 0;
}
