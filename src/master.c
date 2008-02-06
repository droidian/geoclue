/*
 * Geoclue
 * master.c - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <string.h>

#include "master.h"
#include "client.h"

#ifdef HAVE_NETWORK_MANAGER
#include "connectivity-networkmanager.h"
#else
#ifdef HAVE_CONIC
#include "connectivity-conic.h"
#endif
#endif

G_DEFINE_TYPE (GcMaster, gc_master, G_TYPE_OBJECT);

static GList *providers = NULL;

static gboolean gc_iface_master_create (GcMaster      *master,
					const char   **object_path,
					GError       **error);
static gboolean gc_iface_master_shutdown (GcMaster *master,
					  GError  **error);

#include "gc-iface-master-glue.h"

#define GEOCLUE_MASTER_PATH "/org/freedesktop/Geoclue/Master/client"
static gboolean
gc_iface_master_create (GcMaster      *master,
			const char   **object_path,
			GError       **error)
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

static GeoclueRequireFlags
parse_require_strings (GcMaster *master,
		       char    **flags)
{
	GeoclueRequireFlags requires = GEOCLUE_REQUIRE_FLAGS_NONE;
	int i;

	for (i = 0; flags[i]; i++) {
		if (strcmp (flags[i], "RequiresNetwork") == 0) {
			requires |= GEOCLUE_REQUIRE_FLAGS_NETWORK;
		} else if (strcmp (flags[i], "RequiresGPS") == 0) {
			requires |= GEOCLUE_REQUIRE_FLAGS_GPS;
		}
	}

	return requires;
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
dump_position (ProviderDetails   *provider,
	       ProviderInterface *iface)
{
	GeocluePositionFields fields;

	fields = iface->details.position.fields;
	g_print ("Position Information: %s\n", provider->name);
	g_print ("---------------------\n");
	g_print ("   Timestamp: %d\n", iface->details.position.timestamp);
	g_print ("   Latitude: %.2f %s\n", iface->details.position.latitude,
		 fields & GEOCLUE_POSITION_FIELDS_LATITUDE ? "" : "(not set)");
	g_print ("   Longitude: %.2f %s\n", iface->details.position.longitude,
		 fields & GEOCLUE_POSITION_FIELDS_LONGITUDE ? "" : "(not set)");
	g_print ("   Altitude: %.2f %s\n", iface->details.position.altitude,
		 fields & GEOCLUE_POSITION_FIELDS_ALTITUDE ? "" : "(not set)");
}

static gboolean
update_interface (ProviderDetails   *provider,
		  ProviderInterface *iface,
		  GError           **error)
{
	GError *real_error = NULL;

	switch (iface->type) {
	case POSITION_INTERFACE:
		if (provider->status == GEOCLUE_STATUS_AVAILABLE) {
			GeocluePositionFields fields;
			
			fields = geoclue_position_get_position 
				(iface->details.position.position,
				 &iface->details.position.timestamp,
				 &iface->details.position.latitude,
				 &iface->details.position.longitude,
				 &iface->details.position.altitude,
				 &iface->details.position.accuracy,
				 &real_error);
			if (real_error != NULL) {
				if (*error != NULL) {
					*error = real_error;
				} else {
					g_warning ("Error updating position: %s",
						   real_error->message);
					g_error_free (real_error);
				}

				return FALSE;
			}
			iface->details.position.fields = fields;

			dump_position (provider, iface);
		}
		
		break;
		
	case VELOCITY_INTERFACE:
		break;
		
	default:
		break;
	}

	return TRUE;
}

static void
provider_status_changed (GeoclueCommon   *common,
			 GeoclueStatus    status,
			 ProviderDetails *provider)
{
	provider->status = status;
	if (provider->status == GEOCLUE_STATUS_AVAILABLE) {
		int i;
		
		if (provider->interfaces == NULL) {
			return;
		}
		
		/* Update all the available interfaces */
		for (i = 0; i < provider->interfaces->len; i++) {
			update_interface (provider, 
					  provider->interfaces->pdata[i], NULL);
		}
	}
}

static gboolean
initialize_provider (GcMaster        *master,
		     ProviderDetails *provider,
		     GError         **error)
{
	/* This will start the provider */
	provider->geoclue = geoclue_common_new (provider->service,
						provider->path);
	g_signal_connect (G_OBJECT (provider->geoclue), "status-changed",
			  G_CALLBACK (provider_status_changed), provider);
	
	/* TODO fix this for network providers */
	geoclue_common_get_status (provider->geoclue, &provider->status, error);
	
	if (*error != NULL) {
		g_object_unref (provider->geoclue);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
initialize_interface (GcMaster          *master,
		      ProviderDetails   *provider,
		      ProviderInterface *iface,
		      GError           **error)
{
	switch (iface->type) {
	case POSITION_INTERFACE:
		iface->details.position.position = 
			geoclue_position_new (provider->service,
					      provider->path);
		break;
		
	case VELOCITY_INTERFACE:
		iface->details.velocity.velocity = 
			geoclue_velocity_new (provider->service,
					      provider->path);
		break;
		
	default:
		break;
	}
	
	if (update_interface (provider, iface, error) == FALSE) {
		return FALSE;
	}

	return TRUE;
}

static GPtrArray *
parse_interface_strings (GcMaster        *master,
			 ProviderDetails *provider,
			 char           **interfaces,
			 guint            n_interfaces)
{
	GPtrArray *ifaces;
	int i;

	ifaces = g_ptr_array_sized_new (n_interfaces);
	for (i = 0; interfaces[i]; i++) {
		ProviderInterface *iface;
		GError *error = NULL;

		iface = g_new0 (ProviderInterface, 1);

		if (strcmp (interfaces[i], POSITION_IFACE) == 0) {
			iface->type = POSITION_INTERFACE;
		} else if (strcmp (interfaces[i], VELOCITY_IFACE) == 0) {
			iface->type = VELOCITY_INTERFACE;
		} else {
			g_free (iface);
			continue;
		}

		if (initialize_interface (master, provider, 
					  iface, &error) == FALSE) {
			g_warning ("Error starting interface: %s", 
				   error->message);
			g_error_free (error);
			g_free (iface);
		} else {
			g_ptr_array_add (ifaces, iface);
		}
	}

	return ifaces;
}
			
static void
dump_requires (struct _ProviderDetails *details)
{
	g_print ("   Requires\n");
	if (details->requires & GEOCLUE_REQUIRE_FLAGS_GPS) {
		g_print ("      - GPS\n");
	}

	if (details->requires & GEOCLUE_REQUIRE_FLAGS_NETWORK) {
		g_print ("      - Network\n");
	}
}

static void
dump_provides (struct _ProviderDetails *details)
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
dump_interfaces (struct _ProviderDetails *details)
{
	int i;

	g_print ("   Interfaces");
	if (details->interfaces == NULL) {
		g_print (" - None\n");
		return;
	}

	g_print ("\n");
	for (i = 0; i < details->interfaces->len; i++) {
		struct _ProviderInterface *iface;

		iface = details->interfaces->pdata[i];
		switch (iface->type) {
		case POSITION_INTERFACE:
			g_print ("      Position Interface\n");
			break;

		case VELOCITY_INTERFACE:
			g_print ("      Velocity Interface\n");
			break;

		default:
			g_print ("      Unknown Interface\n");
			break;
		}
	}
}

static void
dump_provider_details (struct _ProviderDetails *details)
{
	g_print ("   Name - %s\n", details->name);
	g_print ("   Service - %s\n", details->service);
	g_print ("   Path - %s\n", details->path);

	dump_requires (details);
	dump_provides (details);
	dump_interfaces (details);
}

/* Load the provider details out of a keyfile */
static struct _ProviderDetails *
new_provider (GcMaster   *master,
	      const char *filename)
{
	struct _ProviderDetails *provider;
	GKeyFile *keyfile;
	GError *error = NULL;
	gboolean ret;
	char **flags, **interfaces;
	guint n_interfaces;

	keyfile = g_key_file_new ();
	ret = g_key_file_load_from_file (keyfile, filename, 
					 G_KEY_FILE_NONE, &error);
	if (ret == FALSE) {
		g_warning ("Error loading %s: %s", filename, error->message);
		g_error_free (error);
		g_key_file_free (keyfile);
		return NULL;
	}

	g_print ("Loaded %s\n", filename);
	provider = g_new0 (struct _ProviderDetails, 1);
	provider->name = g_key_file_get_value (keyfile, "Geoclue Provider",
					       "Name", NULL);
	provider->service = g_key_file_get_value (keyfile, "Geoclue Provider",
						  "Service", NULL);
	provider->path = g_key_file_get_value (keyfile, "Geoclue Provider",
					       "Path", NULL);
	
	flags = g_key_file_get_string_list (keyfile, "Geoclue Provider",
					    "Requires", NULL, NULL);
	if (flags != NULL) {
		provider->requires = parse_require_strings (master, flags);
		g_strfreev (flags);
	} else {
		provider->requires = GEOCLUE_REQUIRE_FLAGS_NONE;
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
	    (provider->requires & GEOCLUE_REQUIRE_FLAGS_NETWORK)) {
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

		return NULL;
	}

	interfaces = g_key_file_get_string_list (keyfile, "Geoclue Provider",
						 "Interfaces", 
						 &n_interfaces, NULL);
	if (interfaces != NULL) {
		provider->interfaces = parse_interface_strings (master, 
								provider,
								interfaces,
								n_interfaces);
		g_strfreev (interfaces);
	} else {
		provider->interfaces = NULL;
	}

	dump_provider_details (provider);
	return provider;
}

/* Scan a directory for .provider files */
#define PROVIDER_EXTENSION ".provider"

static GList *
load_providers (GcMaster *master)
{
	GList *providers = NULL;
	GDir *dir;
	GError *error = NULL;
	const char *filename;

	dir = g_dir_open (GEOCLUE_PROVIDERS_DIR, 0, &error);
	if (dir == NULL) {
		g_warning ("Error opening %s: %s", GEOCLUE_PROVIDERS_DIR,
			   error->message);
		g_error_free (error);
		return NULL;
	}

	filename = g_dir_read_name (dir);
	while (filename) {
		struct _ProviderDetails *provider;
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
		provider = new_provider (master, fullname);
		g_free (fullname);

		if (provider) {
			providers = g_list_prepend (providers, provider);
		}

		filename = g_dir_read_name (dir);
	}

	g_dir_close (dir);
	return providers;
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
                        gpointer userdata)
{
	/* TODO: connectivity stuff should maybe have some latency
	 * so we wouldn't start this on any 2sec network problems */
	GList *l;
	GeoclueStatus gc_status;
	
	g_debug ("network status changed: %d", status);
	
	if (providers == NULL) {
		return;
	}
	
	gc_status = ConnectivityStatusToProviderStatus[status];
	for (l = providers; l; l = l->next) {
		ProviderDetails *provider = l->data;
		/* could also check internal status with geoclue_common_get_status
		 * and not do this if it returns UNAVAILABLE ...*/
		
		if ((provider->requires & GEOCLUE_REQUIRE_FLAGS_NETWORK) &&
		    (gc_status != provider->status)) {
			provider_status_changed (provider->geoclue, gc_status, provider);
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
	
	if (providers == NULL) {
		providers = load_providers (master);
	}
}

static gboolean
provider_is_good (ProviderDetails *details,
		  GeoclueAccuracy *accuracy,
		  gboolean         can_update)
{
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

	if (level == GEOCLUE_ACCURACY_LEVEL_DETAILED) {
		required_flags |= GEOCLUE_PROVIDE_FLAGS_DETAILED;
	} else if (level > GEOCLUE_ACCURACY_LEVEL_NONE &&
		   level < GEOCLUE_ACCURACY_LEVEL_DETAILED) {
		required_flags |= GEOCLUE_PROVIDE_FLAGS_FUZZY;
	}
	
	return (details->provides == required_flags);
}

GList *
gc_master_get_providers (GeoclueAccuracy *accuracy,
			 gboolean         can_update,
			 GError         **error)
{
	GList *l, *p = NULL;

	if (providers == NULL) {
		return NULL;
	}

	for (l = providers; l; l = l->next) {
		ProviderDetails *details = l->data;

		if (!provider_is_good (details, accuracy, can_update)) {
			p = g_list_prepend (p, details);
		}
	}

	return g_list_reverse (p);
}
