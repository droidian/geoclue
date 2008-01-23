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

static void geoclue_master_client_position_init (GcIfacePositionClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueMasterClient, geoclue_master_client, 
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						geoclue_master_client_position_init))

static gboolean
gc_iface_master_client_set_requirements (GeoclueMasterClient *client,
					 GeoclueAccuracy     *accuracy,
					 int                  min_time,
					 gboolean             require_updates,
					 GError             **error);

#include "gc-iface-master-client-glue.h"

static void
finalize (GObject *object)
{
	((GObjectClass *) geoclue_master_client_parent_class)->finalize (object);
}

static gboolean
gc_iface_master_client_set_requirements (GeoclueMasterClient *client,
					 GeoclueAccuracy     *accuracy,
					 int                  min_time,
					 gboolean             require_updates,
					 GError             **error)
{
	client->desired_accuracy = geoclue_accuracy_copy (accuracy);
	client->min_time = min_time;
	client->require_updates = require_updates;

	return TRUE;
}

static void
geoclue_master_client_class_init (GeoclueMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;

	dbus_g_object_type_install_info (geoclue_master_client_get_type (),
					 &dbus_glib_gc_iface_master_client_object_info);
}

static void
geoclue_master_client_init (GeoclueMasterClient *client)
{
}

static void
position_changed_cb (GeocluePosition      *position,
		     GeocluePositionFields fields,
		     int                   timestamp,
		     double                latitude,
		     double                longitude,
		     double                altitude,
		     GeoclueAccuracy      *accuracy,
		     GeoclueMasterClient  *client)
{
}

static void
velocity_changed_cb (GeoclueVelocity      *velocity,
		     GeoclueVelocityFields fields,
		     int                   timestamp,
		     double                speed,
		     double                direction,
		     double                climb,
		     GeoclueMasterClient  *client)
{
}

static ProviderInterface *
find_interface (ProviderDetails *provider,
		InterfaceType    type,
		GError         **error)
{
	int i;

	for (i = 0; i < provider->interfaces->len; i++) {
		ProviderInterface *interface = provider->interfaces->pdata[i];

		if (interface->type == type) {
			return interface;
		}
	}

	/* FIXME: Setup error */
	return NULL;
}

static gboolean
setup_interface (GeoclueMasterClient *client,
		 ProviderInterface   *interface,
		 GError             **error)
{
	switch (interface->type) {
	case POSITION_INTERFACE:
		if (interface->details.position.position == NULL) {
			interface->details.position.position = 
				geoclue_position_new (client->provider->service,
						      client->provider->path);
		}

		if (client->require_updates) {
			client->update_id = g_signal_connect 
				(interface->details.position.position,
				 "position-changed", 
				 G_CALLBACK (position_changed_cb), client);
		}

		interface->details.position.fields = 
			geoclue_position_get_position 
			(interface->details.position.position,
			 &interface->details.position.timestamp,
			 &interface->details.position.latitude,
			 &interface->details.position.longitude,
			 &interface->details.position.altitude,
			 &interface->details.position.accuracy,
			 error);
		break;

	case VELOCITY_INTERFACE:
		if (interface->details.position.position == NULL) {
			interface->details.velocity.velocity = 
				geoclue_velocity_new (client->provider->service,
						      client->provider->path);
		}

		if (client->require_updates) {
			client->update_id = g_signal_connect 
				(interface->details.velocity.velocity,
				 "position-changed", 
				 G_CALLBACK (velocity_changed_cb), client);
		}

		interface->details.velocity.fields =
			geoclue_velocity_get_velocity
			(interface->details.velocity.velocity,
			 &interface->details.velocity.timestamp,
			 &interface->details.velocity.speed,
			 &interface->details.velocity.direction,
			 &interface->details.velocity.climb,
			 error);
		break;

	default:
		break;
	}

	return TRUE;
}

static gboolean
setup_provider (GeoclueMasterClient *client,
		ProviderInterface  **interface,
		GError             **error)
{
	client->provider = geoclue_master_get_best_provider (client->desired_accuracy,
							     client->require_updates,
							     error);
	if (client->provider == NULL) {
		return FALSE;
	}

	*interface = find_interface (client->provider, POSITION_INTERFACE,
				    error);
	if (*interface == NULL) {
		return FALSE;
	}

	if (setup_interface (client, *interface, error) == FALSE) {
		return FALSE;
	}

	return TRUE;
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
	GeoclueMasterClient *client = GEOCLUE_MASTER_CLIENT (gc);
	ProviderInterface *interface = NULL;

	if (client->provider == NULL) {
		if (setup_provider (client, &interface, error) == FALSE) {
			return FALSE;
		}

		g_print ("Using %s\n", client->provider->name);
	}

	if (interface == NULL) {
		return FALSE;
	}

	/* How to handle case where we haven't got details yet? */
	*fields = interface->details.position.fields;
	*timestamp = interface->details.position.timestamp;
	*latitude = interface->details.position.latitude;
	*longitude = interface->details.position.longitude;
	*altitude = interface->details.position.altitude;
	*accuracy = interface->details.position.accuracy;

	return TRUE;
}

static void
geoclue_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}
