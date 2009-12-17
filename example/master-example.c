/*
 * Geoclue
 * master-example.c - Example using the Master client API
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
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

/* This example uses geoclue master to get approximate position
 * and address signals. Following params are 
 * given to geoclue_master_client_set_requirements(): 
 * 	min_accuracy = GEOCLUE_ACCURACY_LEVEL_LOCALITY
 * 		Locality means a city or a town. Expect coordinates
 * 		to routinely have 10-20 km error.
 * 	min_time = 0
 * 		No limit on frequency of position-changed signals
 * 		(this is not actually implemented yet)
 * 	require_updates = TRUE
 * 		We want position-changed and address-changed signals
 * 	allowed_resources = GEOCLUE_RESOURCE_NETWORK
 * 		Web services may be used but e.g. GPS is off limits
 * 
 * To ensure at least one working provider for your testing, visit 
 * hostip.info and define your IPs location if it's not set or is 
 * marked as "guessed"
 * 
 * */

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-address.h>
#include <geoclue/geoclue-position.h>

/* Provider methods */
static void
provider_changed_cb (GeoclueMasterClient *client,
                     char *name,
                     char *description, 
                     char *service, 
                     char *path, 
                     gpointer userdata)
{
	g_print ("%s provider changed: %s\n", (char *)userdata, name);
}


/* Address methods */
static void
print_address_key_and_value (char *key, char *value, gpointer user_data)
{
	g_print ("\t%s: %s\n", key, value);
}

static void
address_changed_cb (GeoclueAddress  *address,
		    int              timestamp,
		    GHashTable      *details,
		    GeoclueAccuracy *accuracy)
{
	GeoclueAccuracyLevel level;
	geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	g_print ("New address (accuracy level %d):\n", level);
	g_hash_table_foreach (details, (GHFunc)print_address_key_and_value, NULL);
	g_print ("\n");
}

static GeoclueAddress *
init_address (GeoclueMasterClient *client)
{
	GError *error = NULL;
	GeoclueAddress *address;
	GHashTable *details = NULL;
	GeoclueAccuracyLevel level;
	GeoclueAccuracy *accuracy = NULL;
	int timestamp = 0;
	
	/* create the object and connect to signal */
	address = geoclue_master_client_create_address (client, &error);
	if (!address) {
		g_warning ("Creating GeoclueAddress failed: %s", error->message);
		g_error_free (error);
		return NULL;
	}
	g_signal_connect (G_OBJECT (address), "address-changed",
			  G_CALLBACK (address_changed_cb), NULL);
	
	/* print initial address */
	if (!geoclue_address_get_address (address, &timestamp, 
					  &details, &accuracy, 
					  &error)) {
		g_printerr ("Error getting address: %s\n", error->message);
		g_error_free (error);
	} else {
		geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
		g_print ("Current address: (accuracy level %d):\n", level);
		g_hash_table_foreach (details, (GHFunc)print_address_key_and_value, NULL);
		g_print ("\n");
		g_hash_table_destroy (details);
		geoclue_accuracy_free (accuracy);
	}
	return address;
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
	GeoclueAccuracyLevel level;
	
	geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	g_print ("New position (accuracy level %d):\n", level);
	
	if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
	    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
		g_print ("\t%f, %f\n\n", latitude, longitude);
	} else {
		g_print ("\nlatitude and longitude not valid.\n");
	}
}

static GeocluePosition *
init_position (GeoclueMasterClient *client)
{
	GeocluePosition *position;
	GError *error = NULL;
	GeocluePositionFields fields;
	double lat = 0.0, lon = 0.0;
	GeoclueAccuracy *accuracy;
	
	position = geoclue_master_client_create_position (client, &error);
	if (!position) {
		g_warning ("Creating GeocluePosition failed: %s", error->message);
		g_error_free (error);
		return NULL;
	}
	
	g_signal_connect (G_OBJECT (position), "position-changed",
	                  G_CALLBACK (position_changed_cb), NULL);
	
	/*print initial position */
	fields = geoclue_position_get_position (position, NULL,
	                                        &lat, &lon, NULL,
	                                        &accuracy, &error);
	if (error != NULL) {
		g_warning ("%s", error->message);
		g_error_free (error);
	} else {
		GeoclueAccuracyLevel level;
		
		geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
		g_print ("New position (accuracy level %d):\n", level);
		
		if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
		    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
			g_print ("\t%f, %f\n\n", lat, lon);
		} else {
			g_print ("\nlatitude and longitude not valid.\n");
		}
		
		geoclue_accuracy_free (accuracy);
	}
	
	return position;
}

int
main (int    argc,
      char **argv)
{
	GeoclueMaster *master;
	GeoclueMasterClient *client;
	GeocluePosition *pos;
	GeoclueAddress *addr;
	GMainLoop *mainloop;
	
	g_type_init ();
	
	master = geoclue_master_get_default ();
	client = geoclue_master_create_client (master, NULL, NULL);
	g_object_unref (master);
	
	g_signal_connect (G_OBJECT (client), "address-provider-changed",
	                  G_CALLBACK (provider_changed_cb), "Address");
	g_signal_connect (G_OBJECT (client), "position-provider-changed",
	                  G_CALLBACK (provider_changed_cb), "Position");
	
	if (!geoclue_master_client_set_requirements (client, 
	                                             GEOCLUE_ACCURACY_LEVEL_LOCALITY,
	                                             0,
	                                             TRUE,
	                                             GEOCLUE_RESOURCE_NETWORK,
	                                             NULL)){
		g_printerr ("set_requirements failed");
		g_object_unref (client);
		return 1;
	}
	
	addr = init_address (client);
	pos = init_position (client);
	
	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);
	
	g_main_loop_unref (mainloop);
	g_object_unref (pos);
	g_object_unref (addr);
	g_object_unref (client);
	
	return 0;
}
