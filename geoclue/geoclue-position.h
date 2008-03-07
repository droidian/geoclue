/*
 * Geoclue
 * geoclue-position.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
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


G_END_DECLS

#endif
