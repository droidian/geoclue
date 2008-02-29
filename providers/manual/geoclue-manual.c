/*
 * Geoclue
 * geoclue-manual.c - Manual provider
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
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


static void 
free_address_key_and_value (char *key, char *value, gpointer userdata)
{
	g_free (key);
	g_free (value);
}
static void 
copy_address_key_and_value (char *key, char *value, GHashTable *target)
{
	/*FIXME: copy values?*/
	g_hash_table_insert (target, key, value);
}
static void
free_address_details (GHashTable *details)
{
	if (details) {
		g_hash_table_foreach (details, (GHFunc)free_address_key_and_value, NULL);
		g_hash_table_unref (details);
	}
}
static GHashTable*
copy_address_details (GHashTable *details)
{
	GHashTable *t = g_hash_table_new (g_str_hash, g_str_equal);
	if (details) {
		g_hash_table_foreach (details, (GHFunc)copy_address_key_and_value, t);
	}
	return t;
}

static GeoclueAccuracy*
get_accuracy_for_address (GHashTable *address)
{
	/* TODO: should we define default values for hor/vert accuracy */
	if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_STREET)) {
		return geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_STREET, 0, 0); 
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE)) {
		return geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_POSTALCODE, 0, 0);
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY)) {
		return geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY, 0, 0);
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_REGION)) {
		return geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_REGION, 0, 0); 
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRY) ||
	           g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE)) {
		return geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_COUNTRY, 0, 0); 
	}
	return geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_STREET, 0, 0); 
}

static void geoclue_manual_init_empty (GeoclueManual *manual)
{
	free_address_details (manual->address);
	manual->address = g_hash_table_new (g_str_hash, g_str_equal);
	
	geoclue_accuracy_free (manual->accuracy);
	manual->accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
}


static gboolean
get_status (GcIfaceGeoclue *gc,
	    GeoclueStatus  *status,
	    GError        **error)
{
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static gboolean
shutdown (GcIfaceGeoclue *gc,
	  GError        **error)
{
	GeoclueManual *manual = GEOCLUE_MANUAL (gc);

	g_main_loop_quit (manual->loop);
	return TRUE;
}

gboolean 
validity_ended (GeoclueManual *manual)
{
	manual->event_id = 0;
	geoclue_manual_init_empty (manual);
	
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
	
	free_address_details (manual->address);
	manual->address = address;
	
	geoclue_accuracy_free (manual->accuracy);
	manual->accuracy = get_accuracy_for_address (manual->address);
	
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
	                                   copy_address_details (address));
	
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
	GHashTable *address = g_hash_table_new (g_str_hash, g_str_equal);
	
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
	GeoclueManual *manual = GEOCLUE_MANUAL (object);

	free_address_details (manual->address);
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
	
	geoclue_manual_init_empty (manual);
}

static gboolean
get_address (GcIfaceAddress   *gc,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GeoclueManual *manual = GEOCLUE_MANUAL (gc);
	
	
	if (timestamp) {
		*timestamp = manual->timestamp;
	}
	if (address) {
		*address = copy_address_details (manual->address);
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
