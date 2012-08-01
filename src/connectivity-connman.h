/*
 * Geoclue
 * connectivity-connman.h
 *
 * Author: Javier Fernandez <jfernandez@igalia.com>
 * Copyright (C) 2010 Igalia S.L
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

#ifndef _CONNECTIVITY_CONNMAN_H
#define _CONNECTIVITY_CONNMAN_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "connectivity.h"

G_BEGIN_DECLS

#define GEOCLUE_TYPE_CONNMAN (geoclue_connman_get_type ())
#define GEOCLUE_CONNMAN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_CONNMAN, GeoclueConnman))

typedef struct {
	GObject parent;

	/* private */
	GeoclueNetworkStatus status;
	DBusGConnection *conn;
	DBusGProxy *client;
	gchar *cache_ap_mac;
	int ap_strength;
} GeoclueConnman;

typedef struct {
	GObjectClass parent_class;
} GeoclueConnmanClass;

GType geoclue_connman_get_type (void);

G_END_DECLS

#endif
