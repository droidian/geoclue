/*
 * Geoclue
 * master-provider.c - Provider object for master and master client
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * 
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
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
 * 	implement velocity (maybe this should be somehow related to position)
 * 
 * 	implement other (non-updating) ifaces
 **/

#include <string.h>

#include "main.h"
#include "master-provider.h"
#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-address.h>
#include <geoclue/geoclue-marshal.h>

typedef enum _GeoclueProvideFlags {
	GEOCLUE_PROVIDE_FLAGS_NONE = 0,
	GEOCLUE_PROVIDE_FLAGS_UPDATES = 1 << 0,
} GeoclueProvideFlags;

typedef struct _GcPositionCache {
	/*FIXME: this should probably include GError too ? */
	int timestamp;
	GeocluePositionFields fields;
	double latitude;
	double longitude;
	double altitude;
	GeoclueAccuracy *accuracy;
} GcPositionCache;

typedef struct _GcAddressCache {
	/*FIXME: this should probably have gboolean (return value) too ? */
	int timestamp;
	GHashTable *details;
	GeoclueAccuracy *accuracy;
} GcAddressCache;

typedef struct _GcMasterProviderPrivate {
	char *name;
	char *service;
	char *path;
	GeoclueAccuracyLevel promised_accuracy;
	
	GeoclueResourceFlags required_resources;
	GeoclueProvideFlags provides;
	
	gboolean use_cache;
	GeoclueStatus master_status; /* net_status and status affect this */
	GeoclueNetworkStatus net_status;
	
	GeoclueCommon *geoclue;
	GeoclueStatus status;
	
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
gc_master_provider_handle_new_accuracy (GcMasterProvider *provider,
                                        GeoclueAccuracy  *accuracy)
{
	GeoclueAccuracyLevel old_level, new_level;
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	if (priv->position_cache.accuracy) {
		geoclue_accuracy_get_details (priv->position_cache.accuracy,
		                              &old_level, NULL, NULL);
	} else {
		old_level = priv->promised_accuracy;
	}
	geoclue_accuracy_get_details (accuracy,
	                              &new_level, NULL, NULL);
	if (old_level != new_level) {
		g_signal_emit (provider, signals[ACCURACY_CHANGED], 0,
		               new_level);
	}
}

static void
gc_master_provider_set_position (GcMasterProvider      *provider,
                                 GeocluePositionFields  fields,
                                 int                    timestamp,
                                 double                 latitude,
                                 double                 longitude,
                                 double                 altitude,
                                 GeoclueAccuracy       *accuracy)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	gc_master_provider_handle_new_accuracy (provider, accuracy);
	
	priv->position_cache.timestamp = timestamp;
	priv->position_cache.fields = fields;
	priv->position_cache.latitude = latitude;
	priv->position_cache.longitude = longitude;
	priv->position_cache.altitude = altitude;
	
	geoclue_accuracy_free (priv->position_cache.accuracy);
	priv->position_cache.accuracy = geoclue_accuracy_copy (accuracy);
	
	g_signal_emit (provider, signals[POSITION_CHANGED], 0, 
	               fields, timestamp, 
	               latitude, longitude, altitude, 
	               priv->position_cache.accuracy);
}

static void 
free_address_key_and_value (char *key, char *value, gpointer userdata)
{
	g_free (key);
	g_free (value);
}
static void 
copy_address_key_and_value (char *key, char *value, GHashTable *target)
{
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

static void
gc_master_provider_set_address (GcMasterProvider *provider,
                                int               timestamp,
                                GHashTable       *details,
                                GeoclueAccuracy  *accuracy)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	gc_master_provider_handle_new_accuracy (provider, accuracy);
	
	
	priv->address_cache.timestamp = timestamp;
	
	free_address_details (priv->address_cache.details);
	priv->address_cache.details = copy_address_details (details);
	
	geoclue_accuracy_free (priv->address_cache.accuracy);
	priv->address_cache.accuracy = geoclue_accuracy_copy (accuracy);
	
	g_signal_emit (provider, signals[ADDRESS_CHANGED], 0, 
	               priv->address_cache.timestamp, 
	               priv->address_cache.details, 
	               priv->address_cache.accuracy);
}



static GeoclueResourceFlags
parse_resource_strings (char **flags)
{
	GeoclueResourceFlags resources = GEOCLUE_RESOURCE_FLAGS_NONE;
	int i;
	
	for (i = 0; flags[i]; i++) {
		if (strcmp (flags[i], "RequiresNetwork") == 0) {
			resources |= GEOCLUE_RESOURCE_FLAGS_NETWORK;
		} else if (strcmp (flags[i], "RequiresGPS") == 0) {
			resources |= GEOCLUE_RESOURCE_FLAGS_GPS;
		}
	}
	
	return resources;
}

static GeoclueProvideFlags
parse_provide_strings (char **flags)
{
	GeoclueProvideFlags provides = GEOCLUE_PROVIDE_FLAGS_NONE;
	int i;
	
	for (i = 0; flags[i]; i++) {
		if (strcmp (flags[i], "ProvidesUpdates") == 0) {
			provides |= GEOCLUE_PROVIDE_FLAGS_UPDATES;
		}
	}
	
	return provides;
}

static void 
gc_master_provider_update_cache (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	
	if ((priv->use_cache == FALSE) ||
	    (priv->master_status != GEOCLUE_STATUS_AVAILABLE)) {
		return;
	}
	
	g_debug ("%s: Updating cache ", priv->name);
	
	if (priv->position) {
		int timestamp;
		double lat, lon, alt;
		GeocluePositionFields fields;
		GeoclueAccuracy *accuracy;
		GError *error = NULL;
		
		fields = geoclue_position_get_position (priv->position,
							&timestamp,
							&lat, &lon, &alt,
							&accuracy, 
							&error);
		if (error)
		{
			/* FIXME should cache the error too ? */
			
			g_warning ("Error updating position cache: %s", error->message);
			g_error_free (error);
		} else {
			
			priv->position_cache.timestamp = timestamp; /* this is debatable */
			
			if (fields != priv->position_cache.fields || 
			    lat != priv->position_cache.latitude || 
			    lon != priv->position_cache.longitude ||
			    alt != priv->position_cache.altitude) {
				
				gc_master_provider_set_position (provider,
								 fields, timestamp,
								 lat, lon, alt,
								 accuracy);
			}
		}
	}
	
	if (priv->address) {
		int timestamp;
		GHashTable *details;
		GeoclueAccuracy *accuracy;
		GError *error = NULL;
		
		if (!geoclue_address_get_address (priv->address,
		                                  &timestamp,
		                                  &details,
		                                  &accuracy,
		                                  &error)) {
			/* FIXME should cache the error or return value too ? */
			
			g_warning ("Error updating address cache: %s", error->message);
			g_error_free (error);
			return;
		} else {
			priv->address_cache.timestamp = timestamp;
			
			/*FIXME should check if address has changed from currently cached one */
			gc_master_provider_set_address (provider,
			                                timestamp,
			                                details,
			                                accuracy);
			
		}
	}
}


/* Sets master_status based on provider status and net_status
 * Should be called whenever priv->status or priv->net_status change */
static void
gc_master_provider_handle_status_change (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	GeoclueStatus new_master_status;
	
	if (priv->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK) {
		switch (priv->net_status) {
			case GEOCLUE_CONNECTIVITY_UNKNOWN:
				/*TODO: what should be done? Now falling through*/
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
		gboolean startup = (priv->master_status == -1);
		
		priv->master_status = new_master_status;
		if (!startup) {
			g_signal_emit (provider, signals[STATUS_CHANGED], 0, new_master_status);
			gc_master_provider_update_cache (provider);
		}
	}
}


/* signal handlers for the actual providers signals */

static void
provider_status_changed (GeoclueCommon   *common,
                         GeoclueStatus    status,
                         GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	priv->status = status;
	gc_master_provider_handle_status_change (provider);
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
	                                 accuracy);
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
                                        accuracy);
}


static void
finalize (GObject *object)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (object);
	
	geoclue_accuracy_free (priv->position_cache.accuracy);
	
	geoclue_accuracy_free (priv->address_cache.accuracy);
	
	g_free (priv->name);
	g_free (priv->service);
	g_free (priv->path);
	
	G_OBJECT_CLASS (gc_master_provider_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (object);
	
	if (priv->geoclue) {
		g_object_unref (priv->geoclue);
		priv->geoclue = NULL;
	}
	
	if (priv->position) {
		g_object_unref (priv->position);
		priv->position = NULL;
	}
	
	if (priv->address) {
		g_object_unref (priv->address);
		priv->address = NULL;
	}
	if (priv->address_cache.details) {
		g_hash_table_foreach (priv->address_cache.details, (GHFunc)free_address_key_and_value, NULL);
		g_hash_table_unref (priv->address_cache.details);
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
						  g_cclosure_marshal_VOID__INT,
						  G_TYPE_NONE, 1,
						  G_TYPE_INT);
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
						 dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING),
						 dbus_g_type_get_collection ("GPtrArray", GEOCLUE_ACCURACY_TYPE));
}

static void
gc_master_provider_init (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	priv->master_status = -1;
	
	priv->geoclue = NULL;
	
	priv->position = NULL;
	priv->position_cache.accuracy = NULL;
	
	priv->address = NULL;
	priv->address_cache.accuracy = NULL;
	priv->address_cache.details = NULL;
}


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
	GError *error;
	
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
	if (priv->required_resources & GEOCLUE_RESOURCE_FLAGS_GPS) {
		g_print ("      - GPS\n");
	}

	if (priv->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK) {
		g_print ("      - Network\n");
	}
}

static void
gc_master_provider_dump_provides (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	g_print ("   Provides\n");
	if (priv->provides & GEOCLUE_PROVIDE_FLAGS_UPDATES) {
		g_print ("      - Updates\n");
	}
}

static void
gc_master_provider_dump_provider_details (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	g_print ("\n   Name - %s\n", priv->name);
	g_print ("   Service - %s\n", priv->service);
	g_print ("   Path - %s\n", priv->path);
	g_print ("   Accuracy level - %d\n", priv->promised_accuracy);

	gc_master_provider_dump_required_resources (provider);
	gc_master_provider_dump_provides (provider);
	
	if (priv->position) {
		g_print ("   Interface - Position\n");
		gc_master_provider_dump_position (provider);
	}
	if (priv->address) {
		g_print ("   Interface - Address\n");
		gc_master_provider_dump_address (provider);
	}
}



static gboolean
gc_master_provider_initialize (GcMasterProvider *provider,
                               GError          **error)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	
	/* This will start the provider */
	priv->geoclue = geoclue_common_new (priv->service,
	                                    priv->path);
	if (!geoclue_common_set_options (priv->geoclue,
	                                 geoclue_get_main_options (),
	                                 error)) {
		g_print ("Error setting options: %s\n", (*error)->message);
		g_object_unref (priv->geoclue);
		priv->geoclue = NULL;
		return FALSE;
	}
	
	g_signal_connect (G_OBJECT (priv->geoclue), "status-changed",
			  G_CALLBACK (provider_status_changed), provider);
	
	geoclue_common_get_status (priv->geoclue, &priv->status, error);
	gc_master_provider_handle_status_change (provider);
	
	if (*error != NULL) {
		g_object_unref (priv->geoclue);
		priv->geoclue = NULL;
		return FALSE;
	}
	
	return TRUE;
}

static void
gc_master_provider_add_interfaces (GcMasterProvider *provider,
                                   char            **interfaces)
{
	GcMasterProviderPrivate *priv;
	int i;
	
	priv = GET_PRIVATE (provider);
	
	if (!interfaces) {
		g_warning ("No interfaces defined for %s", priv->name);
		return;
	}
	for (i = 0; interfaces[i]; i++) {
		
		if (strcmp (interfaces[i], GEOCLUE_POSITION_INTERFACE_NAME) == 0) {
			g_assert (priv->position == NULL);
			
			priv->position = geoclue_position_new (priv->service, 
			                                       priv->path);
			g_signal_connect (G_OBJECT (priv->position), "position-changed",
			                  G_CALLBACK (position_changed), provider);
		} else if (strcmp (interfaces[i], GEOCLUE_ADDRESS_INTERFACE_NAME) == 0) {
			g_assert (priv->address == NULL);
			
			priv->address = geoclue_address_new (priv->service, 
			                                     priv->path);
			g_signal_connect (G_OBJECT (priv->address), "address-changed",
			                  G_CALLBACK (address_changed), provider);
		} else {
			g_warning ("Not processing interface %s", interfaces[i]);
			continue;
		}
	}
}

static void
network_status_changed (gpointer *connectivity, 
                        GeoclueNetworkStatus status, 
                        GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	
	priv = GET_PRIVATE (provider);
	
	priv->net_status = status;
	gc_master_provider_handle_status_change (provider);
}


/* public methods (for GcMaster and GcMasterClient) */

/* Loads provider details from 'filename'. if *net_status_callback is
 * defined, net_status_callback may be assigned to a network status 
 * callback function */
GcMasterProvider *
gc_master_provider_new (const char *filename,
                        GeoclueConnectivity *connectivity)
{
	GcMasterProvider *provider;
	GcMasterProviderPrivate *priv;
	GKeyFile *keyfile;
	GError *error = NULL;
	gboolean ret;
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
	
	priv->promised_accuracy = 
		g_key_file_get_integer (keyfile, "Geoclue Provider",
		                        "Accuracy", NULL);
	
	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
	                                    "Requires", NULL, NULL);
	if (flags != NULL) {
		priv->required_resources = parse_resource_strings (flags);
		g_strfreev (flags);
	} else {
		priv->required_resources = GEOCLUE_RESOURCE_FLAGS_NONE;
	}
	
	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
	                                    "Provides", NULL, NULL);
	if (flags != NULL) {
		priv->provides = parse_provide_strings (flags);
		g_strfreev (flags);
	} else {
		priv->provides = GEOCLUE_PROVIDE_FLAGS_NONE;
	}
	priv->use_cache = FALSE;
	if (connectivity && 
	    (priv->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK)) {
		
		/* we have network status events: mark network provider 
		 * with update flag, set the callback and set use_cache */
		priv->provides |= GEOCLUE_PROVIDE_FLAGS_UPDATES;
		
		g_signal_connect (connectivity, 
		                  "status-changed",
		                  G_CALLBACK (network_status_changed), 
		                  provider);
		priv->net_status = geoclue_connectivity_get_status (connectivity);
		priv->use_cache = TRUE;
	}
	
	if (gc_master_provider_initialize (provider, &error) == FALSE) {
		g_warning ("Error starting %s: %s", priv->name,
		           error->message);
		g_object_unref (provider);
		return NULL;
	}
	
	interfaces = g_key_file_get_string_list (keyfile, "Geoclue Provider",
						 "Interfaces", 
						 NULL, NULL);
	gc_master_provider_add_interfaces (provider, interfaces);
	g_strfreev (interfaces);
	
	gc_master_provider_update_cache (provider);
	
	gc_master_provider_dump_provider_details (provider);
	
	
	return provider;
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
	
	if (priv->use_cache) {
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
	
	if (priv->use_cache) {
		if (timestamp != NULL) {
			*timestamp = priv->address_cache.timestamp;
		}
		if (details != NULL) {
			*details = copy_address_details (priv->address_cache.details);
		}
		if (accuracy != NULL) {
			*accuracy = geoclue_accuracy_copy (priv->address_cache.accuracy);
		}
		return TRUE;
	} else {
		return geoclue_address_get_address (priv->address,
		                                    timestamp,
		                                    details, 
		                                    accuracy, 
		                                    error);
	}
}



static GcInterfaceFlags
gc_master_provider_get_supported_interfaces (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE  (provider);
	GcInterfaceFlags ifaces = GC_IFACE_NONE;
	
	if (priv->geoclue != NULL) {
		ifaces |= GC_IFACE_GEOCLUE;
	}
	if (priv->position != NULL) {
		ifaces |= GC_IFACE_POSITION;
	}
	if (priv->address != NULL) {
		ifaces |= GC_IFACE_ADDRESS;
	}
	return ifaces;
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
	GeoclueProvideFlags required_flags = GEOCLUE_PROVIDE_FLAGS_NONE;
	
	priv = GET_PRIVATE (provider);
	
	if (need_update) {
		required_flags |= GEOCLUE_PROVIDE_FLAGS_UPDATES;
	}
	
	supported_ifaces = gc_master_provider_get_supported_interfaces (provider);
	
	/* provider must provide all that is required and
	 * cannot require a resource that is not allowed */
	/* TODO: really, we need to change some of those terms... */
	
	return (((supported_ifaces & iface_type) == iface_type) &&
	        ((priv->provides & required_flags) == required_flags) &&
	        (priv->promised_accuracy >= min_accuracy) &&
	        ((priv->required_resources & (~allowed_resources)) == 0));
}

GeoclueNetworkStatus 
gc_master_provider_get_status (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->master_status;
}

/*returns a reference, but is not meant for editing...*/
char * 
gc_master_provider_get_name (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->name;
}

GeoclueAccuracyLevel
gc_master_provider_get_accuracy_level (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	return priv->promised_accuracy;
}

/* GCompareFunc for sorting providers by accuracy and required resources */
gint
gc_master_provider_compare (GcMasterProvider *a, 
                            GcMasterProvider *b)
{
	int diff;
	GeoclueAccuracyLevel level_a, level_b;
	
	GcMasterProviderPrivate *priv_a = GET_PRIVATE (a);
	GcMasterProviderPrivate *priv_b = GET_PRIVATE (b);
	
	if (priv_a->position_cache.accuracy) {
		geoclue_accuracy_get_details (priv_a->position_cache.accuracy,
		                              &level_a, NULL, NULL);
	} else {
		level_a = priv_a->promised_accuracy;
	}
	if (priv_b->position_cache.accuracy) {
		geoclue_accuracy_get_details (priv_b->position_cache.accuracy,
		                              &level_b, NULL, NULL);
	} else {
		level_b = priv_b->promised_accuracy;
	}
	
	diff =  level_b - level_a;
	if (diff != 0) {
		return diff;
	}
	return priv_a->required_resources - priv_b->required_resources;
}
