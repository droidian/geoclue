/*
 * Geoclue
 * geoclue-velocity.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
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


G_END_DECLS

#endif
