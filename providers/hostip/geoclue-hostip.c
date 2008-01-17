/*
 * Geoclue
 * geoclue-hostip.c - A hostip.info-based Address/Position provider
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <time.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-error.h>

#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#include "geoclue-hostip.h"

#define GEOCLUE_DBUS_SERVICE_HOSTIP "org.freedesktop.Geoclue.Providers.Hostip"
#define GEOCLUE_DBUS_PATH_HOSTIP "/org/freedesktop/Geoclue/Providers/Hostip"

#define HOSTIP_URL "http://api.hostip.info/"

#define HOSTIP_NS_GML_NAME "gml"
#define HOSTIP_NS_GML_URI "http://www.opengis.net/gml"
#define HOSTIP_NS_HOSTIP_NAME "hostip"
#define HOSTIP_NS_HOSTIP_URI "http://www.hostip.info/api"

#define HOSTIP_COUNTRY_XPATH "//gml:featureMember/hostip:Hostip/hostip:countryName"
#define HOSTIP_COUNTRYCODE_XPATH "//gml:featureMember/hostip:Hostip/hostip:countryAbbrev"
#define HOSTIP_LOCALITY_XPATH "//gml:featureMember/hostip:Hostip/gml:name"
#define HOSTIP_LATLON_XPATH "//gml:featureMember/hostip:Hostip//gml:coordinates"

static void geoclue_hostip_init (GeoclueHostip *obj);
static void geoclue_hostip_position_init (GcIfacePositionClass  *iface);
static void geoclue_hostip_address_init (GcIfaceAddressClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueHostip, geoclue_hostip, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                geoclue_hostip_position_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
                                                geoclue_hostip_address_init))


/* Geoclue interface implementation */

static gboolean
geoclue_hostip_get_status (GcIfaceGeoclue  *iface,
                               gboolean        *available,
                               GError         **error)
{
	/* TODO: if connection status info is only in master, how do we do this? */
	g_set_error (error, GEOCLUE_ERROR, 
	             GEOCLUE_ERROR_NOT_IMPLEMENTED, "Not implemented yet");
	return FALSE;
}

static gboolean
geoclue_hostip_shutdown (GcIfaceGeoclue  *iface,
                             GError         **error)
{
	GeoclueHostip *obj = GEOCLUE_HOSTIP (iface);
	g_main_loop_quit (obj->loop);
	return TRUE;
}

/* Position interface implementation */

static gboolean 
geoclue_hostip_get_position (GcIfacePosition        *iface,
                                 GeocluePositionFields  *fields,
                                 int                    *timestamp,
                                 double                 *latitude,
                                 double                 *longitude,
                                 double                 *altitude,
                                 GeoclueAccuracy       **accuracy,
                                 GError                **error)
{
	GeoclueHostip *obj = (GEOCLUE_HOSTIP (iface));
	gchar *coord_str = NULL;
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	
	if (!gc_web_service_query (obj->web_service, NULL)) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
		return FALSE;
	}
	
	
	if (gc_web_service_get_string (obj->web_service, 
	                                &coord_str, HOSTIP_LATLON_XPATH)) {
		if (sscanf (coord_str, "%lf,%lf", longitude , latitude) == 2) {
			*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
			*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
		}
		g_free (coord_str);
	}
	
	time ((time_t *)timestamp);
	
	if (*fields == GEOCLUE_POSITION_FIELDS_NONE) {
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
		                                  0, 0); 
	} else {
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY,
		                                  20000, 0);
	}
	return TRUE;
}

/* Address interface implementation */

static gboolean 
geoclue_hostip_get_address (GcIfaceAddress   *iface,
                                int              *timestamp,
                                GHashTable      **address,
                                GeoclueAccuracy **accuracy,
                                GError          **error)
{
	GeoclueHostip *obj = GEOCLUE_HOSTIP (iface);
	gchar *locality = NULL;
	gchar *country = NULL;
	gchar *country_code = NULL;
	
	if (!gc_web_service_query (obj->web_service, NULL)) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
		return FALSE;
	}
	
	*address = g_hash_table_new(g_str_hash, g_str_equal);
	
	if (gc_web_service_get_string (obj->web_service, 
	                               &locality, HOSTIP_LOCALITY_XPATH)) {
		/* hostip "sctructured data" for the win... */
		if (g_ascii_strcasecmp (locality, "(Unknown city)") == 0 ||
		    g_ascii_strcasecmp (locality, "(Unknown City?)") == 0) {

			g_free (locality);
			locality = NULL;
		} else {
			g_hash_table_insert (*address, 
					     GEOCLUE_ADDRESS_KEY_LOCALITY, 
					     locality);
		}
	}
	
	if (gc_web_service_get_string (obj->web_service, 
	                               &country_code, HOSTIP_COUNTRYCODE_XPATH)) {
		if (g_ascii_strcasecmp (country_code, "XX") == 0) {
			g_free (country_code);
			country_code = NULL;
		} else {
			g_hash_table_insert (*address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE,
			                     country_code);
		}
	}
	
	if (gc_web_service_get_string (obj->web_service, 
	                               &country, HOSTIP_COUNTRY_XPATH)) {
		if (g_ascii_strcasecmp (country, "(Unknown Country?)") == 0) {
			g_free (country);
			country = NULL;
		} else {
			g_hash_table_insert (*address, GEOCLUE_ADDRESS_KEY_COUNTRY, 
			                     country);
		}
	}
	
	time ((time_t *)timestamp);
	
	/* TODO: fix accuracies */
	if (locality && country) {
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY,
	 	                                  20000, 20000);
	} else if (country) {
		/*TODO: maybe a short list of largest countries in world to get this more correct */
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_COUNTRY,
	 	                                  2000000, 2000000);
	} else {
		/* TODO: fix me */
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
		                                  40000000, 40000000);
	}
	return TRUE;
}

static void
geoclue_hostip_finalize (GObject *obj)
{
	GeoclueHostip *self = (GeoclueHostip *) obj;
	
	g_object_unref (self->web_service);
	
	((GObjectClass *) geoclue_hostip_parent_class)->finalize (obj);
}


/* Initialization */

static void
geoclue_hostip_class_init (GeoclueHostipClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->get_status = geoclue_hostip_get_status;
	p_class->shutdown = geoclue_hostip_shutdown;
	
	o_class->finalize = geoclue_hostip_finalize;
}

static void
geoclue_hostip_init (GeoclueHostip *obj)
{
	gc_provider_set_details (GC_PROVIDER (obj), 
	                         GEOCLUE_DBUS_SERVICE_HOSTIP,
	                         GEOCLUE_DBUS_PATH_HOSTIP,
				 "Hostip", "Hostip provider");
	
	obj->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->web_service, HOSTIP_URL);
	gc_web_service_add_namespace (obj->web_service,
	                              HOSTIP_NS_GML_NAME, HOSTIP_NS_GML_URI);
	gc_web_service_add_namespace (obj->web_service,
	                              HOSTIP_NS_HOSTIP_NAME, HOSTIP_NS_HOSTIP_URI);
}

static void
geoclue_hostip_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = geoclue_hostip_get_position;
}

static void
geoclue_hostip_address_init (GcIfaceAddressClass  *iface)
{
	iface->get_address = geoclue_hostip_get_address;
}

int 
main()
{
	g_type_init();
	
	GeoclueHostip *o = g_object_new (GEOCLUE_TYPE_HOSTIP, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (o->loop);
	
	g_main_loop_unref (o->loop);
	g_object_unref (o);
	
	return 0;
}
