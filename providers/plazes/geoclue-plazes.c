/*
 * Geoclue
 * geoclue-plazes.c - A plazes.com-based Address/Position provider
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
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

#include <config.h>

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/gc-web-service.h>
#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#include "connectivity.h"

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
	GeoclueConnectivity *conn;
	GcWebService *web_service;
	GeoclueStatus last_status;
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
	GeocluePlazes *plazes = GEOCLUE_PLAZES (iface);

	*status = plazes->last_status;
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeocluePlazes *plazes = GEOCLUE_PLAZES (provider);
	g_main_loop_quit (plazes->loop);
}

static void
geoclue_plazes_set_status (GeocluePlazes *self, GeoclueStatus status)
{
	if (status != self->last_status) {
		self->last_status = status;
		gc_iface_geoclue_emit_status_changed (GC_IFACE_GEOCLUE (self), status);
    }
}

static char *
mac_strdown (char *mac)
{
	guint i;
	for (i = 0; mac[i] != '\0' ; i++) {
		if (g_ascii_isalpha (mac[i]))
			mac[i] = g_ascii_tolower (mac[i]);
	}
	return mac;
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
	char *mac, *mac_lc;
	
	plazes = (GEOCLUE_PLAZES (iface));
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (timestamp) {
		*timestamp = time (NULL);
	}

	mac = geoclue_connectivity_get_router_mac (plazes->conn);
	if (mac == NULL) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Router mac address query failed");
		geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_ERROR);
		return FALSE;
	}

	mac = mac_strdown (mac);

	geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_ACQUIRING);

	mac_lc = g_ascii_strdown (mac, -1);
	g_free (mac);
	if (!gc_web_service_query (plazes->web_service, error,
	                           PLAZES_KEY_MAC, mac,
	                           (char *)0)) {
		g_free (mac_lc);
        // did not get a reply; we can try again later
		geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_AVAILABLE);
		g_set_error (error, GEOCLUE_ERROR,
		             GEOCLUE_ERROR_NOT_AVAILABLE,
		             "Did not get reply from server");
		return FALSE;
	}

	g_free (mac);

	if (latitude && gc_web_service_get_double (plazes->web_service, 
	                                           latitude, PLAZES_LAT_XPATH)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	}
	if (longitude && gc_web_service_get_double (plazes->web_service, 
	                                            longitude, PLAZES_LON_XPATH)) {
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
	}
	
	if (accuracy) {
        /* Educated guess. Plazes are typically hand pointed on 
         * a map, or geocoded from address, so should be fairly 
         * accurate */
        *accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_STREET, 0, 0);
	}
	
    if (!(*fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
         *fields & GEOCLUE_POSITION_FIELDS_LONGITUDE)) {

        // we got a reply, but could not exploit it. It would probably be the
        // same next time.
		geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_ERROR);
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Could not understand reply from server");
		return FALSE;
    }
	
	geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_AVAILABLE);

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
	char *mac, *mac_lc;

	GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;
	
	if (timestamp) {
		*timestamp = time (NULL);
	}

	mac = geoclue_connectivity_get_router_mac (plazes->conn);

	if (mac == NULL) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Router mac address query failed");
		geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_ERROR);
		return FALSE;
	}
	
    geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_ACQUIRING);

	mac_lc = g_ascii_strdown (mac, -1);
	g_free (mac);
	if (!gc_web_service_query (plazes->web_service, error,
	                           PLAZES_KEY_MAC, mac_lc, 
	                           (char *)0)) {
		g_free (mac_lc);
		geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_AVAILABLE);
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Did not get reply from server");
		return FALSE;
	}
	g_free (mac_lc);

	if (address) {
		char *str;
		
		*address = geoclue_address_details_new ();
		
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/country")) {
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_COUNTRY,
			                                str);
			g_free (str);
			level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/country_code")) {
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_COUNTRYCODE,
			                                str);
			g_free (str);
			level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/city")) {
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_LOCALITY,
			                                str);
			g_free (str);
			level = GEOCLUE_ACCURACY_LEVEL_LOCALITY;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/zip_code")) {
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_POSTALCODE,
			                                str);
			g_free (str);
			level = GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
		}
		if (gc_web_service_get_string (plazes->web_service, 
		                               &str, "//plaze/address")) {
			geoclue_address_details_insert (*address,
			                                GEOCLUE_ADDRESS_KEY_STREET,
			                                str);
			g_free (str);
			level = GEOCLUE_ACCURACY_LEVEL_STREET;
		}
	}

    if (level == GEOCLUE_ACCURACY_LEVEL_NONE) {
        // we got a reply, but could not exploit it. It would probably be the
        // same next time.
		geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_ERROR);
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Could not understand reply from server");
		return FALSE;
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

	if (plazes->conn != NULL) {
		g_object_unref (plazes->conn);
		plazes->conn = NULL;
	}
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

	plazes->conn = geoclue_connectivity_new ();
	plazes->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (plazes->web_service, PLAZES_URL);
    geoclue_plazes_set_status (plazes, GEOCLUE_STATUS_AVAILABLE);
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
