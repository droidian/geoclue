/*
 * Geoclue
 * gc-iface-position.h - GInterface for org.freedesktop.Geoclue.Position
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GC_IFACE_POSITION_H
#define _GC_IFACE_POSITION_H

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GC_TYPE_IFACE_POSITION (gc_iface_position_get_type ())
#define GC_IFACE_POSITION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_IFACE_POSITION, GcIfacePosition))
#define GC_IFACE_POSITION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_IFACE_POSITION, GcIfacePositionClass))
#define GC_IS_IFACE_POSITION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_IFACE_POSITION))
#define GC_IS_IFACE_POSITION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_IFACE_POSITION))
#define GC_IFACE_POSITION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GC_TYPE_IFACE_POSITION, GcIfacePositionClass))

typedef struct _GcIfacePosition GcIfacePosition; /* Dummy typedef */
typedef struct _GcIfacePositionClass GcIfacePositionClass;

struct _GcIfacePositionClass {
        GTypeInterface base_iface;

	/* signals */
	void (* position_changed) (GcIfacePosition      *gc,
				   GeocluePositionFields fields,
				   int                   timestamp,
				   double                latitude,
				   double                longitude,
				   double                altitude,
				   GeoclueAccuracy      *accuracy);

	/* vtable */
	gboolean (* get_position) (GcIfacePosition       *gc,
				   GeocluePositionFields *fields,
				   int                   *timestamp,
				   double                *latitude,
				   double                *longitude,
				   double                *altitude,
				   GeoclueAccuracy      **accuracy,
				   GError               **error);
};

GType gc_iface_position_get_type (void);

void gc_iface_position_emit_position_changed (GcIfacePosition      *gc,
					      GeocluePositionFields fields,
					      int                   timestamp,
					      double                latitude,
					      double                longitude,
					      double                altitude,
					      GeoclueAccuracy      *accuracy);

G_END_DECLS

#endif
