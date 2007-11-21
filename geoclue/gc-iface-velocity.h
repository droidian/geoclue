/*
 * Geoclue
 * gc-iface-velocity.h - GInterface for org.freedesktop.Geoclue.Velocity
 *
 * Author: Iain Holmes <iain@openedhand.com>
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

typedef enum {
	GEOCLUE_VELOCITY_FIELDS_NONE = 0,
	GEOCLUE_VELOCITY_FIELDS_LATITUDE = 1 << 0,
	GEOCLUE_VELOCITY_FIELDS_LONGITUDE = 1 << 1,
	GEOCLUE_VELOCITY_FIELDS_ALTITUDE = 1 << 2
} GeoclueVelocityFields;

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

void gc_iface_velocity_changed (GcIfaceVelocity      *gc,
				GeoclueVelocityFields fields,
				int                   timestamp,
				double                speed,
				double                direction,
				double                climb);

G_END_DECLS

#endif
