/*
 * Geoclue
 * geoclue-manual.c - Manual address provider
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

/** Geoclue manual provider
 *  
 * This is an address provider which gets its address data from user 
 * input. No UI is included, any application may query the address from 
 * the user and submit it to manual provider through the D-Bus API:
 *    org.freedesktop.Geoclue.Manual.SetAddress
 *    org.freedesktop.Geoclue.Manual.SetAddressFields
 * 
 * SetAddress allows setting the current address as a GeoclueAddress, 
 * while SetAddressFields is a convenience version with separate 
 * address fields. Shell example using SetAddressFields:
 * 
 * dbus-send --print-reply --type=method_call \
 *           --dest=org.freedesktop.Geoclue.Providers.Manual \
 *           /org/freedesktop/Geoclue/Providers/Manual \
 *           org.freedesktop.Geoclue.Manual.SetAddressFields \
 *           int32:7200 \
 *           string: \
 *           string:"Finland" \
 *           string: \
 *           string:"Helsinki" \
 *           string: \
 *           string: \
 *           string:"Solnantie 24"
 * 
 * This would make the provider emit a AddressChanged signal with 
 * accuracy level GEOCLUE_ACCURACY_STREET. Unless new SetAddress* calls 
 * are made, provider will emit another signal in two hours (7200 sec), 
 * with empty address and GEOCLUE_ACCURACY_NONE.
 **/

#include <config.h>
#include <string.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-address.h>

typedef struct {
	GcProvider parent;
	
	GMainLoop *loop;
	
	guint event_id;
	
	int timestamp;
	GHashTable *address;
	GeoclueAccuracy *accuracy;
} GeoclueManual;

typedef struct {
	GcProviderClass parent_class;
} GeoclueManualClass;

#define GEOCLUE_TYPE_MANUAL (geoclue_manual_get_type ())
#define GEOCLUE_MANUAL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_MANUAL, GeoclueManual))

static void geoclue_manual_address_init (GcIfaceAddressClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueManual, geoclue_manual, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
                                                geoclue_manual_address_init))

static gboolean
geoclue_manual_set_address (GeoclueManual *manual,
                            int valid_until,
                            GHashTable *address,
                            GError **error);

static gboolean
geoclue_manual_set_address_fields (GeoclueManual *manual,
                                   int valid_until,
                                   char *country_code,
                                   char *country,
                                   char *region,
                                   char *locality,
                                   char *area,
                                   char *postalcode,
                                   char *street,
                                   GError **error);

#include "geoclue-manual-glue.h"


static GeoclueAccuracyLevel
get_accuracy_for_address (GHashTable *address)
{
	if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_STREET)) {
		return GEOCLUE_ACCURACY_LEVEL_STREET;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY)) {
		return GEOCLUE_ACCURACY_LEVEL_LOCALITY;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_REGION)) {
		return GEOCLUE_ACCURACY_LEVEL_REGION;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRY) ||
	           g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	}
	return GEOCLUE_ACCURACY_LEVEL_NONE;
}

static gboolean
get_status (GcIfaceGeoclue *gc,
	    GeoclueStatus  *status,
	    GError        **error)
{
	GeoclueAccuracyLevel level;
  
	geoclue_accuracy_get_details (GEOCLUE_MANUAL (gc)->accuracy,
	                              &level, NULL, NULL);
	if (level == GEOCLUE_ACCURACY_LEVEL_NONE) {
		*status = GEOCLUE_STATUS_UNAVAILABLE;
	} else {
		*status = GEOCLUE_STATUS_AVAILABLE;
	}
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueManual *manual;
	
	manual = GEOCLUE_MANUAL (provider);
	g_main_loop_quit (manual->loop);
}

gboolean 
validity_ended (GeoclueManual *manual)
{
	manual->event_id = 0;
	g_hash_table_remove_all (manual->address);
	geoclue_accuracy_set_details (manual->accuracy,
	                              GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
	
	gc_iface_address_emit_address_changed (GC_IFACE_ADDRESS (manual),
	                                       manual->timestamp,
	                                       manual->address,
	                                       manual->accuracy);
	return FALSE;
}


static void
geoclue_manual_set_address_common (GeoclueManual *manual,
                                   int valid_for,
                                   GHashTable *address)
{
	if (manual->event_id > 0) {
		g_source_remove (manual->event_id);
	}
	
	manual->timestamp = time (NULL);
	
	g_hash_table_destroy (manual->address);
	manual->address = address;
	
	geoclue_accuracy_set_details (manual->accuracy,
	                              get_accuracy_for_address (address),
	                              0, 0);
	
	gc_iface_address_emit_address_changed (GC_IFACE_ADDRESS (manual),
	                                       manual->timestamp,
	                                       manual->address,
	                                       manual->accuracy);
	
	if (valid_for > 0) {
		manual->event_id = g_timeout_add (valid_for * 1000, 
		                                  (GSourceFunc)validity_ended, 
		                                  manual);
	}
}

static gboolean
geoclue_manual_set_address (GeoclueManual *manual,
                            int valid_for,
                            GHashTable *address,
                            GError **error)
{
	geoclue_manual_set_address_common (manual,
	                                   valid_for,
	                                   geoclue_address_details_copy (address));
	return TRUE;
}

static gboolean
geoclue_manual_set_address_fields (GeoclueManual *manual,
                                   int valid_for,
                                   char *country_code,
                                   char *country,
                                   char *region,
                                   char *locality,
                                   char *area,
                                   char *postalcode,
                                   char *street,
                                   GError **error)
{
	GHashTable *address;
	
	address = geoclue_address_details_new ();
	if (country_code && (strlen (country_code) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_COUNTRYCODE), 
		                     g_strdup (country_code));
	}
	if (country && (strlen (country) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_COUNTRY), 
		                     g_strdup (country));
	}
	if (region && (strlen (region) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_REGION), 
		                     g_strdup (region));
	}
	if (locality && (strlen (locality) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_LOCALITY), 
		                     g_strdup (locality));
	}
	if (area && (strlen (area) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_AREA), 
		                     g_strdup (area));
	}
	if (postalcode && (strlen (postalcode) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_POSTALCODE), 
		                     g_strdup (postalcode));
	}
	if (street && (strlen (street) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_STREET), 
		                     g_strdup (street));
	}
	
	geoclue_manual_set_address_common (manual,
	                                   valid_for,
	                                   address);
	return TRUE;
}


static void
finalize (GObject *object)
{
	GeoclueManual *manual;
	
	manual = GEOCLUE_MANUAL (object);
	
	g_hash_table_destroy (manual->address);
	geoclue_accuracy_free (manual->accuracy);
	
	((GObjectClass *) geoclue_manual_parent_class)->finalize (object);
}

static void
geoclue_manual_class_init (GeoclueManualClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	GcProviderClass *p_class = (GcProviderClass *) klass;
	
	o_class->finalize = finalize;
	
	p_class->get_status = get_status;
	p_class->shutdown = shutdown;
	
	dbus_g_object_type_install_info (geoclue_manual_get_type (),
	                                 &dbus_glib_geoclue_manual_object_info);
}

static void
geoclue_manual_init (GeoclueManual *manual)
{
	gc_provider_set_details (GC_PROVIDER (manual),
	                         "org.freedesktop.Geoclue.Providers.Manual",
	                         "/org/freedesktop/Geoclue/Providers/Manual",
	                         "Manual", "Manual provider");
	
	manual->address = geoclue_address_details_new ();
	manual->accuracy = 
		geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
}

static gboolean
get_address (GcIfaceAddress   *gc,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GeoclueManual *manual = GEOCLUE_MANUAL (gc);
	GeoclueAccuracyLevel level;
  
	geoclue_accuracy_get_details (manual->accuracy, &level, NULL, NULL);
	if (level == GEOCLUE_ACCURACY_LEVEL_NONE) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "No manual address set");
		return FALSE;
	}

	if (timestamp) {
		*timestamp = manual->timestamp;
	}
	if (address) {
		*address = geoclue_address_details_copy (manual->address);
	}
	if (accuracy) {
		*accuracy = geoclue_accuracy_copy (manual->accuracy);
	}
	
	return TRUE;
}

static void
geoclue_manual_address_init (GcIfaceAddressClass *iface)
{
	iface->get_address = get_address;
}

int
main (int    argc,
      char **argv)
{
	GeoclueManual *manual;
	
	g_type_init ();
	
	manual = g_object_new (GEOCLUE_TYPE_MANUAL, NULL);
	manual->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (manual->loop);
	
	g_main_loop_unref (manual->loop);
	g_object_unref (manual);
	
	return 0;
}
