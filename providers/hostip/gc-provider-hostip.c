/*
 * Geoclue
 * gc-provider-hostip.c - A hostip.info-based Address/Position provider
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 */

#include <config.h>

#include <time.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>

#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#include "gc-provider-hostip.h"

#define GC_DBUS_SERVICE_HOSTIP "org.freedesktop.Geoclue.Providers.Hostip"
#define GC_DBUS_PATH_HOSTIP "/org/freedesktop/Geoclue/Providers/Hostip"

#define HOSTIP_URL "http://api.hostip.info/"

#define HOSTIP_NS_GML_NAME "gml"
#define HOSTIP_NS_GML_URI "http://www.opengis.net/gml"
#define HOSTIP_NS_HOSTIP_NAME "hostip"
#define HOSTIP_NS_HOSTIP_URI "http://www.hostip.info/api"

#define HOSTIP_COUNTRY_XPATH "//gml:featureMember/hostip:Hostip/hostip:countryName"
#define HOSTIP_COUNTRYCODE_XPATH "//gml:featureMember/hostip:Hostip/hostip:countryAbbrev"
#define HOSTIP_LOCALITY_XPATH "//gml:featureMember/hostip:Hostip/gml:name"
#define HOSTIP_LATLON_XPATH "//gml:featureMember/hostip:Hostip//gml:coordinates"

static void gc_provider_hostip_init (GcProviderHostip *obj);
static void gc_provider_hostip_position_init (GcIfacePositionClass  *iface);
static void gc_provider_hostip_address_init (GcIfaceAddressClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GcProviderHostip, gc_provider_hostip, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                gc_provider_hostip_position_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
                                                gc_provider_hostip_address_init))


/* Geoclue interface implementation */

static gboolean
gc_provider_hostip_get_version (GcIfaceGeoclue  *iface,
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
gc_provider_hostip_get_status (GcIfaceGeoclue  *iface,
                               gboolean        *available,
                               GError         **error)
{
	/* TODO: if connection status info is only in master, how do we do this? */
	g_set_error (error, GEOCLUE_ERROR, 
	             GEOCLUE_ERROR_NOT_IMPLEMENTED, "Not implemented yet");
	return FALSE;
}

static gboolean
gc_provider_hostip_shutdown (GcIfaceGeoclue  *iface,
                             GError         **error)
{
	GcProviderHostip *obj = GC_PROVIDER_HOSTIP (iface);
	g_main_loop_quit (obj->loop);
	return TRUE;
}

/* Position interface implementation */

static gboolean 
gc_provider_hostip_get_position (GcIfacePosition        *iface,
                                 GeocluePositionFields  *fields,
                                 int                    *timestamp,
                                 double                 *latitude,
                                 double                 *longitude,
                                 double                 *altitude,
                                 GeoclueAccuracy       **accuracy,
                                 GError                **error)
{
	GcProviderHostip *obj = (GC_PROVIDER_HOSTIP (iface));
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
gc_provider_hostip_get_address (GcIfaceAddress   *iface,
                                int              *timestamp,
                                GHashTable      **address,
                                GeoclueAccuracy **accuracy,
                                GError          **error)
{
	GcProviderHostip *obj = GC_PROVIDER_HOSTIP (iface);
	gchar *locality = NULL;
	gchar *country = NULL;
	
	if (!gc_web_service_query (obj->web_service, NULL)) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, "Web service query failed");
		return FALSE;
	}
	
	*address = g_hash_table_new(g_str_hash, g_str_equal);
	
	if (gc_web_service_get_string (obj->web_service, 
	                               &locality, HOSTIP_LOCALITY_XPATH)) {
		/* hostip "sctructured data" for the win... */
		if (g_ascii_strcasecmp (locality, "(Unknown city)") == 0) {
			g_free (locality);
			locality = NULL;
		} else {
			
			/* TODO: get the keys from geoclue-types.h */
			g_hash_table_insert (*address, "locality", locality);
		}
	}
	
	if (gc_web_service_get_string (obj->web_service, 
	                               &country, HOSTIP_COUNTRYCODE_XPATH)) {
		
		/* TODO: get the keys from geoclue-types.h */
		g_hash_table_insert (*address, "countrycode", country);
	}
	
	if (gc_web_service_get_string (obj->web_service, 
	                               &country, HOSTIP_COUNTRY_XPATH)) {
		
		/* TODO: get the keys from geoclue-types.h */
		g_hash_table_insert (*address, "country", country);
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
gc_provider_hostip_finalize (GObject *obj)
{
	GcProviderHostip *self = (GcProviderHostip *) obj;
	
	g_object_unref (self->web_service);
	
	((GObjectClass *) gc_provider_hostip_parent_class)->finalize (obj);
}


/* Initialization */

static void
gc_provider_hostip_class_init (GcProviderHostipClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->get_version = gc_provider_hostip_get_version;
	p_class->get_status = gc_provider_hostip_get_status;
	p_class->shutdown = gc_provider_hostip_shutdown;
	
	o_class->finalize = gc_provider_hostip_finalize;
}

static void
gc_provider_hostip_init (GcProviderHostip *obj)
{
	gc_provider_set_details (GC_PROVIDER (obj), 
	                         GC_DBUS_SERVICE_HOSTIP,
	                         GC_DBUS_PATH_HOSTIP);
	
	obj->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (obj->web_service, HOSTIP_URL);
	gc_web_service_add_namespace (obj->web_service,
	                              HOSTIP_NS_GML_NAME, HOSTIP_NS_GML_URI);
	gc_web_service_add_namespace (obj->web_service,
	                              HOSTIP_NS_HOSTIP_NAME, HOSTIP_NS_HOSTIP_URI);
}

static void
gc_provider_hostip_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = gc_provider_hostip_get_position;
}

static void
gc_provider_hostip_address_init (GcIfaceAddressClass  *iface)
{
	iface->get_address = gc_provider_hostip_get_address;
}

int 
main()
{
	g_type_init();
	
	GcProviderHostip *o = g_object_new (GC_TYPE_PROVIDER_HOSTIP, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (o->loop);
	
	g_main_loop_unref (o->loop);
	g_object_unref (o);
	
	return 0;
}
