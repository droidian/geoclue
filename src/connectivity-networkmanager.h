/*
 * Geoclue
 * connectivity-networkmanager.h
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _CONNECTIVITY_NETWORKMANAGER_H
#define _CONNECTIVITY_NETWORKMANAGER_H

#include <glib-object.h>
#include "connectivity.h"


G_BEGIN_DECLS

#define GEOCLUE_TYPE_NETWORKMANAGER (geoclue_networkmanager_get_type ())
#define GEOCLUE_NETWORKMANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_NETWORKMANAGER, GeoclueNetworkManager))

typedef struct {
	GObject parent;
	
	/* private */
	GeoclueNetworkStatus status;
	DBusGConnection *connection;
} GeoclueNetworkManager;

typedef struct {
	GObjectClass parent_class;
} GeoclueNetworkManagerClass;

GType geoclue_networkmanager_get_type (void);

G_END_DECLS

#endif
