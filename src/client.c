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
#include <geoclue/gc-iface-address.h>

#include "client.h"

static void gc_master_client_position_init (GcIfacePositionClass *iface);
static void gc_master_client_address_init (GcIfaceAddressClass *iface);

G_DEFINE_TYPE_WITH_CODE (GcMasterClient, gc_master_client, 
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						gc_master_client_position_init)
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
						gc_master_client_address_init))

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient        *client,
					 GeoclueAccuracyLevel   min_accuracy,
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

static void
position_changed (GcMasterProvider     *provider,
                  GeocluePositionFields fields,
                  int                   timestamp,
                  double                latitude,
                  double                longitude,
                  double                altitude,
                  GeoclueAccuracy      *accuracy,
                  GcMasterClient       *client)
{
	gc_iface_position_emit_position_changed
		(GC_IFACE_POSITION (client),
		 fields,
		 timestamp,
		 latitude, longitude, altitude,
		 accuracy);
}

static void
address_changed (GcMasterProvider     *provider,
                 int                   timestamp,
                 GHashTable           *details,
                 GeoclueAccuracy      *accuracy,
                 GcMasterClient       *client)
{
	gc_iface_address_emit_address_changed
		(GC_IFACE_ADDRESS (client),
		 timestamp,
		 details,
		 accuracy);
}

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient        *client,
					 GeoclueAccuracyLevel   min_accuracy,
					 int                    min_time,
					 gboolean               require_updates,
					 GeoclueResourceFlags   allowed_resources,
					 GError               **error)
{
	
	GList *providers = NULL;
	
	client->min_accuracy = min_accuracy;
	client->min_time = min_time;
	client->require_updates = require_updates;
	client->allowed_resources = allowed_resources;
	
	/* position provider */
	providers = gc_master_get_providers (GC_IFACE_POSITION,
	                                     min_accuracy,
	                                     require_updates,
	                                     allowed_resources,
	                                     NULL);
	g_debug ("%d position providers matching requirements found", g_list_length (providers));
	if (!providers) {
		// TODO: should have a return value to indicate provider existence?
	} else {
		
		/* choose the most accurate provider (first one) */
		client->position_provider = providers->data;
		
		g_signal_connect (G_OBJECT (client->position_provider),
				  "position-changed",
				  G_CALLBACK (position_changed),
				  client);
		
		g_list_free (providers);
	}
	
	/* Address provider */
	providers = gc_master_get_providers (GC_IFACE_ADDRESS,
	                                     min_accuracy,
	                                     require_updates,
	                                     allowed_resources,
	                                     NULL);
	g_debug ("%d address providers matching requirements found", g_list_length (providers));
	if (!providers) {
		// TODO: should have a return value to indicate provider existence?
	} else {
		
		/* choose the most accurate provider (first one) */
		client->address_provider = providers->data;
		
		g_signal_connect (G_OBJECT (client->address_provider),
				  "address-changed",
				  G_CALLBACK (address_changed),
				  client);
		
		g_list_free (providers);
	}
	
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
get_position (GcIfacePosition       *iface,
	      GeocluePositionFields *fields,
	      int                   *timestamp,
	      double                *latitude,
	      double                *longitude,
	      double                *altitude,
	      GeoclueAccuracy      **accuracy,
	      GError               **error)
{
	GcMasterClient *client = GC_MASTER_CLIENT (iface);
	/*TODO: should maybe set sensible defaults and get providers, 
	 * if set_requirements has not been called?? */
	
	if (client->position_provider == NULL) {
		/* TODO: set error*/
		g_warning ("get_position called, but no provider available");
		return FALSE;
	}
	
	*fields = gc_master_provider_get_position
		(client->position_provider,
		 timestamp,
		 latitude, longitude, altitude,
		 accuracy,
		 error);
	return TRUE;
}

static gboolean 
get_address (GcIfaceAddress   *iface,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GcMasterClient *client = GC_MASTER_CLIENT (iface);
	
	if (client->address_provider == NULL) {
		/* TODO: set error*/
		g_warning ("get_position called, but no provider available");
		return FALSE;
	}
	
	return gc_master_provider_get_address
		(client->address_provider,
		 timestamp,
		 address,
		 accuracy,
		 error);
}

static void
gc_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}

static void
gc_master_client_address_init (GcIfaceAddressClass *iface)
{
	iface->get_address = get_address;
}
