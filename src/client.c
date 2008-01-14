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

static void
finalize (GObject *object)
{
	((GObjectClass *) geoclue_master_client_parent_class)->finalize (object);
}

static void
geoclue_master_client_class_init (GeoclueMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
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
