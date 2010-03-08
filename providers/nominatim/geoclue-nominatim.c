/*
 * Geoclue
 * geoclue-nominatim.c - A nominatim.openstreetmap.org-based "Geocode" and
 *                          "Reverse geocode" provider
 * 
 * Copyright 2010 by Intel Corporation
 *  
 * Author: Jussi Kukkonen <jku@linux.intel.com>
 */

/*
 * The used web service APIs are documented at 
 * http://wiki.openstreetmap.org/wiki/Nominatim
 * 
 */

#include <config.h>

#include <time.h>
#include <string.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-address-details.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-geocode.h>
#include <geoclue/gc-iface-reverse-geocode.h>
#include "geoclue-nominatim.h"


#define GEOCLUE_NOMINATIM_DBUS_SERVICE "org.freedesktop.Geoclue.Providers.Nominatim"
#define GEOCLUE_NOMINATIM_DBUS_PATH "/org/freedesktop/Geoclue/Providers/Nominatim"

#define GEOCODE_URL "http://nominatim.openstreetmap.org/search"
#define REV_GEOCODE_URL "http://nominatim.openstreetmap.org/reverse"

#define NOMINATIM_HOUSE "//reversegeocode/addressparts/house"
#define NOMINATIM_ROAD "//reversegeocode/addressparts/road"
#define NOMINATIM_VILLAGE "//reversegeocode/addressparts/village"
#define NOMINATIM_SUBURB "//reversegeocode/addressparts/suburb"
#define NOMINATIM_CITY "//reversegeocode/addressparts/city"
#define NOMINATIM_POSTCODE "//reversegeocode/addressparts/postcode"
#define NOMINATIM_COUNTY "//reversegeocode/addressparts/county"
#define NOMINATIM_COUNTRY "//reversegeocode/addressparts/country"
#define NOMINATIM_COUNTRYCODE "//reversegeocode/addressparts/country_code"

#define NOMINATIM_LAT "//searchresults/place[1]/@lat"
#define NOMINATIM_LON "//searchresults/place[1]/@lon"
#define NOMINATIM_LATLON_HOUSE "//searchresults/place[1]/house"
#define NOMINATIM_LATLON_ROAD "//searchresults/place[1]/road"
#define NOMINATIM_LATLON_VILLAGE "//searchresults/place[1]/village"
#define NOMINATIM_LATLON_SUBURB "//searchresults/place[1]/suburb"
#define NOMINATIM_LATLON_POSTCODE "//searchresults/place[1]/postcode"
#define NOMINATIM_LATLON_CITY "//searchresults/place[1]/city"
#define NOMINATIM_LATLON_COUNTY "//searchresults/place[1]/county"
#define NOMINATIM_LATLON_COUNTRY "//searchresults/place[1]/country"
#define NOMINATIM_LATLON_COUNTRYCODE "//searchresults/place[1]/countrycode"
 
static void geoclue_nominatim_init (GeoclueNominatim *obj);
static void geoclue_nominatim_geocode_init (GcIfaceGeocodeClass *iface);
static void geoclue_nominatim_reverse_geocode_init (GcIfaceReverseGeocodeClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueNominatim, geoclue_nominatim, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_GEOCODE,
                                                geoclue_nominatim_geocode_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_REVERSE_GEOCODE,
                                                geoclue_nominatim_reverse_geocode_init))


/* Geoclue interface implementation */

static gboolean
geoclue_nominatim_get_status (GcIfaceGeoclue *iface,
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
	GeoclueNominatim *obj = GEOCLUE_NOMINATIM (provider);
	
	g_main_loop_quit (obj->loop);
}


static void
search_string_append (GString *str, const char *val)
{
	if (!val || strlen (val) == 0) {
		return;
	}

	if (str->len != 0) {
		g_string_append (str, ", ");
	}
	g_string_append (str, val);
}

static GeoclueAccuracy*
get_geocode_accuracy (GcWebService *geocoder)
{
	char *str;
	GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;

	if (gc_web_service_get_string (geocoder, &str, NOMINATIM_LATLON_HOUSE)) {
		level = GEOCLUE_ACCURACY_LEVEL_DETAILED;
	} else if (gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_ROAD)) {
		level = GEOCLUE_ACCURACY_LEVEL_STREET;
	} else if (gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_SUBURB) ||
	           gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_POSTCODE) ||
	           gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_VILLAGE)) {
		level = GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
	} else if (gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_CITY)) {
		level = GEOCLUE_ACCURACY_LEVEL_LOCALITY;
	} else if (gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_COUNTY)) {
		level = GEOCLUE_ACCURACY_LEVEL_REGION;
	} else if (gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_COUNTRY) ||
	           gc_web_service_get_string (geocoder, &str,
	                                      NOMINATIM_LATLON_COUNTRYCODE)) {
		level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	}

	return geoclue_accuracy_new (level, 0, 0);
}

/* Geocode interface implementation */
static gboolean
geoclue_nominatim_address_to_position (GcIfaceGeocode        *iface,
                                       GHashTable            *address,
                                       GeocluePositionFields *fields,
                                       double                *latitude,
                                       double                *longitude,
                                       double                *altitude,
                                       GeoclueAccuracy      **accuracy,
                                       GError               **error)
{
	GeoclueNominatim *obj = GEOCLUE_NOMINATIM (iface);
	gchar *country, *region, *locality, *postalcode, *street;
	GString *str;

	country = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRY);
	locality = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY);
	postalcode = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE);
	region = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_REGION);
	street = g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_STREET);

	str = g_string_new ("");
	search_string_append (str, street);
	search_string_append (str, locality);
	search_string_append (str, region);
	search_string_append (str, postalcode);
	search_string_append (str, country);

	if (!gc_web_service_query (obj->geocoder, error,
	                           "q", str->str,
	                           "format", "xml",
	                           "polygon", "0",
	                           "addressdetails", "1",
	                           (char *)0)) {
		g_string_free (str, TRUE);
		return FALSE;
	}
	g_string_free (str, TRUE);

	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (latitude && gc_web_service_get_double (obj->geocoder, 
	                                           latitude, NOMINATIM_LAT)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	}

	if (longitude &&  gc_web_service_get_double (obj->geocoder, 
	                                             longitude, NOMINATIM_LON)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE; 
	}

	if (accuracy) {
		*accuracy = get_geocode_accuracy (obj->geocoder); 
	}

	return TRUE;
}

static gboolean
geoclue_nominatim_freeform_address_to_position (GcIfaceGeocode        *iface,
                                                const char            *address,
                                                GeocluePositionFields *fields,
                                                double                *latitude,
                                                double                *longitude,
                                                double                *altitude,
                                                GeoclueAccuracy      **accuracy,
                                                GError               **error)
{
	GeoclueNominatim *obj = GEOCLUE_NOMINATIM (iface);

	if (!gc_web_service_query (obj->geocoder, error,
	                           "q", address,
	                           "format", "xml",
	                           "polygon", "0",
	                           "addressdetails", "1",
	                           (char *)0)) {
		return FALSE;
	}

	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (latitude && gc_web_service_get_double (obj->geocoder,
	                                           latitude, NOMINATIM_LAT)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	}

	if (longitude &&  gc_web_service_get_double (obj->geocoder,
	                                             longitude, NOMINATIM_LON)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
	}

	if (accuracy) {
		*accuracy = get_geocode_accuracy (obj->geocoder);
	}

	return TRUE;
}

/* ReverseGeocode interface implementation */

static gboolean
geoclue_nominatim_position_to_address (GcIfaceReverseGeocode  *iface,
                                       double                  latitude,
                                       double                  longitude,
                                       GeoclueAccuracy        *position_accuracy,
                                       GHashTable            **address,
                                       GeoclueAccuracy       **address_accuracy,
                                       GError                **error)
{
	GeoclueNominatim *obj = GEOCLUE_NOMINATIM (iface);
	gchar *locality = NULL;
	gchar *region = NULL;
	gchar *country = NULL;
	gchar *countrycode = NULL;
	gchar *area = NULL;
	gchar *street = NULL;
	gchar *postcode = NULL;

	GeoclueAccuracyLevel in_acc = GEOCLUE_ACCURACY_LEVEL_DETAILED;
	gchar lat[G_ASCII_DTOSTR_BUF_SIZE];
	gchar lon[G_ASCII_DTOSTR_BUF_SIZE];

	if (!address) {
		return TRUE;
	}

	g_ascii_dtostr (lat, G_ASCII_DTOSTR_BUF_SIZE, latitude);
	g_ascii_dtostr (lon, G_ASCII_DTOSTR_BUF_SIZE, longitude);
	if (!gc_web_service_query (obj->rev_geocoder, error,
	                           "lat", lat,
	                           "lon", lon,
	                           "format", "xml",
	                           "zoom", "18", /* could set this based on position_accuracy */
                               "addressdetails", "1",
	                           (char *)0)) {
		return FALSE;
	}

	if (position_accuracy) {
		geoclue_accuracy_get_details (position_accuracy, &in_acc, NULL, NULL);
	}

	*address = geoclue_address_details_new ();

	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_COUNTRY && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &countrycode, NOMINATIM_COUNTRYCODE)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_COUNTRYCODE,
		                                countrycode);
		g_free (countrycode);
		geoclue_address_details_set_country_from_code (*address);
	}
	if (!g_hash_table_lookup (*address, GEOCLUE_ADDRESS_KEY_COUNTRY) &&
	    in_acc >= GEOCLUE_ACCURACY_LEVEL_COUNTRY && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &country, NOMINATIM_COUNTRY)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_COUNTRY,
		                                country);
		g_free (country);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_REGION && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &region, NOMINATIM_COUNTY)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_REGION,
		                                region);
		g_free (region);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_LOCALITY && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &locality, NOMINATIM_CITY)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_LOCALITY,
		                                locality);
		g_free (locality);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_POSTALCODE && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &area, NOMINATIM_VILLAGE)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_AREA,
		                                area);
		g_free (area);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_POSTALCODE && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &postcode, NOMINATIM_POSTCODE)) {
		geoclue_address_details_insert (*address,
		                                GEOCLUE_ADDRESS_KEY_POSTALCODE,
		                                postcode);
		g_free (postcode);
	}
	if (in_acc >= GEOCLUE_ACCURACY_LEVEL_STREET && 
	    gc_web_service_get_string (obj->rev_geocoder,
	                               &street, NOMINATIM_ROAD)) {
		char *nr;

		if (gc_web_service_get_string (obj->rev_geocoder,
		                               &nr, NOMINATIM_HOUSE)) {
			char *full_street = g_strdup_printf ("%s %s", street, nr);
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_STREET,
			                                full_street);
			g_free (nr);
			g_free (full_street);
		} else  {
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_STREET,
			                                street);
		}
		g_free (street);
	}

	if (address_accuracy) { 
		GeoclueAccuracyLevel level = geoclue_address_details_get_accuracy_level (*address);
		*address_accuracy = geoclue_accuracy_new (level, 0.0, 0.0);
	}
	return TRUE;
}

static void
geoclue_nominatim_finalize (GObject *obj)
{
	((GObjectClass *) geoclue_nominatim_parent_class)->finalize (obj);
}

static void
geoclue_nominatim_dispose (GObject *obj)
{
	GeoclueNominatim *self = (GeoclueNominatim *) obj;

	if (self->geocoder) {
		g_object_unref (self->geocoder);
		self->geocoder = NULL;
	}

	if (self->rev_geocoder) {
		g_object_unref (self->rev_geocoder);
		self->rev_geocoder = NULL;
	}

	((GObjectClass *) geoclue_nominatim_parent_class)->dispose (obj);
}

/* Initialization */

static void
geoclue_nominatim_class_init (GeoclueNominatimClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_nominatim_get_status;

	o_class->finalize = geoclue_nominatim_finalize;
	o_class->dispose = geoclue_nominatim_dispose;
}

static void
geoclue_nominatim_init (GeoclueNominatim *obj)
{
	gc_provider_set_details (GC_PROVIDER (obj), 
	                         GEOCLUE_NOMINATIM_DBUS_SERVICE,
	                         GEOCLUE_NOMINATIM_DBUS_PATH,
				 "Nominatim", "Nominatim (OpenStreetMap geocoder) provider");

	obj->geocoder = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->geocoder, GEOCODE_URL);
	
	obj->rev_geocoder = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->rev_geocoder, REV_GEOCODE_URL);
}

static void
geoclue_nominatim_geocode_init (GcIfaceGeocodeClass *iface)
{
	iface->address_to_position = geoclue_nominatim_address_to_position;
	iface->freeform_address_to_position = geoclue_nominatim_freeform_address_to_position;
}

static void
geoclue_nominatim_reverse_geocode_init (GcIfaceReverseGeocodeClass *iface)
{
	iface->position_to_address = geoclue_nominatim_position_to_address;
}

int 
main()
{
	GeoclueNominatim *obj;
	
	g_type_init();
	obj = g_object_new (GEOCLUE_TYPE_NOMINATIM, NULL);
	obj->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (obj->loop);
	
	g_main_loop_unref (obj->loop);
	g_object_unref (obj);
	
	return 0;
}
