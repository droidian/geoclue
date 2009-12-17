/*
 * Geoclue
 * geoclue-velocity.h - 
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

#ifndef _GEOCLUE_VELOCITY_H
#define _GEOCLUE_VELOCITY_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_VELOCITY (geoclue_velocity_get_type ())
#define GEOCLUE_VELOCITY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_VELOCITY, GeoclueVelocity))
#define GEOCLUE_IS_VELOCITY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_VELOCITY))

#define GEOCLUE_VELOCITY_INTERFACE_NAME "org.freedesktop.Geoclue.Velocity"

typedef struct _GeoclueVelocity {
	GeoclueProvider provider;
} GeoclueVelocity;

typedef struct _GeoclueVelocityClass {
	GeoclueProviderClass provider_class;

	void (* velocity_changed) (GeoclueVelocity      *velocity,
				   GeoclueVelocityFields fields,
				   int                   timestamp,
				   double                speed,
				   double                direction,
				   double                climb);
} GeoclueVelocityClass;

GType geoclue_velocity_get_type (void);

GeoclueVelocity *geoclue_velocity_new (const char *service,
				       const char *path);

GeoclueVelocityFields geoclue_velocity_get_velocity (GeoclueVelocity  *velocity,
						     int              *timestamp,
						     double           *speed,
						     double           *direction,
						     double           *climb,
						     GError          **error);

typedef void (*GeoclueVelocityCallback) (GeoclueVelocity      *velocity,
					 GeoclueVelocityFields fields,
					 int                   timestamp,
					 double                speed,
					 double                direction,
					 double                climb,
					 GError               *error,
					 gpointer              userdata);

void geoclue_velocity_get_velocity_async (GeoclueVelocity         *velocity,
					  GeoclueVelocityCallback  callback,
					  gpointer                 userdata);

G_END_DECLS

#endif
