/*
 * Geoclue
 * geoclue-geonames.c - A geonames.org-based "Geocode" and
 *                          "Reverse geocode" provider
 * 
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
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

/*
 * The used web service APIs are documented at 
 * http://www.geonames.org/export/
 * 
 * Geonames currently does not support street level geocoding. There
 * is a street level reverse geocoder in beta, but it's US only. 
 * http://www.geonames.org/export/reverse-geocoding.html
 */

#include <config.h>

#include <time.h>
#include <dbus/dbus-glib-bindings.h>


#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-address-details.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-geocode.h>
#include <geoclue/gc-iface-reverse-geocode.h>
#include "geoclue-geonames.h"


#define GEOCLUE_GEONAMES_DBUS_SERVICE "org.freedesktop.Geoclue.Providers.Geonames"
#define GEOCLUE_GEONAMES_DBUS_PATH "/org/freedesktop/Geoclue/Providers/Geonames"

#define REV_GEOCODE_STREET_URL "http://ws.geonames.org/findNearestAddress"
#define REV_GEOCODE_PLACE_URL "http://ws.geonames.org/findNearby"
#define GEOCODE_PLACE_URL "http://ws.geonames.org/search"
#define GEOCODE_POSTALCODE_URL "http://ws.geonames.org/postalCodeSearch"

#define POSTALCODE_LAT "//geonames/code/lat"
#define POSTALCODE_LON "//geonames/code/lng"

#define GEONAME_LAT "//geonames/geoname/lat"
#define GEONAME_LON "//geonames/geoname/lng"
#define GEONAME_NAME "//geonames/geoname/name"
#define GEONAME_COUNTRY "//geonames/geoname/countryName"
#define GEONAME_ADMIN1 "//geonames/geoname/adminName1"
#define GEONAME_COUNTRYCODE "//geonames/geoname/countryCode"

#define ADDRESS_STREETNO "//geonames/address/streetNumber"
#define ADDRESS_STREET "//geonames/address/street"
#define ADDRESS_POSTALCODE "//geonames/address/postalcode"
#define ADDRESS_ADMIN2 "//geonames/address/adminName2"
#define ADDRESS_ADMIN1 "//geonames/address/adminName1"
#define ADDRESS_COUNTRY "//geonames/geoname/countryName"
#define ADDRESS_COUNTRYCODE "//geonames/geoname/countryCode"

 
static void geoclue_geonames_init (GeoclueGeonames *obj);
static void geoclue_geonames_geocode_init (GcIfaceGeocodeClass *iface);
static void geoclue_geonames_reverse_geocode_init (GcIfaceReverseGeocodeClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueGeonames, geoclue_geonames, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_GEOCODE,
                                                geoclue_geonames_geocode_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_REVERSE_GEOCODE,
                                                geoclue_geonames_reverse_geocode_init))


/* Geoclue interface implementation */

static gboolean
geoclue_geonames_get_status (GcIfaceGeoclue *iface,
			     GeoclueStatus  *status,
			     GError        **error)
{
	/* Assumption that we are available so long as the 
	   providers requirements are met: ie network is up */
	*status = GEOCLUE_STATUS_AVAILABLE;

	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueGeonames *obj = GEOCLUE_GEONAMES (provider);
	
	g_main_loop_quit (obj->loop);
}


/* Geocode interface implementation */

static gboolean
geoclue_geonames_address_to_position (GcIfaceGeocode        *iface,
                                      GHashTable            *address,
                                      GeocluePositionFields *fields,
                                      double                *latitude,
                                      double                *longitude,
                                      double                *altitude,
                                      GeoclueAccuracy      **accuracy,
                                      GError               **error)
{
	GeoclueGeonames *obj = GEOCLUE_GEONAMES (iface);
	gchar *countrycode, *locality, *postalcode;
	
	countrycode = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE);
	locality = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY);
	postalcode = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE);
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	
	if (countrycode && postalcode) {
		if (!gc_web_service_query (obj->postalcode_geocoder, error,
		                           "postalcode", postalcode,
		                           "country", countrycode,
		                           "maxRows", "1",
		                           "style", "FULL",
		                           (char *)0)) {
			return FALSE;
		}
		if (gc_web_service_get_double (obj->postalcode_geocoder, 
		                               latitude, POSTALCODE_LAT) &&
		    gc_web_service_get_double (obj->postalcode_geocoder, 
		                               longitude, POSTALCODE_LON)) {
			*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE; 
			*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE; 
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_POSTALCODE,
			                                  0, 0);
		}
	} else if (countrycode && locality) {
		if (!gc_web_service_query (obj->place_geocoder, error,
		                           "name", locality,
		                           "country", countrycode,
		                           "maxRows", "1",
		                           "style", "FULL",
		                           (char *)0)) {
			return FALSE;
		}
		if (gc_web_service_get_double (obj->place_geocoder, 
		                               latitude, GEONAME_LAT) &&
		    gc_web_service_get_double (obj->place_geocoder, 
		                               longitude, GEONAME_LON)) {
			*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE; 
			*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE; 
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY,
			                                  0, 0);
		}
	}
	if (*accuracy == NULL) {
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
	}
	return TRUE;
}


/* ReverseGeocode interface implementation */

static gboolean
geoclue_geonames_position_to_address (GcIfaceReverseGeocode  *iface,
                                      double                  latitude,
                                      double                  longitude,
                                      GeoclueAccuracy        *position_accuracy,
                                      GHashTable            **address,
                                      GeoclueAccuracy       **address_accuracy,
                                      GError                **error)
{
	GeoclueGeonames *obj = GEOCLUE_GEONAMES (iface);
	gchar lat[G_ASCII_DTOSTR_BUF_SIZE];
	gchar lon[G_ASCII_DTOSTR_BUF_SIZE];
	gchar *locality = NULL;
	gchar *region = NULL;
	gchar *country = NULL;
	gchar *countrycode = NULL;
	GeoclueAccuracyLevel in_acc = GEOCLUE_ACCURACY_LEVEL_DETAILED;
	
	if (!address) {
		return TRUE;
	}
	g_ascii_dtostr (lat, G_ASCII_DTOSTR_BUF_SIZE, latitude);
	g_ascii_dtostr (lon, G_ASCII_DTOSTR_BUF_SIZE, longitude);
	if (!gc_web_service_query (obj->rev_place_geocoder, error,
	                           "lat", lat,
	                           "lng", lon,
	                           "featureCode","PPL",  /* http://www.geonames.org/export/codes.html*/
	                           "featureCode","PPLA",
	                           "featureCode","PPLC",
	                           "featureCode","PPLG",
	                           "featureCode","PPLL",
	                           "featureCode","PPLR",
	                           "featureCode","PPLS",
	                           "maxRows", "1",
	                           "style", "FULL",
	                           (char *)0)) {
		return FALSE;
	}
	
	if (position_accuracy) {
		geoclue_accuracy_get_details (position_accuracy, &in_acc, NULL, NULL);
	}
	
	*address = g_hash_table_new (g_str_hash, g_str_equal);
	
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_COUNTRY && 
	    gc_web_service_get_string (obj->rev_place_geocoder,
	                               &countrycode, GEONAME_COUNTRYCODE)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_COUNTRYCODE,
		                                countrycode);
		g_free (countrycode);
		geoclue_address_details_set_country_from_code (*address);
	}
	if (!g_hash_table_lookup (*address, GEOCLUE_ADDRESS_KEY_COUNTRY) &&
	    in_acc >= GEOCLUE_ACCURACY_LEVEL_COUNTRY && 
	    gc_web_service_get_string (obj->rev_place_geocoder,
	                               &country, GEONAME_COUNTRY)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_COUNTRY,
		                                country);
		g_free (country);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_REGION && 
	    gc_web_service_get_string (obj->rev_place_geocoder,
	                               &region, GEONAME_ADMIN1)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_REGION,
		                                region);
		g_free (region);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_LOCALITY && 
	    gc_web_service_get_string (obj->rev_place_geocoder,
	                               &locality, GEONAME_NAME)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_LOCALITY,
		                                locality);
		g_free (locality);
	}
	
	if (address_accuracy) { 
		GeoclueAccuracyLevel level = geoclue_address_details_get_accuracy_level (*address);
		*address_accuracy = geoclue_accuracy_new (level, 0.0, 0.0);
	}
	return TRUE;
}

static void
geoclue_geonames_finalize (GObject *obj)
{
	((GObjectClass *) geoclue_geonames_parent_class)->finalize (obj);
}

static void
geoclue_geonames_dispose (GObject *obj)
{
	GeoclueGeonames *self = (GeoclueGeonames *) obj;

	if (self->place_geocoder) {
		g_object_unref (self->place_geocoder);
		self->place_geocoder = NULL;
	}

	if (self->postalcode_geocoder) {
		g_object_unref (self->postalcode_geocoder);
		self->postalcode_geocoder = NULL;
	}

	if (self->rev_place_geocoder) {
		g_object_unref (self->rev_place_geocoder);
		self->rev_place_geocoder = NULL;
	}

	if (self->rev_street_geocoder) {
		g_object_unref (self->rev_street_geocoder);
		self->rev_street_geocoder = NULL;
	}
	
	((GObjectClass *) geoclue_geonames_parent_class)->dispose (obj);
}

/* Initialization */

static void
geoclue_geonames_class_init (GeoclueGeonamesClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_geonames_get_status;

	o_class->finalize = geoclue_geonames_finalize;
	o_class->dispose = geoclue_geonames_dispose;
}

static void
geoclue_geonames_init (GeoclueGeonames *obj)
{
	gc_provider_set_details (GC_PROVIDER (obj), 
	                         GEOCLUE_GEONAMES_DBUS_SERVICE,
	                         GEOCLUE_GEONAMES_DBUS_PATH,
				 "Geonames", "Geonames provider");
	
	obj->place_geocoder = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->place_geocoder, 
	                             GEOCODE_PLACE_URL);
	
	obj->postalcode_geocoder = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->postalcode_geocoder, 
	                             GEOCODE_POSTALCODE_URL);
	
	obj->rev_place_geocoder = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->rev_place_geocoder, 
	                             REV_GEOCODE_PLACE_URL);
	
	obj->rev_street_geocoder = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->rev_street_geocoder, 
	                             REV_GEOCODE_STREET_URL);
}


static void
geoclue_geonames_geocode_init (GcIfaceGeocodeClass *iface)
{
	iface->address_to_position = geoclue_geonames_address_to_position;
}

static void
geoclue_geonames_reverse_geocode_init (GcIfaceReverseGeocodeClass *iface)
{
	iface->position_to_address = geoclue_geonames_position_to_address;
}

int 
main()
{
	GeoclueGeonames *obj;
	
	g_type_init();
	obj = g_object_new (GEOCLUE_TYPE_GEONAMES, NULL);
	obj->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (obj->loop);
	
	g_main_loop_unref (obj->loop);
	g_object_unref (obj);
	
	return 0;
}
