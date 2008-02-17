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
 * TODO: check geoclue-accuracy frees...
 *        fix status especially for network providers
 **/

#include <string.h>

#include "main.h"
#include "master-provider.h"
#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-marshal.h>

#define POSITION_IFACE "org.freedesktop.Geoclue.Position"

typedef enum _GeoclueProvideFlags {
	GEOCLUE_PROVIDE_FLAGS_NONE = 0,
	GEOCLUE_PROVIDE_FLAGS_UPDATES = 1 << 0,
	GEOCLUE_PROVIDE_FLAGS_DETAILED = 1 << 1,
	GEOCLUE_PROVIDE_FLAGS_FUZZY = 1 << 2,
} GeoclueProvideFlags;

typedef struct _GeocluePositionCache {
	int timestamp;
	GeocluePositionFields fields;
	double latitude;
	double longitude;
	double altitude;
	GeoclueAccuracy *accuracy;
} GeocluePositionCache;


typedef struct _GcMasterProviderPrivate {
	char *name;
	char *service;
	char *path;
	
	GeoclueResourceFlags required_resources;
	GeoclueProvideFlags provides;
	
	gboolean use_cache;
	
	/* for Geoclue interface */
	GeoclueCommon *geoclue;
	GeoclueStatus status;
	
	/* for Position interface */
	GeocluePosition *position;
	GeocluePositionCache position_cache;
} GcMasterProviderPrivate;

enum {
	POSITION_CHANGED,
	LAST_SIGNAL
};
static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GC_TYPE_MASTER_PROVIDER, GcMasterProviderPrivate))

G_DEFINE_TYPE (GcMasterProvider, gc_master_provider, G_TYPE_OBJECT)

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
	
	priv->position_cache.timestamp = timestamp;
	priv->position_cache.fields = fields;
	priv->position_cache.latitude = latitude;
	priv->position_cache.longitude = longitude;
	priv->position_cache.altitude = altitude;
	
	/*FIXME: free old one?*/
	priv->position_cache.accuracy = geoclue_accuracy_copy (accuracy);
	
	g_signal_emit (provider, signals[POSITION_CHANGED], 0, 
	               fields, timestamp, 
	               latitude, longitude, altitude, 
	               priv->position_cache.accuracy);
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
		} else if (strcmp (flags[i], "ProvidesDetailedAccuracy") == 0) {
			provides |= GEOCLUE_PROVIDE_FLAGS_DETAILED;
		} else if (strcmp (flags[i], "ProvidesFuzzyAccuracy") == 0) {
			provides |= GEOCLUE_PROVIDE_FLAGS_FUZZY;
		}
	}
	
	return provides;
}



/* signal handlers for the actual providers signals */

static void
provider_status_changed (GeoclueCommon   *common,
                         GeoclueStatus    status,
                         GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	/* TODO emit status_changed ? */
	
	priv->status = status;
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
finalize (GObject *object)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (object);
	
	if (priv->position_cache.accuracy) {
		geoclue_accuracy_free (priv->position_cache.accuracy);
	}
	
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
	
	G_OBJECT_CLASS (gc_master_provider_parent_class)->dispose (object);
}

static void
gc_master_provider_class_init (GcMasterProviderClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;
	
	g_type_class_add_private (klass, sizeof (GcMasterProviderPrivate));
	
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
}

static void
gc_master_provider_init (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	priv->geoclue = NULL;
	priv->position = NULL;
	
	priv->position_cache.accuracy = NULL;
}

/* update_cache is called when GcMaster has asked for it
 * (e.g. network is available) */
static void 
gc_master_provider_update_cache (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	int timestamp;
	double lat, lon, alt;
	GeocluePositionFields fields;
	GeoclueAccuracy *accuracy;
	GError *error;
	
	priv = GET_PRIVATE (provider);
	error = NULL;
	
	if (priv->position == NULL || priv->use_cache == FALSE) {
		return;
	}
	
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
		return;
	}
	
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


static void
gc_master_provider_dump_position (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv;
	GeocluePositionFields fields;
	int time;
	double lat, lon, alt;
	GError *error = NULL;
	
	priv = GET_PRIVATE (provider);
	
	fields = gc_master_provider_get_position (provider,
	                                          &time, 
	                                          &lat, &lon, &alt,
	                                          NULL, &error);
	
	g_print ("Position Information:\n");
	g_print ("---------------------\n");
	if (error) {
		g_print ("  Error: %s", error->message);
		g_error_free (error);
		return;
	}
	g_print ("   Timestamp: %d\n", time);
	g_print ("   Latitude: %.2f %s\n", lat,
		 fields & GEOCLUE_POSITION_FIELDS_LATITUDE ? "" : "(not set)");
	g_print ("   Longitude: %.2f %s\n", lon,
		 fields & GEOCLUE_POSITION_FIELDS_LONGITUDE ? "" : "(not set)");
	g_print ("   Altitude: %.2f %s\n", alt,
		 fields & GEOCLUE_POSITION_FIELDS_ALTITUDE ? "" : "(not set)");
	
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

	if (priv->provides & GEOCLUE_PROVIDE_FLAGS_DETAILED) {
		g_print ("      - Detailed Accuracy\n");
	} else if (priv->provides & GEOCLUE_PROVIDE_FLAGS_FUZZY) {
		g_print ("      - Fuzzy Accuracy\n");
	} else {
		g_print ("      - No Accuracy\n");
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

	gc_master_provider_dump_required_resources (provider);
	gc_master_provider_dump_provides (provider);
	
	if (priv->position) {
		g_print ("   Interface - Position\n");
		gc_master_provider_dump_position (provider);
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
	
	/* TODO fix this for network providers ? */
	geoclue_common_get_status (priv->geoclue, &priv->status, error);
	
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
		
		
		if (strcmp (interfaces[i], POSITION_IFACE) == 0) {
			g_assert (priv->position == NULL);
			
			priv->position = geoclue_position_new (priv->service, 
			                                       priv->path);
			g_signal_connect (G_OBJECT (priv->position), "position-changed",
			                  G_CALLBACK (position_changed), provider);
		} else {
			g_warning ("Not processing interface %s", interfaces[i]);
			continue;
		}
	}
}



/* public methods (for GcMaster and GcMasterClient) */

GcMasterProvider *
gc_master_provider_new (const char *filename,
                        gboolean network_status_events)
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
	
	/* FIXME: might want to set use_cache == FALSE 
	 * for gps providers at least, so there wouldn't
	 * be so much unnecessary copying ? */
	priv->use_cache = network_status_events;
	
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
	
	/* if master is connectivity event aware, mark network provider
	 * with update flag */
	if ((network_status_events) && 
	    (priv->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK)) {
		priv->provides |= GEOCLUE_PROVIDE_FLAGS_UPDATES;
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

/* might want to change this to a bitfield defining all supported ifaces */
gboolean 
gc_master_provider_is_position_provider (GcMasterProvider *provider)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE  (provider);
	
	return (priv->position != NULL);
}

gboolean
gc_master_provider_is_good (GcMasterProvider *provider,
                            GeoclueAccuracy      *accuracy,
                            GeoclueResourceFlags  allowed_resources,
                            gboolean              need_update)
{
	GeoclueAccuracyLevel level;
	GcMasterProviderPrivate *priv;
	GeoclueProvideFlags required_flags = GEOCLUE_PROVIDE_FLAGS_NONE;
	
	priv = GET_PRIVATE (provider);
	
	if (need_update) {
		required_flags |= GEOCLUE_PROVIDE_FLAGS_UPDATES;
	}

	if (accuracy != NULL) {
		geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	} else {
		level = GEOCLUE_ACCURACY_LEVEL_NONE;
	}
	
	/*FIXME: what if only numeric accuracy is defined? */
	if (level == GEOCLUE_ACCURACY_LEVEL_DETAILED) {
		required_flags |= GEOCLUE_PROVIDE_FLAGS_DETAILED;
	} else if (level >= GEOCLUE_ACCURACY_LEVEL_NONE &&
		   level < GEOCLUE_ACCURACY_LEVEL_DETAILED) {
		
		required_flags |= GEOCLUE_PROVIDE_FLAGS_FUZZY;
	}
	
	/* provider must provide all that is required and
	 * cannot require a resource that is not allowed */
	/* TODO: really, we need to change some of those terms... */
	
	/* TODO shouldn't check equality for provides, should check flags ? */
	return ((priv->provides == required_flags) &&
	        ((priv->required_resources & (~allowed_resources)) == 0));
}

/* for GcMaster */
void
gc_master_provider_network_status_changed (GcMasterProvider *provider,
                                           GeoclueStatus status)
{
	GcMasterProviderPrivate *priv = GET_PRIVATE (provider);
	
	if ((priv->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK) &&
	    (status != priv->status)) {
		
		provider_status_changed (priv->geoclue, status, provider);
		
		/* update position cache */
		/* FIXME what should happen when net is down:
		 * should position cache be updated ? */
		if (priv->position && status == GEOCLUE_STATUS_AVAILABLE) {
			gc_master_provider_update_cache (provider);
		}
	}
}
