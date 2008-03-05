/*
 * Geoclue
 * address-example.c - Example using the Address client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <glib.h>
#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-address.h>

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
	GeoclueCommon *common = NULL;
	GeoclueAddress *address = NULL;
	int timestamp;
	GHashTable *details = NULL;
	GeoclueAccuracy *accuracy = NULL;
	GeoclueAccuracyLevel level;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc < 2 || argc % 2 != 0) {
		g_printerr ("Usage:\n  address-example <provider_name> [option value]\n");
		return 1;
	}
	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
        common = geoclue_common_new (service, path);
        if (common == NULL) {
                g_printerr ("Error while creating GeoclueCommon object.\n");
                return 1;
        }

	/* Create new GeoclueAddress */
	address = geoclue_address_new (service, path);
	g_free (service);
	g_free (path);
	if (address == NULL) {
		g_printerr ("Error while creating GeoclueAddress object.\n");
		return 1;
	}
	
        /* Set options */
        if (argc > 2) {
                GHashTable *options;

                options = parse_options (argc, argv);
                if (!geoclue_common_set_options (common, options, &error)) {
                        g_printerr ("Error setting options: %s\n", 
                                    error->message);
                        g_error_free (error);
                        error = NULL;
                }
                g_hash_table_destroy (options);
        }
	
	/* Query current address */
	if (!geoclue_address_get_address (address, &timestamp, 
	                                      &details, &accuracy, 
	                                      &error)) {
		g_printerr ("Error getting address: %s\n", error->message);
		g_error_free (error);
		g_object_unref (address);
		return 1;
	}
	geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	
	/* address data is in GHashTable details, need to turn that into a string */
	g_print ("Current address: (accuracy level %d)\n", level);
	g_hash_table_foreach (details, (GHFunc)print_address_key_and_value, NULL);
	
	g_hash_table_destroy (details);
	geoclue_accuracy_free (accuracy);
	g_object_unref (address);
	
	return 0;
}
