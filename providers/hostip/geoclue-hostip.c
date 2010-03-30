/*
 * Geoclue
 * geoclue-hostip.c - A hostip.info-based Address/Position provider
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
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

#define HOSTIP_COUNTRY_XPATH "//gml:featureMember/Hostip/countryName"
#define HOSTIP_COUNTRYCODE_XPATH "//gml:featureMember/Hostip/countryAbbrev"
#define HOSTIP_LOCALITY_XPATH "//gml:featureMember/Hostip/gml:name"
#define HOSTIP_LATLON_XPATH "//gml:featureMember/Hostip//gml:coordinates"

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
geoclue_hostip_get_status (GcIfaceGeoclue *iface,
			   GeoclueStatus  *status,
			   GError        **error)
{
	/* Assume it is available so long as all the requirements are satisfied
	   ie: Network is available */
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueHostip *obj = GEOCLUE_HOSTIP (provider);
	g_main_loop_quit (obj->loop);
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
	
	if (!gc_web_service_query (obj->web_service, error, (char *)0)) {
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
		                                  0, 0);
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
	
	if (!gc_web_service_query (obj->web_service, error, (char *)0)) {
		return FALSE;
	}
	
	if (address) {
		*address = geoclue_address_details_new ();
		if (gc_web_service_get_string (obj->web_service, 
					       &locality, HOSTIP_LOCALITY_XPATH)) {
			/* hostip "sctructured data" for the win... */
			if (g_ascii_strcasecmp (locality, "(Unknown city)") == 0 ||
			    g_ascii_strcasecmp (locality, "(Unknown City?)") == 0) {

				g_free (locality);
				locality = NULL;
			} else {
				geoclue_address_details_insert (*address,
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
				geoclue_address_details_insert (*address,
				                                GEOCLUE_ADDRESS_KEY_COUNTRYCODE,
				                                country_code);
				geoclue_address_details_set_country_from_code (*address);
			}
		}

		if (!g_hash_table_lookup (*address, GEOCLUE_ADDRESS_KEY_COUNTRY) &&
		    gc_web_service_get_string (obj->web_service, 
		                               &country, HOSTIP_COUNTRY_XPATH)) {
			if (g_ascii_strcasecmp (country, "(Unknown Country?)") == 0) {
				g_free (country);
				country = NULL;
			} else {
				geoclue_address_details_insert (*address,
				                                GEOCLUE_ADDRESS_KEY_COUNTRY,
				                                country);
			}
		}
	}

	if (timestamp) {
		*timestamp = time (NULL);
	}

	if (accuracy) {
		if (locality && country) {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY,
							  0, 0);
		} else if (country) {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_COUNTRY,
							  0, 0);
		} else {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
							  0, 0);
		}
	}
	g_free (locality);
	g_free (country);
	g_free (country_code);

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
	
	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_hostip_get_status;
	
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
