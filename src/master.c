/*
 * Geoclue
 * master.c - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <string.h>

#include "main.h"
#include "master.h"
#include "client.h"
#include "master-position.h"

#ifdef HAVE_NETWORK_MANAGER
#include "connectivity-networkmanager.h"
#else
#ifdef HAVE_CONIC
#include "connectivity-conic.h"
#endif
#endif

G_DEFINE_TYPE (GcMaster, gc_master, G_TYPE_OBJECT);

static GList *position_providers = NULL;

static gboolean gc_iface_master_create (GcMaster    *master,
					const char **object_path,
					GError     **error);
static gboolean gc_iface_master_set_options (GcMaster   *master,
                                             GHashTable *options,
                                             GError    **error);
static gboolean gc_iface_master_shutdown (GcMaster *master,
					  GError  **error);

#include "gc-iface-master-glue.h"

#define GEOCLUE_MASTER_PATH "/org/freedesktop/Geoclue/Master/client"
static gboolean
gc_iface_master_create (GcMaster    *master,
			const char **object_path,
			GError     **error)
{
	static guint32 serial = 0;
	GcMasterClient *client;
	char *path;

	path = g_strdup_printf ("%s%d", GEOCLUE_MASTER_PATH, serial++);
	client = g_object_new (GC_TYPE_MASTER_CLIENT, NULL);
	dbus_g_connection_register_g_object (master->connection, path,
					     G_OBJECT (client));

	*object_path = path;
	return TRUE;
}

static void
merge_options (gpointer k,
               gpointer v,
               gpointer data)
{
        GHashTable *options = data;
        const char *key = k;
        const char *value = v;

        /* For each option that has been passed in we need to 
           check if it overrides one already in the options table */
        g_hash_table_replace (options, g_strdup (key), g_strdup (value));
}

static gboolean
gc_iface_master_set_options (GcMaster   *master,
                             GHashTable *options,
                             GError    **error)
{
        g_hash_table_foreach (options, merge_options, 
                              geoclue_get_main_options ());
        return TRUE;
}

static gboolean
gc_iface_master_shutdown (GcMaster *master,
			  GError  **error)
{
	return TRUE;
}

static void
gc_master_class_init (GcMasterClass *klass)
{
	dbus_g_object_type_install_info (gc_master_get_type (),
					 &dbus_glib_gc_iface_master_object_info);
}

static GeoclueResourceFlags
parse_resource_strings (GcMaster *master,
		       char    **flags)
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
parse_provide_strings (GcMaster *master,
		       char    **flags)
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

static void
dump_position (GcMasterPosition *position)
{
	GeocluePositionFields fields;
	int time;
	double lat, lon, alt;
	GError *error = NULL;
	
	fields = gc_master_position_get_position (position,
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
provider_status_changed (GeoclueCommon   *common,
                         GeoclueStatus    status,
                         ProviderDetails *details)
{
	g_assert (details != NULL);
	
	g_print ("Changing provider status: %d->%d\n", details->status, status);
	details->status = status;
}

static gboolean
initialize_provider (GcMaster        *master,
		     ProviderDetails *provider,
		     GError         **error)
{
	/* This will start the provider */
	provider->geoclue = geoclue_common_new (provider->service,
						provider->path);
        if (!geoclue_common_set_options (provider->geoclue,
                                         geoclue_get_main_options (),
                                         error)) {
                g_print ("Error setting options: %s\n", (*error)->message);
                g_object_unref (provider->geoclue);
                provider->geoclue = NULL;
                return FALSE;
        }

	g_signal_connect (G_OBJECT (provider->geoclue), "status-changed",
			  G_CALLBACK (provider_status_changed), provider);

	/* TODO fix this for network providers */
	geoclue_common_get_status (provider->geoclue, &provider->status, error);
	
	if (*error != NULL) {
		g_object_unref (provider->geoclue);
                provider->geoclue = NULL;
		return FALSE;
	}
	
	return TRUE;
}

static void
add_new_interfaces (GcMaster        *master,
                    ProviderDetails *provider,
                    char           **interfaces)
{
	int i;
	
	if (!interfaces) {
		g_warning ("No interfaces defined for %s", provider->name);
		return;
	}
	for (i = 0; interfaces[i]; i++) {
		
		
		if (strcmp (interfaces[i], POSITION_IFACE) == 0) {
			g_assert (provider->master_position == NULL);
			
			/* FIXME: might want to set use_cache == FALSE for gps at least ? */
			provider->master_position = 
				gc_master_position_new (provider->service,
				                        provider->path,
				                        TRUE);
			
			position_providers = g_list_prepend (position_providers,
			                                     provider);
		} else {
			g_warning ("Not processing interface %s", interfaces[i]);
			continue;
		}
		
	}
}

static void
dump_required_resources (ProviderDetails *details)
{
	g_print ("   Requires\n");
	if (details->required_resources & GEOCLUE_RESOURCE_FLAGS_GPS) {
		g_print ("      - GPS\n");
	}

	if (details->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK) {
		g_print ("      - Network\n");
	}
}

static void
dump_provides (ProviderDetails *details)
{
	g_print ("   Provides\n");
	if (details->provides & GEOCLUE_PROVIDE_FLAGS_UPDATES) {
		g_print ("      - Updates\n");
	}

	if (details->provides & GEOCLUE_PROVIDE_FLAGS_DETAILED) {
		g_print ("      - Detailed Accuracy\n");
	} else if (details->provides & GEOCLUE_PROVIDE_FLAGS_FUZZY) {
		g_print ("      - Fuzzy Accuracy\n");
	} else {
		g_print ("      - No Accuracy\n");
	}
}

static void
dump_provider_details (struct _ProviderDetails *details)
{
	g_print ("   Name - %s\n", details->name);
	g_print ("   Service - %s\n", details->service);
	g_print ("   Path - %s\n", details->path);

	dump_required_resources (details);
	dump_provides (details);
	
	if (details->master_position) {
		g_print ("   Interface - Position\n");
		dump_position (details->master_position);
	}
}

/* Load the provider details out of a keyfile */
static void
add_new_provider (GcMaster   *master,
                  const char *filename)
{
	/*FIXME*/
	ProviderDetails *provider;
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
		return;
	}

	g_print ("Loaded %s\n", filename);
	provider = g_new0 (ProviderDetails, 1);
	provider->name = g_key_file_get_value (keyfile, "Geoclue Provider",
					       "Name", NULL);
	provider->service = g_key_file_get_value (keyfile, "Geoclue Provider",
						  "Service", NULL);
	provider->path = g_key_file_get_value (keyfile, "Geoclue Provider",
					       "Path", NULL);
	
	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
					    "Requires", NULL, NULL);
	if (flags != NULL) {
		provider->required_resources = parse_resource_strings (master, flags);
		g_strfreev (flags);
	} else {
		provider->required_resources = GEOCLUE_RESOURCE_FLAGS_NONE;
	}

	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
					    "Provides", NULL, NULL);
	if (flags != NULL) {
		provider->provides = parse_provide_strings (master, flags);
		g_strfreev (flags);
	} else {
		provider->provides = GEOCLUE_PROVIDE_FLAGS_NONE;
	}
	
	/* if master is connectivity event aware, mark all network providers 
	 * with update flag */
	if ((master->connectivity != NULL) && 
	    (provider->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK)) {
		provider->provides |= GEOCLUE_PROVIDE_FLAGS_UPDATES;
	}
	if (initialize_provider (master, provider, &error) == FALSE) {
		g_warning ("Error starting %s - %s", provider->name,
			   error->message);
		g_error_free (error);

		g_free (provider->path);
		g_free (provider->name);
		g_free (provider->service);
		g_free (provider);

		return;
	}
	
	provider->master_position = NULL;
	interfaces = g_key_file_get_string_list (keyfile, "Geoclue Provider",
						 "Interfaces", 
						 NULL, NULL);
	add_new_interfaces (master, provider, interfaces);
	g_strfreev (interfaces);
	
	dump_provider_details (provider);
}

/* Scan a directory for .provider files */
#define PROVIDER_EXTENSION ".provider"

static void
load_providers (GcMaster *master)
{
	GDir *dir;
	GError *error = NULL;
	const char *filename;

	dir = g_dir_open (GEOCLUE_PROVIDERS_DIR, 0, &error);
	if (dir == NULL) {
		g_warning ("Error opening %s: %s", GEOCLUE_PROVIDERS_DIR,
			   error->message);
		g_error_free (error);
		return;
	}

	filename = g_dir_read_name (dir);
	while (filename) {
		char *fullname, *ext;

		g_print ("Found %s", filename);
		ext = strrchr (filename, '.');
		if (ext == NULL || strcmp (ext, PROVIDER_EXTENSION) != 0) {
			g_print (" - Ignored\n");
			filename = g_dir_read_name (dir);
			continue;
		}

		fullname = g_build_filename (GEOCLUE_PROVIDERS_DIR, 
					     filename, NULL);
		g_print ("\n");
		add_new_provider (master, fullname);
		g_free (fullname);
		
		filename = g_dir_read_name (dir);
	}

	g_dir_close (dir);
}

static int ConnectivityStatusToProviderStatus[] = {
	GEOCLUE_STATUS_UNAVAILABLE,  /*GEOCLUE_CONNECTIVITY_UNKNOWN,*/
	GEOCLUE_STATUS_UNAVAILABLE,  /*GEOCLUE_CONNECTIVITY_OFFLINE,*/
	GEOCLUE_STATUS_ACQUIRING,    /*GEOCLUE_CONNECTIVITY_ACQUIRING,*/
	GEOCLUE_STATUS_AVAILABLE     /*GEOCLUE_CONNECTIVITY_ONLINE,*/
};

static void
network_status_changed (GeoclueConnectivity *connectivity, 
                        GeoclueNetworkStatus status, 
                        GcMaster *master)
{
	/* TODO: connectivity stuff should maybe have some latency
	 * so we wouldn't start this on any 2sec network problems */
	GList *l;
	GeoclueStatus gc_status;
	
	if (position_providers == NULL) {
		return;
	}
	
	gc_status = ConnectivityStatusToProviderStatus[status];
	for (l = position_providers; l; l = l->next) {
		ProviderDetails *details = l->data;
		
		if ((details->required_resources & GEOCLUE_RESOURCE_FLAGS_NETWORK) &&
		    (gc_status != details->status)) {
			
			provider_status_changed (details->geoclue, 
			                         gc_status, details);
			
			/* update position cache */
			/* FIXME what should happen when net is down:
			 * should position cache be updated ? */
			if (details->master_position && gc_status == GEOCLUE_STATUS_AVAILABLE) {
				gc_master_position_update_cache (details->master_position);
			}
		}
	}
}

static void
gc_master_init (GcMaster *master)
{
	GError *error = NULL;
	
	
	master->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (master->connection == NULL) {
		g_warning ("Could not get %s: %s", GEOCLUE_DBUS_BUS, 
			   error->message);
		g_error_free (error);
	}
	
	master->connectivity = NULL;
#ifdef HAVE_NETWORK_MANAGER
	master->connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_NETWORKMANAGER, NULL));
#else
#ifdef HAVE_CONIC
	master->connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_CONIC, NULL));
#endif
#endif
	g_signal_connect (master->connectivity, "status-changed",
	                  G_CALLBACK (network_status_changed), master);
	
	if (position_providers == NULL) {
		load_providers (master);
	}
}

static gboolean
provider_is_good (ProviderDetails      *details,
		  GeoclueAccuracy      *accuracy,
		  GeoclueResourceFlags  allowed_resources,
		  gboolean              can_update)
{
	/*FIXME*/
	
	GeoclueProvideFlags required_flags = GEOCLUE_PROVIDE_FLAGS_NONE;
	GeoclueAccuracyLevel level;

	if (can_update) {
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
	} else if (level > GEOCLUE_ACCURACY_LEVEL_NONE &&
		   level < GEOCLUE_ACCURACY_LEVEL_DETAILED) {
		required_flags |= GEOCLUE_PROVIDE_FLAGS_FUZZY;
	}
	
	/* provider must provide all that is required and
	 * cannot require a resource that is not allowed */
	/* TODO: really, we need to change some of those terms... */
	
	return (details->provides == required_flags &&
	        (details->required_resources & (~allowed_resources)) == 0);
}

GList *
gc_master_get_position_providers (GeoclueAccuracy      *accuracy,
                                  gboolean              can_update,
                                  GeoclueResourceFlags  allowed,
                                  GError              **error)
{
	GList *l, *p = NULL;

	if (position_providers == NULL) {
		return NULL;
	}
	
	/* we should probably return in some order? 
	 * requirement or accuracy? */
	for (l = position_providers; l; l = l->next) {
		ProviderDetails *details = l->data;
		
		if (!provider_is_good (details, accuracy, can_update, allowed)) {
			p = g_list_prepend (p, details);
		}
	}

	return g_list_reverse (p);
}
