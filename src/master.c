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

G_DEFINE_TYPE (GeoclueMaster, geoclue_master, G_TYPE_OBJECT);

static GList *providers = NULL;

static gboolean gc_iface_master_create (GeoclueMaster *master,
					const char   **object_path,
					GError       **error);
static gboolean gc_iface_master_shutdown (GeoclueMaster *master,
					  GError       **error);

#include "gc-iface-master-glue.h"

#define GEOCLUE_MASTER_PATH "/org/freedesktop/Geoclue/Master/client"
static gboolean
gc_iface_master_create (GeoclueMaster *master,
			const char   **object_path,
			GError       **error)
{
	static guint32 serial = 0;
	GeoclueMasterClient *client;
	char *path;

	path = g_strdup_printf ("%s%d", GEOCLUE_MASTER_PATH, serial++);
	client = g_object_new (GEOCLUE_TYPE_MASTER_CLIENT, NULL);
	dbus_g_connection_register_g_object (master->connection, path,
					     G_OBJECT (client));

	*object_path = path;
	return TRUE;
}

static gboolean
gc_iface_master_shutdown (GeoclueMaster *master,
			  GError       **error)
{
	return TRUE;
}

static void
geoclue_master_class_init (GeoclueMasterClass *klass)
{
	dbus_g_object_type_install_info (geoclue_master_get_type (),
					 &dbus_glib_gc_iface_master_object_info);
}

static GeoclueRequireFlags
parse_require_strings (GeoclueMaster *master,
		       char         **flags)
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
parse_provide_strings (GeoclueMaster *master,
		       char         **flags)
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

static GPtrArray *
parse_interface_strings (GeoclueMaster *master,
			 char         **interfaces,
			 guint          n_interfaces)
{
	GPtrArray *ifaces;
	int i;

	ifaces = g_ptr_array_sized_new (n_interfaces);
	for (i = 0; interfaces[i]; i++) {
		struct _ProviderInterface *iface;

		iface = g_new0 (struct _ProviderInterface, 1);

		if (strcmp (interfaces[i], POSITION_IFACE) == 0) {
			iface->type = POSITION_INTERFACE;
		} else if (strcmp (interfaces[i], VELOCITY_IFACE) == 0) {
			iface->type = VELOCITY_INTERFACE;
		} else {
			g_free (iface);
			continue;
		}

		g_ptr_array_add (ifaces, iface);
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
new_provider (GeoclueMaster *master,
	      const char    *filename)
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
	provider = g_new (struct _ProviderDetails, 1);
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

	interfaces = g_key_file_get_string_list (keyfile, "Geoclue Provider",
						 "Interfaces", 
						 &n_interfaces, NULL);
	if (interfaces != NULL) {
		provider->interfaces = parse_interface_strings (master, 
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
load_providers (GeoclueMaster *master)
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

static void
geoclue_master_init (GeoclueMaster *master)
{
	GError *error = NULL;

	master->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (master->connection == NULL) {
		g_warning ("Could not get %s: %s", GEOCLUE_DBUS_BUS, 
			   error->message);
		g_error_free (error);
	}

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
geoclue_master_get_providers (GeoclueAccuracy *accuracy,
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
