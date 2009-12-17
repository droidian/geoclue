/*
 * Geoclue
 * geoclue-master.h -
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
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

#ifndef _GEOCLUE_MASTER_H
#define _GEOCLUE_MASTER_H

#include <glib-object.h>
#include <geoclue/geoclue-master-client.h>

G_BEGIN_DECLS

#define GEOCLUE_MASTER_DBUS_SERVICE "org.freedesktop.Geoclue.Master"
#define GEOCLUE_MASTER_DBUS_PATH "/org/freedesktop/Geoclue/Master"
#define GEOCLUE_MASTER_DBUS_INTERFACE "org.freedesktop.Geoclue.Master"

#define GEOCLUE_TYPE_MASTER (geoclue_master_get_type ())
#define GEOCLUE_MASTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_MASTER, GeoclueMaster))
#define GEOCLUE_IS_MASTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_MASTER))

typedef struct _GeoclueMaster {
	GObject parent;
} GeoclueMaster;

typedef struct _GeoclueMasterClass {
	GObjectClass parent_class;
} GeoclueMasterClass;

GType geoclue_master_get_type (void);

GeoclueMaster *geoclue_master_get_default (void);

GeoclueMasterClient *geoclue_master_create_client (GeoclueMaster *master,
						   char         **object_path,
						   GError       **error);
typedef void (*GeoclueCreateClientCallback) (GeoclueMaster       *master,
					     GeoclueMasterClient *client,
					     char                *object_path,
					     GError              *error,
					     gpointer             userdata);
void geoclue_master_create_client_async (GeoclueMaster              *master,
					 GeoclueCreateClientCallback callback,
					 gpointer                    userdata);

G_END_DECLS

#endif
