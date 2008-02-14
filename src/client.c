/*
 * Geoclue
 * client.c - Geoclue Master Client
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>

#include "client.h"

static void gc_master_client_position_init (GcIfacePositionClass *iface);

G_DEFINE_TYPE_WITH_CODE (GcMasterClient, gc_master_client, 
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						gc_master_client_position_init))

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient        *client,
					 GeoclueAccuracy       *accuracy,
					 int                    min_time,
					 gboolean               require_updates,
					 GeoclueResourceFlags   allowed_resources,
					 GError               **error);

#include "gc-iface-master-client-glue.h"

static void
finalize (GObject *object)
{
	((GObjectClass *) gc_master_client_parent_class)->finalize (object);
}

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient        *client,
					 GeoclueAccuracy       *accuracy,
					 int                    min_time,
					 gboolean               require_updates,
					 GeoclueResourceFlags   allowed_resources,
					 GError               **error)
{
	/* get_providers here, choose which one to use */
	
	GList *providers = NULL;
	
	client->desired_accuracy = geoclue_accuracy_copy (accuracy);
	client->min_time = min_time;
	client->require_updates = require_updates;
	client->allowed_resources = allowed_resources;
	
	providers = gc_master_get_position_providers (client->desired_accuracy,
	                                              require_updates,
	                                              allowed_resources,
	                                              NULL);
	if (!providers) {
		// error?
		return TRUE;
	}
	
	/* TODO select which provider/interface to use out of the possible ones
	 * Now using the first one */
	client->position_provider = providers->data;
	g_print ("Requirements set, provider '%s' chosen\n", client->position_provider->name);
	g_list_free (providers);
	
	return TRUE;
}

static void
gc_master_client_class_init (GcMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;

	dbus_g_object_type_install_info (gc_master_client_get_type (),
					 &dbus_glib_gc_iface_master_client_object_info);
}

static void
gc_master_client_init (GcMasterClient *client)
{
}

static gboolean
get_position (GcIfacePosition       *gc,
	      GeocluePositionFields *fields,
	      int                   *timestamp,
	      double                *latitude,
	      double                *longitude,
	      double                *altitude,
	      GeoclueAccuracy      **accuracy,
	      GError               **error)
{
	GcMasterClient *client = GC_MASTER_CLIENT (gc);
	PositionInterface *iface;
		
	/*TODO: should maybe set sensible defaults and get providers, 
	 * if set_requirements has not been called?? */
	
	if (client->position_provider == NULL) {
		return FALSE;
	}
	
	iface = client->position_provider->position;
	if (client->position_provider->provides & GEOCLUE_PROVIDE_FLAGS_UPDATES) {
		
		*timestamp = iface->timestamp;
		*latitude = iface->latitude;
		*longitude = iface->longitude;
		*altitude = iface->altitude;
		*fields = iface->fields;
		*accuracy = geoclue_accuracy_copy (iface->accuracy);
	} else {
		/* the data in iface is not necessarily up-to-date */
		*fields = geoclue_position_get_position (iface->position,
		                                         timestamp,
		                                         latitude,
		                                         longitude,
		                                         altitude,
		                                         accuracy,
		                                         error);
	}
	return TRUE;
}

static void
gc_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}
