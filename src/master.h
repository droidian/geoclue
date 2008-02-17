/*
 * Geoclue
 * master.h - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#ifndef _MASTER_H_
#define _MASTER_H_

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-accuracy.h>
#include "connectivity.h"

#define GC_TYPE_MASTER (gc_master_get_type ())
#define GC_MASTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_MASTER, GcMaster))

typedef struct {
	GObject parent;
	
	GMainLoop *loop;
	DBusGConnection *connection;
	GeoclueConnectivity *connectivity;
} GcMaster;

typedef struct {
	GObjectClass parent_class;
} GcMasterClass;

GType gc_master_get_type (void);
GList *gc_master_get_position_providers (GeoclueAccuracy      *accuracy,
					 gboolean              can_update,
					 GeoclueResourceFlags  allowed,
					 GError              **error);

#endif
	
