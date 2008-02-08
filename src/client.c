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
gc_iface_master_client_set_requirements (GcMasterClient      *client,
					 GeoclueAccuracy     *accuracy,
					 int                  min_time,
					 gboolean             require_updates,
					 GError             **error);

#include "gc-iface-master-client-glue.h"

static void
finalize (GObject *object)
{
	((GObjectClass *) gc_master_client_parent_class)->finalize (object);
}

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient      *client,
					 GeoclueAccuracy     *accuracy,
					 int                  min_time,
					 gboolean             require_updates,
					 GError             **error)
{
	/* get_providers here, choose which one to use */
	
	/*
	GList *providers = NULL;
	PositionInterface *p;
	
	client->desired_accuracy = geoclue_accuracy_copy (accuracy);
	client->min_time = min_time;
	client->require_updates = require_updates;
	
	providers = gc_master_get_providers (client->desired_accuracy,
	                                     require_updates,
	                                     GEOCLUE_REQUIRE_FLAGS_NETWORK, ///FIXME should be in the DBus API
	                                     NULL);
	if (!providers) {
		// error?
		return TRUE;
	}
	
	// select which provider/interface to use out of the possible ones
	p = providers->data;
	g_list_free (providers);
	
	int i=0;
	ProviderInterface *iface;
	for (i = 0; i < p->interfaces->len; i++) {
		iface = g_ptr_array_index (p->interfaces, i);
		if (iface->type == POSITION_INTERFACE) {
			client->iface = iface;
			break;
		}
	}
	*/
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
	/*TODO: should set sensible defaults and get providers ? */
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
	
	if (client->position_provider == NULL) {
		return FALSE;
	}
	
	/* FIXME: assuming this is up-to-date */
	
	/*
	*timestamp = client->pos_iface->timestamp;
	*latitude = client->pos_iface->latitude;
	*longitude = client->pos_iface->longitude;
	*altitude = client->pos_iface->altitude;
	*fields = client->pos_iface->fields;
	*accuracy = geoclue_accuracy_copy (client->pos_iface->accuracy);
	*/
	
	return TRUE;
}

static void
gc_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}
