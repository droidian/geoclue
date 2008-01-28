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
	client->desired_accuracy = geoclue_accuracy_copy (accuracy);
	client->min_time = min_time;
	client->require_updates = require_updates;

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

static void
position_changed_cb (GeocluePosition      *position,
		     GeocluePositionFields fields,
		     int                   timestamp,
		     double                latitude,
		     double                longitude,
		     double                altitude,
		     GeoclueAccuracy      *accuracy,
		     GcMasterClient       *client)
{
}

static void
velocity_changed_cb (GeoclueVelocity      *velocity,
		     GeoclueVelocityFields fields,
		     int                   timestamp,
		     double                speed,
		     double                direction,
		     double                climb,
		     GcMasterClient       *client)
{
}

static ProviderInterface *
find_interface (ProviderDetails *provider,
		InterfaceType    type,
		GError         **error)
{
	int i;

	if (provider->interfaces == NULL) {
		return NULL;
	}

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
provider_is_available (ProviderDetails *provider)
{
	GeoclueStatus status;
	GError *error = NULL;

	if (!geoclue_common_get_status (provider->geoclue, &status, &error)) {
		g_warning ("Error getting status: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	return (status != GEOCLUE_STATUS_UNAVAILABLE);
}

static gboolean
setup_interface (GcMasterClient      *client,
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
		if (*error != NULL) {
			g_print ("Error: %s\n", (*error)->message);
			return FALSE;
		}

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

static void
setup_provider (GcMasterClient  *client,
		ProviderDetails *provider)
{
	/* We need the Geoclue interface to know the status of the object
	   but if someone else has created the object then we don't want to
	   destroy it on them if we shut this provider down */
	if (provider->geoclue == NULL) {
		provider->geoclue = geoclue_common_new (provider->service,
							provider->path);
		g_object_add_weak_pointer (G_OBJECT (provider->geoclue),
					   (gpointer) &provider->geoclue);
	} else {
		g_object_ref (provider->geoclue);
	}
}

static void
shutdown_provider (GcMasterClient  *client,
		   ProviderDetails *provider)
{
	g_object_unref (provider->geoclue);
}

static gboolean
find_provider (GcMasterClient      *client,
	       ProviderInterface  **interface,
	       GError             **error)
{
	GList *providers, *p;
	GError *sub_error = NULL;

	providers = gc_master_get_providers (client->desired_accuracy,
					     client->require_updates,
					     &sub_error);
	if (providers == NULL) {
		return FALSE;
	}

	for (p = providers; p; p = p->next) {
		ProviderDetails *provider = p->data;

		setup_provider (client, provider);

		/* Find interface before testing availablity as it doesn't
		   require a process to be started up */
		*interface = find_interface (provider, POSITION_INTERFACE,
					     &sub_error);
		if (*interface == NULL) {
			g_print ("Skipped %s - no interface\n", provider->name);
			shutdown_provider (client, provider);
			continue;
		}

		if (provider_is_available (provider) == FALSE) {
			g_print ("Skipped %s - not available\n", 
				 provider->name);
			shutdown_provider (client, provider);
			continue;
		}

		client->provider = provider;
		if (setup_interface (client, *interface, &sub_error)) {
			g_list_free (providers);
			return TRUE;
		} else {
			client->provider = NULL;
			g_print ("Skipped %s\n", provider->name);
			if (sub_error) {
				g_print ("Error - %s\n", sub_error->message);
				g_error_free (sub_error);
				sub_error = NULL;
			}

			shutdown_provider (client, provider);
		}
	}

	g_list_free (providers);
	client->provider = NULL;
	*interface = NULL;
	return FALSE;
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
	ProviderInterface *interface = NULL;

	g_print ("Getting position\n");
	if (client->provider == NULL) {
		if (find_provider (client, &interface, error) == FALSE) {
			g_print ("Error...\n");
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
	/* *accuracy = interface->details.position.accuracy; */
	*accuracy = geoclue_accuracy_new (0, 0, 0);

	return TRUE;
}

static void
gc_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}
