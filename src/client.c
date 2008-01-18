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
gc_iface_master_client_set_accuracy (GeoclueMasterClient *client,
				     GeoclueAccuracy     *accuracy,
				     GError             **error);
static gboolean
gc_iface_master_client_set_minimum_time (GeoclueMasterClient *client,
					 int                  min_time,
					 GError             **error);
static gboolean 
gc_iface_master_client_start_updates (GeoclueMasterClient *client,
				      GError             **error);
static gboolean
gc_iface_master_client_stop_updates (GeoclueMasterClient *client,
				     GError             **error);

#include "gc-iface-master-client-glue.h"

static void
finalize (GObject *object)
{
	((GObjectClass *) geoclue_master_client_parent_class)->finalize (object);
}

static gboolean
gc_iface_master_client_set_accuracy (GeoclueMasterClient *client,
				     GeoclueAccuracy     *accuracy,
				     GError             **error)
{
	client->desired_accuracy = geoclue_accuracy_copy (accuracy);
	return TRUE;
}

static gboolean
gc_iface_master_client_set_minimum_time (GeoclueMasterClient *client,
					 int                  min_time,
					 GError             **error)
{
	client->time_gap = min_time;
	return TRUE;
}

static gboolean 
gc_iface_master_client_start_updates (GeoclueMasterClient *client,
				      GError             **error)
{
	client->update = TRUE;
	return TRUE;
}

static gboolean
gc_iface_master_client_stop_updates (GeoclueMasterClient *client,
				     GError             **error)
{
	client->update = FALSE;
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
	return TRUE;
}

static void
geoclue_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}
