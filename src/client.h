/*
 * Geoclue
 * client.h - Master process client
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <glib-object.h>
#include <geoclue/geoclue-accuracy.h>

#include "master.h"

#define GC_TYPE_MASTER_CLIENT (gc_master_client_get_type ())
#define GC_MASTER_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_MASTER_CLIENT, GcMasterClient))

typedef struct {
	GObject parent;

	GeoclueAccuracy *desired_accuracy;
	int min_time;
	gboolean require_updates;
	GeoclueResourceFlags allowed_resources;
	
	guint32 update_id;
	ProviderDetails *position_provider;
} GcMasterClient;

typedef struct {
	GObjectClass parent_class;
} GcMasterClientClass;

GType gc_master_client_get_type (void);

#endif
