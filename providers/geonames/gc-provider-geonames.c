/*
 * Geoclue
 * gc-provider-geonames.c - A geonames.org-based "Geocode" and
 *                          "Reverse geocode" provider
 * 
 * Copyright (C) 2007 OpenedHand Ltd
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
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
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-geocode.h>
#include <geoclue/gc-iface-reverse-geocode.h>
#include "gc-provider-geonames.h"


#define GC_DBUS_SERVICE_GEONAMES "org.freedesktop.Geoclue.Providers.Geonames"
#define GC_DBUS_PATH_GEONAMES "/org/freedesktop/Geoclue/Providers/Geonames"

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

 
static void gc_provider_geonames_init (GcProviderGeonames *obj);
static void gc_provider_geonames_geocode_init (GcIfaceGeocodeClass *iface);
static void gc_provider_geonames_reverse_geocode_init (GcIfaceReverseGeocodeClass *iface);

G_DEFINE_TYPE_WITH_CODE (GcProviderGeonames, gc_provider_geonames, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_GEOCODE,
                                                gc_provider_geonames_geocode_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_REVERSE_GEOCODE,
                                                gc_provider_geonames_reverse_geocode_init))


/* Geoclue interface implementation */

static gboolean
gc_provider_geonames_get_version (GcIfaceGeoclue  *iface,
                                  int             *major,
                                  int             *minor,
                                  int             *micro,
                                  GError         **error)
{
	*major = 1;
	*minor = 0;
	*micro = 0;
	return TRUE;
}

static gboolean
gc_provider_geonames_get_status (GcIfaceGeoclue  *iface,
                                 gboolean        *available,
                                 GError         **error)
{
	/* TODO: if connection status info is only in master, how do we do this? */
	g_set_error (error, GEOCLUE_ERROR, 
	             GEOCLUE_ERROR_NOT_IMPLEMENTED, "Not implemented yet");
	return FALSE;
}

static gboolean
gc_provider_geonames_shutdown (GcIfaceGeoclue  *iface,
                             GError         **error)
{
	GcProviderGeonames *obj = GC_PROVIDER_GEONAMES (iface);
	
	g_main_loop_quit (obj->loop);
	return TRUE;
}


/* Geocode interface implementation */

static gboolean
gc_provider_geonames_address_to_position (GcIfaceGeocode        *iface,
                                          GHashTable            *address,
                                          GeocluePositionFields *fields,
                                          double                *latitude,
                                          double                *longitude,
                                          double                *altitude,
                                          GeoclueAccuracy      **accuracy,
                                          GError               **error)
{
	GcProviderGeonames *obj = GC_PROVIDER_GEONAMES (iface);
	gchar *countrycode, *locality, *postalcode;
	
	countrycode = g_hash_table_lookup (address, "countrycode");
	locality = g_hash_table_lookup (address, "locality");
	postalcode = g_hash_table_lookup (address, "postalcode");
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	
	if (countrycode && postalcode) {
		if (!gc_web_service_query (obj->postalcode_geocoder,
		                           "postalcode", postalcode,
		                           "country", countrycode,
		                           "maxRows", "1",
		                           "style", "FULL",
		                           NULL)) {
			g_set_error (error, GEOCLUE_ERROR, 
			             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
			return FALSE;
		}
		if (gc_web_service_get_double (obj->postalcode_geocoder, 
		                               latitude, POSTALCODE_LAT) &&
		    gc_web_service_get_double (obj->postalcode_geocoder, 
		                               longitude, POSTALCODE_LON)) {
			*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE; 
			*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE; 
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_POSTALCODE,
			                                  2000, 0);
		} else {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
			                                  0, 0);
		}
	} else if (countrycode && locality) {
		if (!gc_web_service_query (obj->place_geocoder,
		                           "name", locality,
		                           "country", countrycode,
		                           "maxRows", "1",
		                           "style", "FULL",
		                           NULL)) {
			g_set_error (error, GEOCLUE_ERROR, 
			             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
			return FALSE;
		}
		if (gc_web_service_get_double (obj->place_geocoder, 
		                               latitude, GEONAME_LAT) &&
		    gc_web_service_get_double (obj->place_geocoder, 
		                               longitude, GEONAME_LON)) {
			*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE; 
			*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE; 
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY,
			                                  20000, 0);
		} else {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
			                                  0, 0);
		}
	}
	return TRUE;
}


/* ReverseGeocode interface implementation */

static gboolean
gc_provider_geonames_position_to_address (GcIfaceReverseGeocode  *iface,
                                          double                  latitude,
                                          double                  longitude,
                                          GHashTable            **address,
                                          GeoclueAccuracy       **accuracy,
                                          GError                **error)
{
	GcProviderGeonames *obj = GC_PROVIDER_GEONAMES (iface);
	gchar lat[G_ASCII_DTOSTR_BUF_SIZE];
	gchar lon[G_ASCII_DTOSTR_BUF_SIZE];
	gchar *locality = NULL;
	gchar *region = NULL;
	gchar *country = NULL;
	gchar *countrycode = NULL;
	
	g_ascii_dtostr (lat, G_ASCII_DTOSTR_BUF_SIZE, latitude);
	g_ascii_dtostr (lon, G_ASCII_DTOSTR_BUF_SIZE, longitude);
	if (!gc_web_service_query (obj->rev_place_geocoder,
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
	                           NULL)) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
		return FALSE;
	}
	
	*address = g_hash_table_new (g_str_hash, g_str_equal);
	
	if (gc_web_service_get_string (obj->rev_place_geocoder,
	                               &countrycode, GEONAME_COUNTRYCODE)) {
		g_hash_table_insert (*address, "countrycode", countrycode);
	}
	if (gc_web_service_get_string (obj->rev_place_geocoder,
	                               &country, GEONAME_COUNTRY)) {
		g_hash_table_insert (*address, "country", country);
	}
	if (gc_web_service_get_string (obj->rev_place_geocoder,
	                               &region, GEONAME_ADMIN1)) {
		g_hash_table_insert (*address, "region", region);
	}
	if (gc_web_service_get_string (obj->rev_place_geocoder,
	                               &locality, GEONAME_NAME)) {
		g_hash_table_insert (*address, "locality", locality);
	}
	
	/* TODO: accuracy does not make sense here, does it? */
	return TRUE;
}

static void
gc_provider_geonames_finalize (GObject *obj)
{
	GcProviderGeonames *self = (GcProviderGeonames *) obj;
	
	g_object_unref (self->place_geocoder);
	g_object_unref (self->postalcode_geocoder);
	g_object_unref (self->rev_place_geocoder);
	g_object_unref (self->rev_street_geocoder);
	
	((GObjectClass *) gc_provider_geonames_parent_class)->finalize (obj);
}

/* Initialization */

static void
gc_provider_geonames_class_init (GcProviderGeonamesClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->get_version = gc_provider_geonames_get_version;
	p_class->get_status = gc_provider_geonames_get_status;
	p_class->shutdown = gc_provider_geonames_shutdown;
	
	o_class->finalize = gc_provider_geonames_finalize;
}

static void
gc_provider_geonames_init (GcProviderGeonames *obj)
{
	gc_provider_set_details (GC_PROVIDER (obj), 
	                         GC_DBUS_SERVICE_GEONAMES,
	                         GC_DBUS_PATH_GEONAMES);
	
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
gc_provider_geonames_geocode_init (GcIfaceGeocodeClass *iface)
{
	iface->address_to_position = gc_provider_geonames_address_to_position;
}

static void
gc_provider_geonames_reverse_geocode_init (GcIfaceReverseGeocodeClass *iface)
{
	iface->position_to_address = gc_provider_geonames_position_to_address;
}

int 
main()
{
	GcProviderGeonames *obj;
	
	g_type_init();
	obj = g_object_new (GC_TYPE_PROVIDER_GEONAMES, NULL);
	obj->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (obj->loop);
	
	g_main_loop_unref (obj->loop);
	g_object_unref (obj);
	
	return 0;
}
