/*
 * Geoclue
 * gc-iface-velocity.h - GInterface for org.freedesktop.Geoclue.Velocity
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

#ifndef _GC_IFACE_VELOCITY_H
#define _GC_IFACE_VELOCITY_H

#include <geoclue/geoclue-types.h>

G_BEGIN_DECLS

#define GC_TYPE_IFACE_VELOCITY (gc_iface_velocity_get_type ())
#define GC_IFACE_VELOCITY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_IFACE_VELOCITY, GcIfaceVelocity))
#define GC_IFACE_VELOCITY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_IFACE_VELOCITY, GcIfaceVelocityClass))
#define GC_IS_IFACE_VELOCITY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_IFACE_VELOCITY))
#define GC_IS_IFACE_VELOCITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_IFACE_VELOCITY))
#define GC_IFACE_VELOCITY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GC_TYPE_IFACE_VELOCITY, GcIfaceVelocityClass))

typedef struct _GcIfaceVelocity GcIfaceVelocity; /* Dummy typedef */
typedef struct _GcIfaceVelocityClass GcIfaceVelocityClass;

struct _GcIfaceVelocityClass {
	GTypeInterface base_iface;

	/* signals */
	void (* velocity_changed) (GcIfaceVelocity      *gc,
				   GeoclueVelocityFields fields,
				   int                   timestamp,
				   double                speed,
				   double                direction,
				   double                climb);

	/* vtable */
	gboolean (* get_velocity) (GcIfaceVelocity       *gc,
				   GeoclueVelocityFields *fields,
				   int                   *timestamp,
				   double                *speed,
				   double                *direction,
				   double                *climb,
				   GError               **error);
};

GType gc_iface_velocity_get_type (void);

void gc_iface_velocity_emit_velocity_changed (GcIfaceVelocity      *gc,
					      GeoclueVelocityFields fields,
					      int                   timestamp,
					      double                speed,
					      double                direction,
					      double                climb);

G_END_DECLS

#endif
