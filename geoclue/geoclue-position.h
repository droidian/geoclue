/*
 * Geoclue
 * geoclue-position.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
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

#ifndef _GEOCLUE_POSITION_H
#define _GEOCLUE_POSITION_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_POSITION (geoclue_position_get_type ())
#define GEOCLUE_POSITION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_POSITION, GeocluePosition))
#define GEOCLUE_IS_POSITION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_POSITION))

#define GEOCLUE_POSITION_INTERFACE_NAME "org.freedesktop.Geoclue.Position"

typedef struct _GeocluePosition {
	GeoclueProvider provider;
} GeocluePosition;

typedef struct _GeocluePositionClass {
	GeoclueProviderClass provider_class;

	void (* position_changed) (GeocluePosition      *position,
				   GeocluePositionFields fields,
				   int                   timestamp,
				   double                latitude,
				   double                longitude,
				   double                altitude,
				   GeoclueAccuracy      *accuracy);
} GeocluePositionClass;

GType geoclue_position_get_type (void);

GeocluePosition *geoclue_position_new (const char *service,
				       const char *path);

GeocluePositionFields geoclue_position_get_position (GeocluePosition  *position,
						     int              *timestamp,
						     double           *latitude,
						     double           *longitude,
						     double           *altitude,
						     GeoclueAccuracy **accuracy,
						     GError          **error);

typedef void (*GeocluePositionCallback) (GeocluePosition      *position,
					 GeocluePositionFields fields,
					 int                   timestamp,
					 double                latitude,
					 double                longitude,
					 double                altitude,
					 GeoclueAccuracy      *accuracy,
					 GError               *error,
					 gpointer              userdata);

void geoclue_position_get_position_async (GeocluePosition         *position,
					  GeocluePositionCallback  callback,
					  gpointer                 userdata);

G_END_DECLS

#endif
