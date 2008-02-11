/*
 * Geoclue
 * common-example.c - Example using the Geoclue common client API
 *
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <glib.h>
#include <geoclue/geoclue-common.h>

int main (int argc, char** argv)
{
	gchar *service, *path;
	GeoclueCommon *geoclue = NULL;
	gchar *name = NULL;
	gchar *desc = NULL;
	GeoclueStatus status;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc != 2) {
		g_printerr ("Usage:\n  common-example <provider_name>\n");
		return 1;
	}
	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	
	/* Create new GeoclueCommon */
	geoclue = geoclue_common_new (service, path);
	g_free (service);
	g_free (path);
	if (geoclue == NULL) {
		g_printerr ("Error while creating GeoclueGeocode object.\n");
		return 1;
	}
	
	
	if (!geoclue_common_get_provider_info (geoclue, 
	                                       &name, &desc,
	                                       &error)) {
		g_printerr ("Error getting provider info: %s\n\n", error->message);
		g_error_free (error);
		error = NULL;
	} else {
		g_print ("Provider info:\n");
		g_print ("\tName: %s\n", name);
		g_print ("\tDescription: %s\n\n", desc);
		g_free (name);
		g_free (desc);
	}
	
	if (!geoclue_common_get_status (geoclue, &status, &error)) {
		g_printerr ("Error getting provider info: %s\n\n", error->message);
		g_error_free (error);
		error = NULL;
	} else {
		switch (status) {
                case GEOCLUE_STATUS_ERROR:
                        g_print ("Provider status: error");
                        break;
                case GEOCLUE_STATUS_UNAVAILABLE:
                        g_print ("Provider status: unavailable");
                        break;
                case GEOCLUE_STATUS_ACQUIRING:
                        g_print ("Provider status: acquiring");
                        break;
                case GEOCLUE_STATUS_AVAILABLE:
                        g_print ("Provider status: available");
                        break;
		}
	}

	g_object_unref (geoclue);

	g_free (geoclue);
	return 0;
	
}
