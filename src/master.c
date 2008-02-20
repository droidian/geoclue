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
#include "master-provider.h"

#ifdef HAVE_NETWORK_MANAGER
#include "connectivity-networkmanager.h"
#else
#ifdef HAVE_CONIC
#include "connectivity-conic.h"
#endif
#endif

G_DEFINE_TYPE (GcMaster, gc_master, G_TYPE_OBJECT);

static GList *providers = NULL;

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

        // For each option that has been passed in we need to 
        //   check if it overrides one already in the options table
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



/* Load the provider details out of a keyfile */
static void
gc_master_add_new_provider (GcMaster   *master,
                            const char *filename)
{
	GcMasterProvider *provider;
	
	provider = gc_master_provider_new (filename, 
	                                   master->connectivity != NULL);
	if (!provider) {
		g_warning ("Loading from %s failed", filename);
		return;
	}
	
	providers = g_list_insert_sorted 
		(providers, provider, 
		 (GCompareFunc)gc_master_provider_compare_by_accuracy);
}

/* Scan a directory for .provider files */
#define PROVIDER_EXTENSION ".provider"

static void
gc_master_load_providers (GcMaster *master)
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

		g_print ("\nFound %s", filename);
		ext = strrchr (filename, '.');
		if (ext == NULL || strcmp (ext, PROVIDER_EXTENSION) != 0) {
			g_print (" - Ignored\n");
			filename = g_dir_read_name (dir);
			continue;
		}

		fullname = g_build_filename (GEOCLUE_PROVIDERS_DIR, 
					     filename, NULL);
		g_print ("\n");
		gc_master_add_new_provider (master, fullname);
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
	
	if (providers == NULL) {
		return;
	}
	
	gc_status = ConnectivityStatusToProviderStatus[status];
	for (l = providers; l; l = l->next) {
		GcMasterProvider *provider = l->data;
		
		/* might be nicer to do this with a signal, but ... */
		gc_master_provider_network_status_changed (provider,
		                                           gc_status);
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
	
	gc_master_load_providers (master);
}


GList *
gc_master_get_providers (GcInterfaceFlags      iface_type,
                         GeoclueAccuracyLevel  min_accuracy,
                         gboolean              can_update,
                         GeoclueResourceFlags  allowed,
                         GError              **error)
{
	GList *l, *p = NULL;
	
	if (providers == NULL) {
		return NULL;
	}
	
	for (l = providers; l; l = l->next) {
		GcMasterProvider *provider = l->data;
		
		if (gc_master_provider_is_good (provider,
		                                iface_type, 
		                                min_accuracy, 
		                                can_update, 
		                                allowed)) {
			p = g_list_prepend (p, provider);
		}
	}
	
	/* return most accurate first */
	return g_list_reverse (p);
}
