/*
 * Geoclue
 * geoclue-plazes.c - A plazes.com-based Address/Position provider
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/gc-web-service.h>
#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#define GEOCLUE_DBUS_SERVICE_PLAZES "org.freedesktop.Geoclue.Providers.Plazes"
#define GEOCLUE_DBUS_PATH_PLAZES "/org/freedesktop/Geoclue/Providers/Plazes"
#define PLAZES_URL "http://plazes.com/suggestions.xml"
#define PLAZES_KEY_MAC "mac_address"
#define PLAZES_LAT_XPATH "//plaze/latitude"
#define PLAZES_LON_XPATH "//plaze/longitude"

#define GEOCLUE_TYPE_PLAZES (geoclue_plazes_get_type ())
#define GEOCLUE_PLAZES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_PLAZES, GeocluePlazes))

typedef struct _GeocluePlazes {
	GcProvider parent;
	GMainLoop *loop;
	GcWebService *web_service;
} GeocluePlazes;

typedef struct _GeocluePlazesClass {
	GcProviderClass parent_class;
} GeocluePlazesClass;


static void geoclue_plazes_init (GeocluePlazes *plazes);
static void geoclue_plazes_position_init (GcIfacePositionClass  *iface);
static void geoclue_plazes_address_init (GcIfaceAddressClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GeocluePlazes, geoclue_plazes, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                geoclue_plazes_position_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
                                                geoclue_plazes_address_init))


/* Geoclue interface implementation */
static gboolean
geoclue_plazes_get_status (GcIfaceGeoclue *iface,
			   GeoclueStatus  *status,
			   GError        **error)
{
	/* Assume available so long as all the requirements are satisfied
	   ie: Network is available */
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeocluePlazes *plazes = GEOCLUE_PLAZES (provider);
	g_main_loop_quit (plazes->loop);
}

#define MAC_LEN 18

static char *
get_mac_address ()
	{
	/* this is an ugly hack, but it seems there is no easy 
	 * ioctl-based way to get the mac address of the router. This 
	 * implementation expects the system to have netstat, grep and awk 
	 * */
	
	FILE *in;
	char mac[MAC_LEN];
	int i;
	
	/*for some reason netstat or /proc/net/arp isn't always ready 
	 * when a connection is already up... Try a couple of times */
	for (i=0; i<10; i++) {
		if (!(in = popen ("ROUTER_IP=`netstat -rn | grep '^0.0.0.0 ' | awk '{ print $2 }'` && grep \"^$ROUTER_IP \" /proc/net/arp | awk '{print $4}'", "r"))) {
			g_warning ("popen failed");
			return NULL;
		}
		
		if (!(fgets (mac, MAC_LEN, in))) {
			if (errno != ENOENT && errno != EAGAIN) {
				g_debug ("error %d", errno);
				return NULL;
			}
			/* try again */
			pclose (in);
			g_debug ("trying again...");
			usleep (200);
			continue;
		}
		pclose (in);
		return g_strdup (mac);
	}
	return NULL;
}




/* Position interface implementation */

static gboolean 
geoclue_plazes_get_position (GcIfacePosition        *iface,
                             GeocluePositionFields  *fields,
                             int                    *timestamp,
                             double                 *latitude,
                             double                 *longitude,
                             double                 *altitude,
                             GeoclueAccuracy       **accuracy,
                             GError                **error)
{
	GeocluePlazes *plazes;
	char *mac;
	
	plazes = (GEOCLUE_PLAZES (iface));
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (timestamp) {
		*timestamp = time (NULL);
	}
	
	mac = get_mac_address ();
	if (mac == NULL) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Router mac address query failed");
		/* TODO: set status == error ? */
		return FALSE;
	}
	
	if (!gc_web_service_query (plazes->web_service, error,
	                           PLAZES_KEY_MAC, mac, 
	                           (char *)0)) {
		g_free (mac);
		return FALSE;
	}
	
	if (latitude && gc_web_service_get_double (plazes->web_service, 
	                                           latitude, PLAZES_LAT_XPATH)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	}
	if (longitude && gc_web_service_get_double (plazes->web_service, 
	                                            longitude, PLAZES_LON_XPATH)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
	}
	
	if (accuracy) {
		if (*fields == GEOCLUE_POSITION_FIELDS_NONE) {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
							  0, 0);
		} else {
			/* Educated guess. Plazes are typically hand pointed on 
			 * a map, or geocoded from address, so should be fairly 
			 * accurate */
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_STREET,
							  0, 0);
		}
	}
	
	return TRUE;
}

/* Address interface implementation */

static gboolean 
geoclue_plazes_get_address (GcIfaceAddress   *iface,
                            int              *timestamp,
                            GHashTable      **address,
                            GeoclueAccuracy **accuracy,
                            GError          **error)
{
	
	GeocluePlazes *plazes = GEOCLUE_PLAZES (iface);
	char *mac;
	GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;
	
	if (timestamp) {
		*timestamp = time (NULL);
	}
	
	mac = get_mac_address ();
	if (mac == NULL) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Router mac address query failed");
		/* TODO: set status == error ? */
		return FALSE;
	}
	
	if (!gc_web_service_query (plazes->web_service, error,
	                           PLAZES_KEY_MAC, mac, 
	                           (char *)0)) {
		g_free (mac);
		return FALSE;
	}
	
	if (address) {
		char *str;
		
		*address = geoclue_address_details_new ();
		
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/country")) {
			g_hash_table_insert (*address, 
			                     g_strdup (GEOCLUE_ADDRESS_KEY_COUNTRY),
			                     str);
			level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/country_code")) {
			g_hash_table_insert (*address, 
			                     g_strdup (GEOCLUE_ADDRESS_KEY_COUNTRYCODE),
			                     str);
			level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/city")) {
			g_hash_table_insert (*address, 
			                     g_strdup (GEOCLUE_ADDRESS_KEY_LOCALITY),
			                     str);
			level = GEOCLUE_ACCURACY_LEVEL_LOCALITY;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/zip_code")) {
			g_hash_table_insert (*address, 
			                     g_strdup (GEOCLUE_ADDRESS_KEY_POSTALCODE),
			                     str);
			level = GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/address")) {
			g_hash_table_insert (*address, 
			                     g_strdup (GEOCLUE_ADDRESS_KEY_STREET),
			                     str);
			level = GEOCLUE_ACCURACY_LEVEL_STREET;
		}
	}
	
	if (accuracy) {
		*accuracy = geoclue_accuracy_new (level, 0, 0);
	}
	
	return TRUE;
}

static void
geoclue_plazes_finalize (GObject *obj)
{
	GeocluePlazes *plazes = GEOCLUE_PLAZES (obj);
	
	g_object_unref (plazes->web_service);
	
	((GObjectClass *) geoclue_plazes_parent_class)->finalize (obj);
}


/* Initialization */

static void
geoclue_plazes_class_init (GeocluePlazesClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_plazes_get_status;
	
	o_class->finalize = geoclue_plazes_finalize;
}

static void
geoclue_plazes_init (GeocluePlazes *plazes)
{
	gc_provider_set_details (GC_PROVIDER (plazes), 
	                         GEOCLUE_DBUS_SERVICE_PLAZES,
	                         GEOCLUE_DBUS_PATH_PLAZES,
	                         "Plazes", "Plazes.com based provider, uses gateway mac address to locate");
	
	plazes->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (plazes->web_service, PLAZES_URL);
}

static void
geoclue_plazes_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = geoclue_plazes_get_position;
}

static void
geoclue_plazes_address_init (GcIfaceAddressClass  *iface)
{
	iface->get_address = geoclue_plazes_get_address;
}

int 
main()
{
	g_type_init();
	
	GeocluePlazes *o = g_object_new (GEOCLUE_TYPE_PLAZES, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (o->loop);
	
	g_main_loop_unref (o->loop);
	g_object_unref (o);
	
	return 0;
}
