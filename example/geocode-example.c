/*
 * Geoclue
 * geocode-example.c - Example using the Geocode client API
 *
 * Provider options are not used in this sample. See other files for
 * examples on that.
 * 
 * Author: Jussi Kukkonen <jku@openedhand.com>
 * Copyright 2007, 2008 by Garmin Ltd. or its subsidiaries
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

#include <glib.h>
#include <geoclue/geoclue-geocode.h>

int main (int argc, char** argv)
{
	gchar *service, *path;
	GeoclueGeocode *geocoder = NULL;
	GeocluePositionFields fields;
	GHashTable *address = NULL;
	double lat, lon;
	GeoclueAccuracy *accuracy = NULL;
	GError *error = NULL;
	
	g_type_init();
	
	if (argc < 2 || argc > 3) {
		g_printerr ("Usage:\n  geocode-example <provider_name>\n");
		return 1;
	}
	g_print ("Using provider '%s'\n", argv[1]);
	service = g_strdup_printf ("org.freedesktop.Geoclue.Providers.%s", argv[1]);
	path = g_strdup_printf ("/org/freedesktop/Geoclue/Providers/%s", argv[1]);
	
	/* Address we'd like geocoded */
	address = geoclue_address_details_new();
	g_hash_table_insert (address, g_strdup ("postalcode"), g_strdup ("00330"));
	g_hash_table_insert (address, g_strdup ("countrycode"), g_strdup ("FI"));
	g_hash_table_insert (address, g_strdup ("locality"), g_strdup ("Helsinki"));
	g_hash_table_insert (address, g_strdup ("street"), g_strdup ("Solnantie 24"));
	
	
	/* Create new GeoclueGeocode */
	geocoder = geoclue_geocode_new (service, path);
	g_free (service);
	g_free (path);
	if (geocoder == NULL) {
		g_printerr ("Error while creating GeoclueGeocode object.\n");
		return 1;
	}
	
	
	/* Geocode. We're not interested in altitude 
	   this time, so leave it NULL. */
	fields = geoclue_geocode_address_to_position (geocoder, address, 
	                                              &lat, &lon, NULL, 
	                                              &accuracy, &error);
	if (error) {
		g_printerr ("Error while geocoding: %s\n", error->message);
		g_error_free (error);
		g_hash_table_destroy (address);
		g_object_unref (geocoder);
		
		return 1;
	}
	/* Print out coordinates if they are valid */
	if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
	    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
		
		GeoclueAccuracyLevel level;
		
		geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
		g_print ("Geocoded position (accuracy level %d): \n", level);
		g_print ("\t%f, %f\n", lat, lon);
		
	} else {
		g_print ("Latitude and longitude not available.\n");
	}
	
	g_hash_table_destroy (address);
	geoclue_accuracy_free (accuracy);
	g_object_unref (geocoder);
	return 0;
	
}
