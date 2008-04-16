/*
 * Geoclue
 * revgeocode-example.c - Example using the ReverseGeocode client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <glib.h>
#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-reverse-geocode.h>

/* GHFunc, use with g_hash_table_foreach */
static void
print_address_key_and_value (char *key, char *value, gpointer user_data)
{
	g_print ("    %s: %s\n", key, value);
}

static GHashTable *
parse_options (int    argc,
               char **argv)
{
        GHashTable *options;
        int i;

        options = g_hash_table_new (g_str_hash, g_str_equal);
        for (i = 2; i < argc; i += 2) {
                g_hash_table_insert (options, argv[i], argv[i + 1]);
        }

        return options;
}

int main (int argc, char** argv)
{
	gchar *service, *path;
	GeoclueReverseGeocode *revgeocoder = NULL;
	GHashTable *address = NULL;
	double lat, lon;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc != 2) {
		g_printerr ("Usage:\n  revgeocode-example <provider_name>\n");
		return 1;
	}
	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	/* Create new GeoclueReverseGeocode */
	revgeocoder = geoclue_reverse_geocode_new (service, path);
	g_free (service);
	g_free (path);
	if (revgeocoder == NULL) {
		g_printerr ("Error while creating GeoclueGeocode object.\n");
		return 1;
	}
	
        /* Set options */
        if (argc > 2) {
                GHashTable *options;
                
                options = parse_options (argc, argv);
                if (!geoclue_provider_set_options (GEOCLUE_PROVIDER (revgeocoder), options, &error)) {
                        g_printerr ("Error setting options: %s\n", 
                                    error->message);
                        g_error_free (error);
                        error = NULL;
                }
                g_hash_table_destroy (options);
        }
		
	address = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                 (GDestroyNotify)g_free, 
	                                 (GDestroyNotify)g_free);
	
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
	g_print ("Reverse Geocoded  [%.2f, %.2f] to address:\n", lat, lon);
	g_hash_table_foreach (address, (GHFunc)print_address_key_and_value, NULL);
	
	g_hash_table_destroy (address);
	g_object_unref (revgeocoder);
	return 0;
	
}
