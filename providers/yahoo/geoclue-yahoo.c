/*
 * Geoclue
 * geoclue-yahoo.c - A "local.yahooapis.com"-based Geocode-provider
 * 
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 */

#include <config.h>

#include <string.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-web-service.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-geocode.h>

#define YAHOO_GEOCLUE_APP_ID "zznSbDjV34HRU5CXQc4D3qE1DzCsJTaKvWTLhNJxbvI_JTp1hIncJ4xTSJFRgjE-"
#define YAHOO_BASE_URL "http://api.local.yahoo.com/MapsService/V1/geocode"
#define GEOCLUE_TYPE_YAHOO (geoclue_yahoo_get_type ())
#define GEOCLUE_YAHOO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_YAHOO, GeoclueYahoo))

typedef struct _GeoclueYahoo {
	GcProvider parent;
	GMainLoop *loop;
	
	GcWebService *web_service;
} GeoclueYahoo;

typedef struct _GeoclueYahooClass {
	GcProviderClass parent_class;
} GeoclueYahooClass;

 
static void geoclue_yahoo_init (GeoclueYahoo *obj);
static void geoclue_yahoo_geocode_init (GcIfaceGeocodeClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueYahoo, geoclue_yahoo, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_GEOCODE,
                                                geoclue_yahoo_geocode_init))


/* Geoclue interface implementation */

static gboolean
geoclue_yahoo_get_status (GcIfaceGeoclue *iface,
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
	GeoclueYahoo *yahoo = GEOCLUE_YAHOO (provider);
	
	g_main_loop_quit (yahoo->loop);
}


static char *
get_address_value (GHashTable *address, char *key)
{
	char *value;
	
	value = g_strdup (g_hash_table_lookup (address, key));
	if (!value) {
		value = g_strdup ("");
	}
	return value;
}
/* Geocode interface implementation */


static gboolean
geoclue_yahoo_address_to_position (GcIfaceGeocode        *iface,
                                   GHashTable            *address,
                                   GeocluePositionFields *fields,
                                   double                *latitude,
                                   double                *longitude,
                                   double                *altitude,
                                   GeoclueAccuracy      **accuracy,
                                   GError               **error)
{
	GeoclueYahoo *yahoo;
	char *street, *postalcode, *locality, *region;
	
	yahoo = GEOCLUE_YAHOO (iface);
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	
	/* weird: the results are all over the globe, but country is not an input parameter... */
	street = get_address_value (address, GEOCLUE_ADDRESS_KEY_STREET);
	postalcode = get_address_value (address, GEOCLUE_ADDRESS_KEY_POSTALCODE);
	locality = get_address_value (address, GEOCLUE_ADDRESS_KEY_LOCALITY);
	region = get_address_value (address, GEOCLUE_ADDRESS_KEY_REGION);
	
	if (!gc_web_service_query (yahoo->web_service,
	                           "appid", YAHOO_GEOCLUE_APP_ID,
	                           "street", street,
	                           "zip", postalcode,
	                           "city", locality,
	                           "state", region,
	                           NULL)) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
		return FALSE;
	}
	
	if (latitude) {
		if (gc_web_service_get_double (yahoo->web_service,
		                               latitude, "//yahoo:Latitude")) {
			*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE; 
		}
	}
	if (longitude) {
		if (gc_web_service_get_double (yahoo->web_service,
		                               longitude, "//yahoo:Longitude")) {
			*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE; 
		}
	}
	
	if (accuracy) {
		char *precision = NULL;
		GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;
		gc_web_service_get_string (yahoo->web_service,
		                           &precision, "//yahoo:Result/attribute::precision");
		if (precision) {
			if ((strcmp (precision, "street") == 0) ||
			    (strcmp (precision, "address") == 0)) {
				level = GEOCLUE_ACCURACY_LEVEL_STREET;
			} else if ((strcmp (precision, "zip") == 0) ||
			           (strcmp (precision, "city") == 0)) {
				level = GEOCLUE_ACCURACY_LEVEL_LOCALITY;
			} else if ((strcmp (precision, "zip+2") == 0) ||
			           (strcmp (precision, "zip+4") == 0)) {
				level = GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
			} else if (strcmp (precision, "state") == 0) {
				level = GEOCLUE_ACCURACY_LEVEL_REGION;
			} else if (strcmp (precision, "country") == 0) {
				level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
			}
			g_free (precision);
		}
		*accuracy = geoclue_accuracy_new (level, 0, 0);
	}
	
	g_free (street);
	g_free (postalcode);
	g_free (locality);
	g_free (region);
	
	return TRUE;
}


static void
geoclue_yahoo_dispose (GObject *obj)
{
	GeoclueYahoo *yahoo = (GeoclueYahoo *) obj;
	
	if (yahoo->web_service) {
		g_object_unref (yahoo->web_service);
		yahoo->web_service = NULL;
	}
	
	((GObjectClass *) geoclue_yahoo_parent_class)->dispose (obj);
}

/* Initialization */

static void
geoclue_yahoo_class_init (GeoclueYahooClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_yahoo_get_status;
	
	o_class->dispose = geoclue_yahoo_dispose;
}

static void
geoclue_yahoo_init (GeoclueYahoo *yahoo)
{
	gc_provider_set_details (GC_PROVIDER (yahoo), 
	                         "org.freedesktop.Geoclue.Providers.Yahoo",
	                         "/org/freedesktop/Geoclue/Providers/Yahoo",
	                         "Yahoo", "Geocode provider that uses the Yahoo! Maps web services API");
	
	yahoo->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (yahoo->web_service, YAHOO_BASE_URL);
	gc_web_service_add_namespace (yahoo->web_service, "yahoo", "urn:yahoo:maps");
}


static void
geoclue_yahoo_geocode_init (GcIfaceGeocodeClass *iface)
{
	iface->address_to_position = geoclue_yahoo_address_to_position;
}

int 
main()
{
	GeoclueYahoo *yahoo;
	
	g_type_init();
	yahoo = g_object_new (GEOCLUE_TYPE_YAHOO, NULL);
	yahoo->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (yahoo->loop);
	
	g_main_loop_unref (yahoo->loop);
	g_object_unref (yahoo);
	
	return 0;
}
