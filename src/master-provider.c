/*
 * Geoclue
 * master-provider.c - Provider object for master and master client
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * 
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 *                2008 OpenedHand Ltd
 */

/**
 *  Provider object for GcMaster. Takes care of cacheing 
 *  queried data.
 * 
 *  Should probably start/stop the actual providers as needed
 *  in the future
 *  
 *  Cache could also be used to save "stale" data for situations when 
 *  current data is not available (MasterClient api would have to 
 *  have a "allowOldData" setting)
 * 
 * TODO: 
 * 	figure out what to do if get_* returns GEOCLUE_ERROR_NOT_AVAILABLE.
 * 	Should try again, but when?
 * 
 * 	implement velocity
 * 
 * 	implement other (non-updating) ifaces
 **/

#include <string.h>

#include "main.h"
#include "master-provider.h"
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-address.h>
#include <geoclue/geoclue-marshal.h>

typedef enum _GeoclueProvideFlags {
	GEOCLUE_PROVIDE_NONE = 0,
	GEOCLUE_PROVIDE_UPDATES = 1 << 0,			/* will send *-changed signals */
	GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION = 1 << 1,	/* data can be queried on new connection, and cached until connection ends */
} GeoclueProvideFlags;

typedef struct _GcPositionCache {
	int timestamp;
	GeocluePositionFields fields;
	double latitude;
	double longitude;
	double altitude;
	GeoclueAccuracy *accuracy;
	GError *error;
} GcPositionCache;

typedef struct _GcAddressCache {
	int timestamp;
	GHashTable *details;
	GeoclueAccuracy *accuracy;
	GError *error;
} GcAddressCache;

typedef struct _GcMasterProviderPrivate {
	char *name;
	char *description;
	
	char *service;
	char *path;
	GcInterfaceFlags interfaces;
	
	GList *position_clients; /* list of clients currently using this provider */
	GList *address_clients;
	
	GeoclueAccuracyLevel expected_accuracy;
	
	GeoclueResourceFlags required_resources;
	GeoclueProvideFlags provides;
	
	GeoclueStatus master_status; /* net_status and status affect this */
	GeoclueNetworkStatus net_status;
	
	GeoclueProvider *geoclue;
	GeoclueStatus status; /* cached status from actual provider */
	
	GeocluePosition *position;
	GcPositionCache position_cache;
	
	GeoclueAddress *address;
	GcAddressCache address_cache;
	
} GcMasterProviderPrivate;

enum {
	STATUS_CHANGED,
	ACCURACY_CHANGED,
	POSITION_CHANGED,
	ADDRESS_CHANGED,
	LAST_SIGNAL
};
static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GC_TYPE_MASTER_PROVIDER, GcMasterProviderPrivate))

G_DEFINE_TYPE (GcMasterProvider, gc_master_provider, G_TYPE_OBJECT)

static void
copy_error (GError **target, GError *source)
{
	if (*target) {
		g_error_free (*target);
		*target = NULL;
	}
	if (source) {
		*target = g_error_copy (source);
	}
}

static gboolean
gc_master_provider_is_running (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return (priv->geoclue != NULL);
}

static void 
gc_master_provider_handle_new_position_accuracy (GcMasterProvider *provider,
                                                 GeoclueAccuracy  *accuracy)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	GeoclueAccuracyLevel old_level;
	GeoclueAccuracyLevel new_level = GEOCLUE_ACCURACY_LEVEL_NONE;
	double new_hor_acc, new_vert_acc;
	
	geoclue_accuracy_get_details (priv->position_cache.accuracy,
	                              &old_level, NULL, NULL);
	if (accuracy) {
		geoclue_accuracy_get_details (accuracy,
		                              &new_level, &new_hor_acc, &new_vert_acc);
	}
	geoclue_accuracy_set_details (priv->position_cache.accuracy,
	                              new_level, new_hor_acc, new_vert_acc);
	
	if (old_level != new_level) {
		g_signal_emit (provider, signals[ACCURACY_CHANGED], 0,
		               GC_IFACE_POSITION, new_level);
	}
}

static void 
gc_master_provider_handle_new_address_accuracy (GcMasterProvider *provider,
                                                GeoclueAccuracy  *accuracy)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	GeoclueAccuracyLevel old_level;
	GeoclueAccuracyLevel new_level = GEOCLUE_ACCURACY_LEVEL_NONE;
	double new_hor_acc, new_vert_acc;
	
	geoclue_accuracy_get_details (priv->address_cache.accuracy,
	                              &old_level, NULL, NULL);
	if (accuracy) {
		geoclue_accuracy_get_details (accuracy,
		                              &new_level, &new_hor_acc, &new_vert_acc);
	}
	geoclue_accuracy_set_details (priv->address_cache.accuracy,
	                              new_level, new_hor_acc, new_vert_acc);
	
	if (old_level != new_level) {
		g_signal_emit (provider, signals[ACCURACY_CHANGED], 0,
		               GC_IFACE_ADDRESS, new_level);
	}
}

static void
gc_master_provider_set_position (GcMasterProvider      *provider,
                                 GeocluePositionFields  fields,
                                 int                    timestamp,
                                 double                 latitude,
                                 double                 longitude,
                                 double                 altitude,
                                 GeoclueAccuracy       *accuracy,
                                 GError                *error)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	priv->position_cache.timestamp = timestamp;
	priv->position_cache.fields = fields;
	priv->position_cache.latitude = latitude;
	priv->position_cache.longitude = longitude;
	priv->position_cache.altitude = altitude;
	
	copy_error (&priv->position_cache.error, error);
	
	/* emit accuracy-changed if needed, so masterclient can re-choose providers 
	 * before we emit position-changed */
	gc_master_provider_handle_new_position_accuracy (provider, accuracy);
	
	if (!error) {
		g_signal_emit (provider, signals[POSITION_CHANGED], 0, 
		               fields, timestamp, 
		               latitude, longitude, altitude, 
		               priv->position_cache.accuracy);
	}
}

static void
gc_master_provider_set_address (GcMasterProvider *provider,
                                int               timestamp,
                                GHashTable       *details,
                                GeoclueAccuracy  *accuracy,
                                GError           *error)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	priv->address_cache.timestamp = timestamp;
	
	g_hash_table_destroy (priv->address_cache.details);
	if (details) {
		priv->address_cache.details = geoclue_address_details_copy (details);
	}else {
		priv->address_cache.details = geoclue_address_details_new ();
	}
	copy_error (&priv->address_cache.error, error);
	
	/* emit accuracy-changed if needed, so masterclient can re-choose providers 
	 * before we emit position-changed */
	gc_master_provider_handle_new_address_accuracy (provider, accuracy);
	
	if (!error) {
		g_signal_emit (provider, signals[ADDRESS_CHANGED], 0, 
		               priv->address_cache.timestamp, 
		               priv->address_cache.details, 
		               priv->address_cache.accuracy);
	}
}



static GeoclueResourceFlags
parse_resource_strings (char **flags)
{
	GeoclueResourceFlags resources = GEOCLUE_RESOURCE_NONE;
	int i;
	
	for (i = 0; flags[i]; i++) {
		if (strcmp (flags[i], "RequiresNetwork") == 0) {
			resources |= GEOCLUE_RESOURCE_NETWORK;
		} else if (strcmp (flags[i], "RequiresCell") == 0) {
			resources |= GEOCLUE_RESOURCE_CELL;
		} else if (strcmp (flags[i], "RequiresGPS") == 0) {
			resources |= GEOCLUE_RESOURCE_GPS;
		}
	}
	
	return resources;
}

static GeoclueProvideFlags
parse_provide_strings (char **flags)
{
	GeoclueProvideFlags provides = GEOCLUE_PROVIDE_NONE;
	int i;
	
	for (i = 0; flags[i]; i++) {
		if (strcmp (flags[i], "ProvidesUpdates") == 0) {
			provides |= GEOCLUE_PROVIDE_UPDATES;
		} else if (strcmp (flags[i], "ProvidesCacheableOnConnection") == 0) {
			provides |= GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION;
		}
	}
	
	return provides;
}

static GcInterfaceFlags
parse_interface_strings (char **strs)
{
	GcInterfaceFlags ifaces = GC_IFACE_GEOCLUE;
	int i;
	
	for (i = 0; strs[i]; i++) {
		if (strcmp (strs[i], GEOCLUE_POSITION_INTERFACE_NAME) == 0) {
			ifaces |= GC_IFACE_POSITION;
		} else if (strcmp (strs[i], GEOCLUE_ADDRESS_INTERFACE_NAME) == 0) {
			ifaces |= GC_IFACE_ADDRESS;
		}
	}
	return ifaces;
}

static GeoclueAccuracyLevel
parse_accuracy_string (char *str)
{
	GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;
	if (!str || strcmp (str, "None") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_NONE;
	} else if (strcmp (str, "Country") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	} else if (strcmp (str, "Region") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_REGION;
	} else if (strcmp (str, "Locality") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_LOCALITY;
	} else if (strcmp (str, "Postalcode") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
	} else if  (strcmp (str, "Street") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_STREET;
	} else if  (strcmp (str, "Detailed") == 0) {
		level = GEOCLUE_ACCURACY_LEVEL_DETAILED;
	} else {
		g_warning ("'%s' is not a recognised accuracy level value", str);
	} 
	return level;
}

static void
gc_master_provider_handle_error (GcMasterProvider *provider, GError *error)
{
	GcMasterProviderPrivate *priv;
	
	g_assert (error);
	
	priv = GET_PRIVATE (provider);
	g_debug ("%s handling error %d", priv->name, error->code);
	
	/* web service providers that are unavailable */
	if (priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION && 
	    error->code == GEOCLUE_ERROR_NOT_AVAILABLE) {
		priv->master_status = GEOCLUE_STATUS_UNAVAILABLE;
		/* TODO set timer to re-check availability */
	}
}

/* Sets master_status based on provider status and net_status
 * Should be called whenever priv->status or priv->net_status change */
static void
gc_master_provider_handle_status_change (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	GeoclueStatus new_master_status;
	
	/* calculate new master status */
	if (priv->required_resources & GEOCLUE_RESOURCE_NETWORK ||
	    priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION) {
		switch (priv->net_status) {
			case GEOCLUE_CONNECTIVITY_UNKNOWN:
				/* falling through */
			case GEOCLUE_CONNECTIVITY_OFFLINE:
				new_master_status = GEOCLUE_STATUS_UNAVAILABLE;
				break;
			case GEOCLUE_CONNECTIVITY_ACQUIRING:
				if (priv->status == GEOCLUE_STATUS_AVAILABLE){
					new_master_status = GEOCLUE_STATUS_ACQUIRING;
				} else {
					new_master_status = priv->status;
				}
				break;
			case GEOCLUE_CONNECTIVITY_ONLINE:
				new_master_status = priv->status;
				break;
			default:
				g_assert_not_reached ();
		}
		
	} else {
		new_master_status = priv->status;
	}
	
	if (new_master_status != priv->master_status) {
		priv->master_status = new_master_status;
		
		g_signal_emit (provider, signals[STATUS_CHANGED], 0, new_master_status);
	}
}


static void 
gc_master_provider_update_cache (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	
	if ((!(priv->provides & GEOCLUE_PROVIDE_UPDATES)) ||
	    (!priv->geoclue)) {
		/* non-cacheable provider or provider not running */
		return;
	}
	
	g_debug ("%s: Updating cache ", priv->name);
	priv->master_status = GEOCLUE_STATUS_ACQUIRING;
	g_signal_emit (provider, signals[STATUS_CHANGED], 0, priv->master_status);
	
	if (priv->position) {
		int timestamp;
		double lat, lon, alt;
		GeocluePositionFields fields;
		GeoclueAccuracy *accuracy = NULL;
		GError *error = NULL;
		
		fields = geoclue_position_get_position (priv->position,
							&timestamp,
							&lat, &lon, &alt,
							&accuracy, 
							&error);
		if (error){
			g_warning ("Error updating position cache: %s", error->message);
			gc_master_provider_handle_error (provider, error);
		}
		gc_master_provider_set_position (provider,
		                                 fields, timestamp,
		                                 lat, lon, alt,
		                                 accuracy, error);
	}
	
	if (priv->address) {
		int timestamp;
		GHashTable *details = NULL;
		GeoclueAccuracy *accuracy = NULL;
		GError *error = NULL;
		
		if (!geoclue_address_get_address (priv->address,
		                                  &timestamp,
		                                  &details,
		                                  &accuracy,
		                                  &error)) {
			g_warning ("Error updating address cache: %s", error->message);
			gc_master_provider_handle_error (provider, error);
		}
		gc_master_provider_set_address (provider,
		                                timestamp,
		                                details,
		                                accuracy,
		                                error);
	}
	
	gc_master_provider_handle_status_change (provider);
}

/* signal handlers for the actual providers signals */

static void
provider_status_changed (GeoclueProvider  *provider,
                         GeoclueStatus     status,
                         GcMasterProvider *master_provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (master_provider);
	
	priv->status = status;
	gc_master_provider_handle_status_change (master_provider);
}

static void
position_changed (GeocluePosition      *position,
                  GeocluePositionFields fields,
                  int                   timestamp,
                  double                latitude,
                  double                longitude,
                  double                altitude,
                  GeoclueAccuracy      *accuracy,
                  GcMasterProvider     *provider)
{
	/* is there a situation when we'd need to check against cache 
	 * if data has really changed? probably not */
	gc_master_provider_set_position (provider,
	                                 fields, timestamp,
	                                 latitude, longitude, altitude,
	                                 accuracy, NULL);
}

static void
address_changed (GeoclueAddress   *address,
                 int               timestamp,
                 GHashTable       *details,
                 GeoclueAccuracy  *accuracy,
                 GcMasterProvider *provider)
{
	/* is there a situation when we'd need to check against cache 
	 * if data has really changed? probably not */
	gc_master_provider_set_address (provider,
	                                timestamp,
	                                details,
                                        accuracy,
                                        NULL);
}


static void
finalize (GObject *object)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (object);
	
	geoclue_accuracy_free (priv->position_cache.accuracy);
	geoclue_accuracy_free (priv->address_cache.accuracy);
	if (priv->position_cache.error) {
		g_error_free (priv->position_cache.error);
	}
	if (priv->address_cache.error) {
		g_error_free (priv->address_cache.error);
	}
	
	g_free (priv->name);
	g_free (priv->description);
	g_free (priv->service);
	g_free (priv->path);
	
	g_free (priv->position_clients);
	g_free (priv->address_clients);
	
	G_OBJECT_CLASS (gc_master_provider_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (object);
	
	priv->geoclue = NULL;
	
	if (priv->position) {
		g_object_unref (priv->position);
		priv->position = NULL;
	}
	
	if (priv->address) {
		g_object_unref (priv->address);
		priv->address = NULL;
	}
	if (priv->address_cache.details) {
		g_hash_table_destroy (priv->address_cache.details);
		priv->address_cache.details = NULL;
	}
	
	G_OBJECT_CLASS (gc_master_provider_parent_class)->dispose (object);
}

static void
gc_master_provider_class_init (GcMasterProviderClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;
	
	g_type_class_add_private (klass, sizeof (GcMasterProviderPrivate));
	
	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GcMasterProviderClass, status_changed), 
						  NULL, NULL,
						  g_cclosure_marshal_VOID__INT,
						  G_TYPE_NONE, 1,
						  G_TYPE_INT);
	signals[ACCURACY_CHANGED] = g_signal_new ("accuracy-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GcMasterProviderClass, accuracy_changed), 
						  NULL, NULL,
						  geoclue_marshal_VOID__INT_INT,
						  G_TYPE_NONE, 2,
						  G_TYPE_INT, G_TYPE_INT);
	signals[POSITION_CHANGED] = g_signal_new ("position-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GcMasterProviderClass, position_changed), 
						  NULL, NULL,
						  geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE_BOXED,
						  G_TYPE_NONE, 6,
						  G_TYPE_INT, G_TYPE_INT,
						  G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE,
						  G_TYPE_POINTER);
	signals[ADDRESS_CHANGED] = g_signal_new ("address-changed",
						 G_TYPE_FROM_CLASS (klass),
						 G_SIGNAL_RUN_FIRST |
						 G_SIGNAL_NO_RECURSE,
						 G_STRUCT_OFFSET (GcMasterProviderClass, address_changed), 
						 NULL, NULL,
						 geoclue_marshal_VOID__INT_BOXED_BOXED,
						 G_TYPE_NONE, 3,
						 G_TYPE_INT, 
						 G_TYPE_POINTER,
						 G_TYPE_POINTER);
}

static void
gc_master_provider_init (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	priv->position_clients = NULL;
	priv->address_clients = NULL;
	
	priv->master_status = GEOCLUE_STATUS_UNAVAILABLE;
	
	priv->geoclue = NULL;
	
	priv->position = NULL;
	priv->position_cache.accuracy = 
		geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0 ,0);
	priv->position_cache.error = NULL;
	
	priv->address = NULL;
	priv->address_cache.accuracy = 
		geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0 ,0);
	priv->address_cache.details = geoclue_address_details_new ();
	priv->address_cache.error = NULL;
}

#if DEBUG_INFO
static void
gc_master_provider_dump_position (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	GeocluePositionFields fields;
	int time;
	double lat, lon, alt;
	GError *error = NULL;
	
	priv = GET_PRIVATE (provider);
	
	
	g_print ("     Position Information:\n");
	g_print ("     ---------------------\n");
	
	fields = gc_master_provider_get_position (provider,
	                                          &time, 
	                                          &lat, &lon, &alt,
	                                          NULL, &error);
	if (error) {
		g_print ("      Error: %s", error->message);
		g_error_free (error);
		return;
	}
	g_print ("       Timestamp: %d\n", time);
	g_print ("       Latitude: %.2f %s\n", lat,
		 fields & GEOCLUE_POSITION_FIELDS_LATITUDE ? "" : "(not set)");
	g_print ("       Longitude: %.2f %s\n", lon,
		 fields & GEOCLUE_POSITION_FIELDS_LONGITUDE ? "" : "(not set)");
	g_print ("       Altitude: %.2f %s\n", alt,
		 fields & GEOCLUE_POSITION_FIELDS_ALTITUDE ? "" : "(not set)");
	
}

static void
dump_address_key_and_value (char *key, char *value, GHashTable *target)
{
	g_print ("       %s: %s\n", key, value);
}

static void
gc_master_provider_dump_address (GcMasterProvider *provider)
{
	int time;
	GHashTable *details;
	GError *error = NULL;
	
	g_print ("     Address Information:\n");
	g_print ("     --------------------\n");
	if (!gc_master_provider_get_address (provider,
	                                     &time, 
	                                     &details,
	                                     NULL, &error)) {
		g_print ("      Error: %s", error->message);
		g_error_free (error);
		return;
	}
	g_print ("       Timestamp: %d\n", time);
	g_hash_table_foreach (details, (GHFunc)dump_address_key_and_value, NULL);
	
}

static void
gc_master_provider_dump_required_resources (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	g_print ("   Requires\n");
	if (priv->required_resources & GEOCLUE_RESOURCE_GPS) {
		g_print ("      - GPS\n");
	}

	if (priv->required_resources & GEOCLUE_RESOURCE_NETWORK) {
		g_print ("      - Network\n");
	}
}

static void
gc_master_provider_dump_provides (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	g_print ("   Provides\n");
	if (priv->provides & GEOCLUE_PROVIDE_UPDATES) {
		g_print ("      - Updates\n");
	}
	if (priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION) {
		g_print ("      - Cacheable on network connection\n");
	}
}

static void
gc_master_provider_dump_provider_details (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	g_print ("\n   Name - %s\n", priv->name);
	g_print ("   Description - %s\n", priv->description);
	g_print ("   Service - %s\n", priv->service);
	g_print ("   Path - %s\n", priv->path);
	g_print ("   Accuracy level - %d\n", priv->expected_accuracy);
	g_print ("   Provider is currently %srunning, status %d\n", 
	         priv->geoclue ? "" : "not ",
	         priv->master_status);
	gc_master_provider_dump_required_resources (provider);
	gc_master_provider_dump_provides (provider);
	
	
	if (priv->interfaces & GC_IFACE_POSITION) {
		g_print ("   Interface - Position\n");
		gc_master_provider_dump_position (provider);
	}
	if (priv->interfaces & GC_IFACE_ADDRESS) {
		g_print ("   Interface - Address\n");
		gc_master_provider_dump_address (provider);
	}
}
#endif

static gboolean
gc_master_provider_initialize_geoclue (GcMasterProvider *master_provider,
                                       GeoclueProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (master_provider);
	GError *error = NULL;
	
	priv->geoclue = provider;
	
	if (!geoclue_provider_set_options (priv->geoclue,
	                                   geoclue_get_main_options (),
	                                   &error)) {
		g_warning ("Error setting provider options: %s\n", error->message);
		g_error_free (error);
		priv->geoclue = NULL;
		return FALSE;
	}
	
	/* priv->name has been read from .provider-file earlier...
	 * could ask the provider anyway, just to be consistent */
	if (!geoclue_provider_get_provider_info (priv->geoclue, NULL, 
	                                         &priv->description, &error)) {
		g_warning ("Error getting provider info: %s\n", error->message);
		g_error_free (error);
		priv->geoclue = NULL;
		return FALSE;
	}
	
	g_signal_connect (G_OBJECT (priv->geoclue), "status-changed",
			  G_CALLBACK (provider_status_changed), master_provider);
	
	
	if (!geoclue_provider_get_status (priv->geoclue, &priv->status, &error)) {
		g_warning ("Error getting provider status: %s\n", error->message);
		g_error_free (error);
		priv->geoclue = NULL;
		return FALSE;
	}
	return TRUE;
}
static gboolean
gc_master_provider_initialize_interfaces (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	
	g_assert (!priv->geoclue);
	
	if (priv->interfaces <= GC_IFACE_GEOCLUE) {
		g_warning ("No interfaces defined for %s", priv->name);
		return FALSE;
	}
	
	if (priv->interfaces & GC_IFACE_POSITION) {
		g_assert (priv->position == NULL);
		
		priv->position = geoclue_position_new (priv->service, 
		                                       priv->path);
		g_signal_connect (G_OBJECT (priv->position), "position-changed",
		                  G_CALLBACK (position_changed), provider);
		
		if (!priv->geoclue) {
			if (!gc_master_provider_initialize_geoclue (provider, 
			                                            GEOCLUE_PROVIDER (priv->position))) {
				return FALSE;
			}
		}
		
	}
	if (priv->interfaces & GC_IFACE_ADDRESS) {
		g_assert (priv->address == NULL);
		
		priv->address = geoclue_address_new (priv->service, 
		                                     priv->path);
		g_signal_connect (G_OBJECT (priv->address), "address-changed",
		                  G_CALLBACK (address_changed), provider);
		
		if (!priv->geoclue) {
			if (!gc_master_provider_initialize_geoclue (provider, 
			                                            GEOCLUE_PROVIDER (priv->address))) {
				return FALSE;
			}
		}
	}
	
	return TRUE;
}


static gboolean
gc_master_provider_initialize (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	 
	priv = GET_PRIVATE (provider);
	
	if (!gc_master_provider_initialize_interfaces (provider)) {
		return FALSE;
	}
	
	gc_master_provider_update_cache (provider);
#if DEBUG_INFO
	gc_master_provider_dump_provider_details (provider);
#endif
	return TRUE;
}

static void
gc_master_provider_deinitialize (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	if (priv->position) {
		g_object_unref (priv->position);
		priv->position = NULL;
	}
	if (priv->address) {
		g_object_unref (priv->address);
		priv->address = NULL;
	}
	priv->geoclue = NULL;
	g_debug ("deinited %s", priv->name);
}

static void
network_status_changed (gpointer *connectivity, 
                        GeoclueNetworkStatus status, 
                        GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	
	priv->net_status = status;
	/* update connection-cacheable providers */
	if (status == GEOCLUE_CONNECTIVITY_ONLINE &&
	    priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION) {
		/* intialize to fill cache (this will handle status change) */
		if (gc_master_provider_initialize (provider)) {
			gc_master_provider_deinitialize (provider);
		}
	} else {
		gc_master_provider_handle_status_change (provider);
	}
}

/* for updating cache on providers that are not running */
static gboolean
update_cache_and_deinit (GcMasterProvider *provider)
{
	/* fill cache */
	if (gc_master_provider_initialize (provider)) {
		gc_master_provider_deinitialize (provider);
	}
	return FALSE;
}


/* public methods (for GcMaster and GcMasterClient) */

/* Loads provider details from 'filename' */ 
GcMasterProvider *
gc_master_provider_new (const char *filename,
                        GeoclueConnectivity *connectivity)
{
	GcMasterProvider *provider;
	GcMasterProviderPrivate *priv;
	GKeyFile *keyfile;
	GError *error = NULL;
	gboolean ret;
	char *accuracy_str; 
	char **flags, **interfaces;
	
	keyfile = g_key_file_new ();
	ret = g_key_file_load_from_file (keyfile, filename, 
	                                 G_KEY_FILE_NONE, &error);
	if (ret == FALSE) {
		g_warning ("Error loading %s: %s", filename, error->message);
		g_error_free (error);
		g_key_file_free (keyfile);
		return NULL;
	}
	
	provider = g_object_new (GC_TYPE_MASTER_PROVIDER, NULL);
	priv = GET_PRIVATE (provider);
	
	priv->name = g_key_file_get_value (keyfile, "Geoclue Provider",
	                                   "Name", NULL);
	priv->service = g_key_file_get_value (keyfile, "Geoclue Provider",
	                                      "Service", NULL);
	priv->path = g_key_file_get_value (keyfile, "Geoclue Provider",
	                                   "Path", NULL);
	
	accuracy_str = g_key_file_get_value (keyfile, "Geoclue Provider",
	                                     "Accuracy", NULL);
	priv->expected_accuracy = parse_accuracy_string (accuracy_str);
	if (accuracy_str){
		g_free (accuracy_str);
	}
	
	/* set cached accuracies to a default value */
	geoclue_accuracy_set_details (priv->position_cache.accuracy,
	                              priv->expected_accuracy, 0.0, 0.0);
	geoclue_accuracy_set_details (priv->address_cache.accuracy,
	                              priv->expected_accuracy, 0.0, 0.0);

	
	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
	                                    "Requires", NULL, NULL);
	if (flags != NULL) {
		priv->required_resources = parse_resource_strings (flags);
		g_strfreev (flags);
	} else {
		priv->required_resources = GEOCLUE_RESOURCE_NONE;
	}
	
	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
	                                    "Provides", NULL, NULL);
	if (flags != NULL) {
		priv->provides = parse_provide_strings (flags);
		g_strfreev (flags);
	} else {
		priv->provides = GEOCLUE_PROVIDE_NONE;
	}
	
	if (connectivity && 
	    (priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION)) {
		
		/* we have network status events: mark network provider 
		 * with update flag, set the callback and set use_cache */
		priv->provides |= GEOCLUE_PROVIDE_UPDATES;
		
		g_signal_connect (connectivity, 
		                  "status-changed",
		                  G_CALLBACK (network_status_changed), 
		                  provider);
		priv->net_status = geoclue_connectivity_get_status (connectivity);
	}
	
	priv->interfaces = GC_IFACE_GEOCLUE;
	interfaces = g_key_file_get_string_list (keyfile, 
	                                         "Geoclue Provider",
	                                         "Interfaces",
	                                         NULL, NULL);
	if (interfaces) {
		priv->interfaces = parse_interface_strings (interfaces);
		g_strfreev (interfaces);
	}
	
	if (priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION &&
	    priv->net_status == GEOCLUE_CONNECTIVITY_ONLINE) {
		/* do this as idle so we can return without waiting for http queries */
		g_idle_add ((GSourceFunc)update_cache_and_deinit, provider);
	}
	return provider;
}

/* client calls this when it wants to use the provider. 
   Returns true if provider was actually started, and 
   client should assume accuracy has changed. 
   Returns false if provider was not started (it was either already
   running or starting the provider failed). */
gboolean 
gc_master_provider_subscribe (GcMasterProvider *provider, 
                              gpointer          client,
                              GcInterfaceFlags  interface)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	gboolean started = FALSE;
	
	/* decide wether to run initialize or not */
	if (!gc_master_provider_is_running (provider)) {
		if (!(priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION)) {
			started = gc_master_provider_initialize (provider);
		}
	}
	
	/* add subscription */
	if (interface & GC_IFACE_POSITION) {
		if (!g_list_find (priv->position_clients, client)) {
			priv->position_clients = g_list_prepend (priv->position_clients, client);
		}
	}
	if (interface & GC_IFACE_ADDRESS) {
		if (!g_list_find (priv->address_clients, client)) {
			priv->address_clients = g_list_prepend (priv->address_clients, client);
		}
	}
	
	return started;
}

/* client calls this when it does not intend to use the provider */
void
gc_master_provider_unsubscribe (GcMasterProvider *provider,
                                gpointer          client,
                                GcInterfaceFlags  interface)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	if (interface & GC_IFACE_POSITION) {
		priv->position_clients = g_list_remove (priv->position_clients, client);
	}
	if (interface & GC_IFACE_ADDRESS) {
		priv->address_clients = g_list_remove (priv->address_clients, client);
	}
	
	if (!priv->position_clients &&
	    !priv->address_clients) {
		/* no one is using this provider, shutdown... */
		/* not clearing cached accuracies on purpose */
		g_debug ("%s without clients", priv->name);
		
		/* gc_master_provider_deinitialize (provider); */
	}
}


GeocluePositionFields
gc_master_provider_get_position (GcMasterProvider *provider,
                                 int              *timestamp,
                                 double           *latitude,
                                 double           *longitude,
                                 double           *altitude,
                                 GeoclueAccuracy **accuracy,
                                 GError          **error)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	g_assert (priv->position || 
	          priv->provides & GEOCLUE_PROVIDE_CACHEABLE_ON_CONNECTION);
	
	if (priv->provides & GEOCLUE_PROVIDE_UPDATES) {
		if (timestamp != NULL) {
			*timestamp = priv->position_cache.timestamp;
		}
		if (latitude != NULL) {
			*latitude = priv->position_cache.latitude;
		}
		if (longitude != NULL) {
			*longitude = priv->position_cache.longitude;
		}
		if (altitude != NULL) {
			*altitude = priv->position_cache.altitude;
		}
		if (accuracy != NULL) {
			*accuracy = geoclue_accuracy_copy (priv->position_cache.accuracy);
		}
		if (error != NULL) {
			g_assert (!*error);
			copy_error (error, priv->position_cache.error);
		}
		return priv->position_cache.fields;
	} else {
		return geoclue_position_get_position (priv->position,
		                                      timestamp,
		                                      latitude, 
		                                      longitude, 
		                                      altitude,
		                                      accuracy, 
		                                      error);
	}
}

gboolean 
gc_master_provider_get_address (GcMasterProvider  *provider,
                                int               *timestamp,
                                GHashTable       **details,
                                GeoclueAccuracy  **accuracy,
                                GError           **error)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	if (priv->provides & GEOCLUE_PROVIDE_UPDATES) {
		
		if (timestamp != NULL) {
			*timestamp = priv->address_cache.timestamp;
		}
		if (details != NULL) {
			*details = geoclue_address_details_copy (priv->address_cache.details);
		}
		if (accuracy != NULL) {
			*accuracy = geoclue_accuracy_copy (priv->address_cache.accuracy);
		}
		if (error != NULL) {
			g_assert (!*error);
			copy_error (error, priv->address_cache.error);
		}
		return (!priv->address_cache.error);
	} else {
		g_assert (priv->address);
		return geoclue_address_get_address (priv->address,
		                                    timestamp,
		                                    details, 
		                                    accuracy, 
		                                    error);
	}
}

gboolean
gc_master_provider_is_good (GcMasterProvider     *provider,
                            GcInterfaceFlags      iface_type,
                            GeoclueAccuracyLevel  min_accuracy,
                            gboolean              need_update,
                            GeoclueResourceFlags  allowed_resources)
{
	GcMasterProviderPrivate *priv;
	GcInterfaceFlags supported_ifaces;
	GeoclueProvideFlags required_flags = GEOCLUE_PROVIDE_NONE;
	
	priv = GET_PRIVATE (provider);
	
	if (need_update) {
		required_flags |= GEOCLUE_PROVIDE_UPDATES;
	}
	
	supported_ifaces = priv->interfaces;
	
	/* provider must provide all that is required and
	 * cannot require a resource that is not allowed */
	/* TODO: really, we need to change some of those terms... */
	
	return (((supported_ifaces & iface_type) == iface_type) &&
	        ((priv->provides & required_flags) == required_flags) &&
	        (priv->expected_accuracy >= min_accuracy) &&
	        ((priv->required_resources & (~allowed_resources)) == 0));
}

GeoclueStatus 
gc_master_provider_get_status (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->master_status;
}

GeoclueAccuracyLevel 
gc_master_provider_get_accuracy (GcMasterProvider *provider, GcInterfaceFlags iface)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	GeoclueAccuracyLevel acc_level;
	
	switch (iface) {
		case GC_IFACE_POSITION:
			geoclue_accuracy_get_details (priv->position_cache.accuracy,
			                              &acc_level, NULL, NULL);
			break;
		case GC_IFACE_ADDRESS:
			geoclue_accuracy_get_details (priv->address_cache.accuracy,
			                              &acc_level, NULL, NULL);
			break;
		default:
			g_assert_not_reached ();
	}
	return acc_level;
}

/*returns a reference, but is not meant for editing...*/
char * 
gc_master_provider_get_name (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->name;
}
char * 
gc_master_provider_get_description (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->description;
}
char * 
gc_master_provider_get_service (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->service;
}
char * 
gc_master_provider_get_path (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->path;
}

/* GCompareDataFunc for sorting providers by accuracy and required resources */
int
gc_master_provider_compare (GcMasterProvider *a, 
                            GcMasterProvider *b,
                            GcInterfaceAccuracy *iface_min_accuracy)
{
	int diff;
	GeoclueAccuracy *acc_a, *acc_b;
	GeoclueAccuracyLevel level_a, level_b, min_level;
	
	
	GcMasterProviderPrivate *priv_a = GET_PRIVATE (a);
	GcMasterProviderPrivate *priv_b = GET_PRIVATE (b);
	
	/* get the current accuracylevels */
	switch (iface_min_accuracy->interface) {
		case GC_IFACE_POSITION:
			acc_a = priv_a->position_cache.accuracy;
			acc_b = priv_b->position_cache.accuracy;
			break;
		case GC_IFACE_ADDRESS:
			acc_a = priv_a->address_cache.accuracy;
			acc_b = priv_b->address_cache.accuracy;
			break;
		default:
			g_warning("iface: %d", iface_min_accuracy->interface);
			g_assert_not_reached ();
	}
	

	geoclue_accuracy_get_details (acc_a, &level_a, NULL, NULL);
	geoclue_accuracy_get_details (acc_b, &level_b, NULL, NULL);
	min_level = iface_min_accuracy->accuracy_level;
	
	/* sort by resource requirements and accuracy, but only if both
	 * providers meet the minimum accuracy requirement  */
	if ((level_b >= min_level) &&
	    (level_a >= min_level)) {
		diff = priv_a->required_resources - priv_b->required_resources;
		if (diff != 0 ) {
			return diff;
		}
		return level_b - level_a;
	}
	
	/* one or both do not meet req's, sort by accuracy */
	return level_b - level_a;
}
