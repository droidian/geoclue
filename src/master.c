/*
 * Geoclue
 * master.c - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <string.h>

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

#include "master.h"
#include "client.h"

typedef enum _InterfaceType {
	POSITION_INTERFACE,
	COURSE_INTERFACE,
} InterfaceType;

typedef enum _GeoclueRequireFlags {
	GEOCLUE_REQUIRE_FLAGS_NONE = 0,
	GEOCLUE_REQUIRE_FLAGS_NETWORK = 1 << 0,
	GEOCLUE_REQUIRE_FLAGS_GPS = 1 << 1,
} GeoclueRequireFlags;

typedef enum _GeoclueProvideFlags {
	GEOCLUE_PROVIDE_FLAGS_NONE = 0,
	GEOCLUE_PROVIDE_FLAGS_UPDATES = 1 << 0,
} GeoclueProvideFlags;

struct _ProviderInterface {
	InterfaceType type;
	int timestamp; /* Last time details was updated */

	union {
		struct {
			GeocluePositionFields fields;

			double latitude;
			double longitude;
			double altitude;
			
			GeoclueAccuracy *accuracy;
		} position;

		struct {
			GeoclueVelocityFields fields;

			double speed;
			double direction;
			double climb;
		} course;
	} details;
};

struct _ProviderDetails {
	char *name;
	char *service;
	char *path;

	GeoclueRequireFlags requires;
	GeoclueProvideFlags provides;

	GPtrArray *interfaces;
};

G_DEFINE_TYPE (GeoclueMaster, geoclue_master, G_TYPE_OBJECT);

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
		}
	}

	return provides;
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
}

static void
dump_provider_details (struct _ProviderDetails *details)
{
	g_print ("   Name - %s\n", details->name);
	g_print ("   Service - %s\n", details->service);
	g_print ("   Path - %s\n", details->path);

	dump_requires (details);
	dump_provides (details);
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
	char **flags;

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
		g_print (" - %s\n", fullname);
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

	master->providers = load_providers (master);
}
