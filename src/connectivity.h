/*
 * Geoclue
 * geoclue-connectivity.h 
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
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

#ifndef _GEOCLUE_CONNECTIVITY_H
#define _GEOCLUE_CONNECTIVITY_H

#include <glib-object.h>
#include <geoclue/geoclue-types.h>

G_BEGIN_DECLS


#define GEOCLUE_TYPE_CONNECTIVITY (geoclue_connectivity_get_type ())
#define GEOCLUE_CONNECTIVITY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_CONNECTIVITY, GeoclueConnectivity))
#define GEOCLUE_IS_CONNECTIVITY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_CONNECTIVITY))
#define GEOCLUE_CONNECTIVITY_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GEOCLUE_TYPE_CONNECTIVITY, GeoclueConnectivityInterface))

typedef struct _GeoclueConnectivity GeoclueConnectivity;
typedef struct _GeoclueConnectivityInterface GeoclueConnectivityInterface;

struct _GeoclueConnectivityInterface {
	GTypeInterface parent;
	
	/* signals */
	void (* status_changed) (GeoclueConnectivity *self,
	                         GeoclueNetworkStatus status);
	
	/* vtable */
	int (*get_status) (GeoclueConnectivity *self);
	GHashTable * (*get_aps) (GeoclueConnectivity *self);
	char * (*get_ap_mac) (GeoclueConnectivity *self);
	char * (*get_router_mac) (GeoclueConnectivity *self);
};

GType geoclue_connectivity_get_type (void);

GeoclueConnectivity *geoclue_connectivity_new (void);

GeoclueNetworkStatus geoclue_connectivity_get_status (GeoclueConnectivity *self);

char *geoclue_connectivity_get_ap_mac (GeoclueConnectivity *self);
char *geoclue_connectivity_get_router_mac (GeoclueConnectivity *self);

GHashTable *geoclue_connectivity_get_aps (GeoclueConnectivity *self);

void
geoclue_connectivity_emit_status_changed (GeoclueConnectivity *self,
                                          GeoclueNetworkStatus status);

G_END_DECLS

#endif
