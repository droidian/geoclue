/*
 * Geoclue
 * revgeocode-example.c - Example using the ReverseGeocode client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <glib.h>
#include <geoclue/geoclue-reverse-geocode.h>

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
	GeoclueReverseGeocode *revgeocoder = NULL;
	GHashTable *address = NULL;
	GString *address_str= NULL;
	double lat, lon;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc != 2) {
		g_printerr ("Usage:\n  revgeocode-example <provider_name>\n");
		return 1;
	}
	g_print ("Using provider '%s'", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	address = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                 (GDestroyNotify)g_free, 
	                                 (GDestroyNotify)g_free);
	
	/* Create new GeoclueReverseGeocode */
	revgeocoder = geoclue_reverse_geocode_new (service, path);
	g_free (service);
	g_free (path);
	if (revgeocoder == NULL) {
		g_printerr ("Error while creating GeoclueGeocode object.\n");
		return 1;
	}
	
	lat = 60.2;
	lon = 24.9;
	if (!geoclue_reverse_geocode_position_to_address (revgeocoder, 
	                                                  lat, lon, 
	                                                  &address, &error)) {
		g_printerr ("Error while reverse geocoding: %s\n", error->message);
		g_error_free (error);
		g_free (revgeocoder);
		return 1;
	}
	
	/* Print out the address */
	address_str = g_string_new (NULL);
	g_hash_table_foreach (address, add_to_string, address_str);
	if (address_str->len == 0) {
		g_print ("Address not available for (%.4f, %.4f).\n", lat, lon);
	} else {
		g_print ("Reverse geocoded address for (%.4f, %.4f):\n", lat, lon);
		g_print (address_str->str);
	}
	
	g_hash_table_destroy (address);
	g_string_free (address_str, TRUE);
	g_free (revgeocoder);
	return 0;
	
}
