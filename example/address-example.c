/*
 * Geoclue
 * address-example.c - Example using the Address client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <glib.h>
#include <geoclue/geoclue-address.h>

/* GHFunc, use with g_hash_table_foreach */
static void
add_to_string (gpointer key, gpointer value, gpointer user_data)
{
	GString *str = (GString *)user_data;
	g_string_append_printf (str, "\t%s = %s\n", key, value);
}


int main (int argc, char** argv)
{
	gchar *service, *path;
	GeoclueAddress *address = NULL;
	int timestamp;
	GHashTable *details = NULL;
	GeoclueAccuracy *accuracy = NULL;
	GString *address_str = NULL;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc != 2) {
		g_printerr ("Usage:\n  address-example <provider_name>\n");
		return 1;
	}
	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	
	/* Create new GeoclueAddress */
	address = geoclue_address_new (service, path);
	g_free (service);
	g_free (path);
	if (address == NULL) {
		g_printerr ("Error while creating GeoclueAddress object.\n");
		return 1;
	}
	
	
	/* Query current address */
	if (!geoclue_address_get_address (address, &timestamp, 
	                                      &details, &accuracy, 
	                                      &error)) {
		g_printerr ("Error getting address: %s\n", error->message);
		g_error_free (error);
		g_free (address);
		return 1;
	}
	
	/* address data is in GHashTable details, need to turn that into a string */
	address_str = g_string_new (NULL);
	g_hash_table_foreach (details, add_to_string, address_str);
	
	/* print current address */
	if (address_str->len == 0) {
		g_print ("Current address not available\n");
	} else {
		GeoclueAccuracyLevel level;
		double horiz_acc;
		
		geoclue_accuracy_get_details (accuracy, &level, &horiz_acc, NULL);
		
		g_print ("Current address:\n");
		g_print (address_str->str);
		g_print ("\n\tAccuracy level %d (%.0f meters)\n", level, horiz_acc);
	}
	
	g_hash_table_destroy (details);
	g_string_free (address_str, TRUE);
	geoclue_accuracy_free (accuracy);
	g_free (address);
	
	return 0;
}
