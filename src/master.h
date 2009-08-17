/*
 * Geoclue
 * master.h - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _MASTER_H_
#define _MASTER_H_

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>
#include "connectivity.h"
#include "master-provider.h"

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
GList *gc_master_get_providers (GcInterfaceFlags      iface_type,
				GeoclueAccuracyLevel  min_accuracy,
				gboolean              can_update,
				GeoclueResourceFlags  allowed,
				GError              **error);

#endif
	
