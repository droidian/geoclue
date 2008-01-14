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

#define GEOCLUE_TYPE_MASTER (geoclue_master_get_type ())
#define GEOCLUE_MASTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_MASTER, GeoclueMaster))

typedef struct {
	GObject parent;

	GMainLoop *loop;
	GList *providers;
	DBusGConnection *connection;
} GeoclueMaster;

typedef struct {
	GObjectClass parent_class;
} GeoclueMasterClass;

GType geoclue_master_get_type (void);

#endif
