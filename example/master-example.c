/*
 * Geoclue
 * master-example.c - Example using the Master client API
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>
#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-address.h>
#include <geoclue/geoclue-position.h>


/* Provider methods */

static void
provider_changed_cb (GeoclueMasterClient *client,
                     char *iface,
                     char *name,
                     char *description, 
                     gpointer userdata)
{
	g_print ("%s provider changed: %s\n", iface, name);
}


/* Address methods */

static void
print_address_key_and_value (char *key, char *value, gpointer user_data)
{
	g_print ("    %s: %s\n", key, value);
}

static void
address_changed_cb (GeoclueAddress  *address,
		    int              timestamp,
		    GHashTable      *details,
		    GeoclueAccuracy *accuracy)
{
	GeoclueAccuracyLevel level;
	geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	g_hash_table_foreach (details, (GHFunc)print_address_key_and_value, NULL);
}

static void
get_address (const char *path)
{
	GError *error = NULL;
	GeoclueAddress *address;
	GHashTable *details = NULL;
	GeoclueAccuracyLevel level;
	GeoclueAccuracy *accuracy = NULL;
	int timestamp = 0;

	address = geoclue_address_new (GEOCLUE_MASTER_DBUS_SERVICE, path);
	
	if (!geoclue_address_get_address (address, &timestamp, 
					  &details, &accuracy, 
					  &error)) {
		g_printerr ("Error getting address: %s\n", error->message);
		g_error_free (error);
		g_object_unref (address);
		return;
	}
	
	geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	g_print ("Current address: (accuracy level %d)\n", level);
	g_hash_table_foreach (details, (GHFunc)print_address_key_and_value, NULL);
	g_hash_table_destroy (details);
	geoclue_accuracy_free (accuracy);

	g_signal_connect (G_OBJECT (address), "address-changed",
			  G_CALLBACK (address_changed_cb), NULL);
}


/* Position methods */

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
		g_print ("Current position: latitude and longitude not valid.\n");
	}
}

static void
get_position (const char *path)
{
	GeocluePosition *position;
	GError *error = NULL;
	GeocluePositionFields fields;
	int timestamp;
	double lat = 0.0, lon = 0.0, alt = 0.0;
	GeoclueAccuracy *accuracy = NULL;

	position = geoclue_position_new (GEOCLUE_MASTER_DBUS_SERVICE, path);
	
	fields = geoclue_position_get_position (position,  &timestamp,
						&lat, &lon, &alt,
						&accuracy, &error);
	if (error != NULL) {
		g_warning ("%s", error->message);
		g_error_free (error);
		g_object_unref (position);
		return;
	}
	
	g_print ("lat - %.2f lon - %.2f alt - %.2f\n", lat, lon, alt);
	
	g_signal_connect (G_OBJECT (position), "position-changed",
			  G_CALLBACK (position_changed_cb), NULL);
}

int
main (int    argc,
      char **argv)
{
	GeoclueMaster *master;
	GeoclueMasterClient *client;
	GError *error = NULL;
	char *path;
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
	                                             GEOCLUE_RESOURCE_NETWORK,
	                                             NULL)){
		g_printerr ("set_requirements failed");
	}
	
	get_address (path);
	
	get_position (path);
	
	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);
	
	return 0;
}
