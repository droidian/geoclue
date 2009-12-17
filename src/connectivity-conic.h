/*
 * Geoclue
 * connectivity-conic.h
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

#ifndef _CONNECTIVITY_CONIC_H
#define _CONNECTIVITY_CONIC_H

#include <glib-object.h>
#include <conicconnection.h>
#include "connectivity.h"

G_BEGIN_DECLS

#define GEOCLUE_TYPE_CONIC (geoclue_conic_get_type ())
#define GEOCLUE_CONIC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_CONIC, GeoclueConic))

typedef struct {
	GObject parent;
	
	/* private */
	GeoclueNetworkStatus status;
	ConIcConnection *conic;
} GeoclueConic;

typedef struct {
	GObjectClass parent_class;
} GeoclueConicClass;

GType geoclue_conic_get_type (void);

G_END_DECLS

#endif
