/*
 * Geoclue
 * master.h - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#ifndef _MASTER_H_
#define _MASTER_H_

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-velocity.h>

#define POSITION_IFACE "org.freedesktop.Geoclue.Position"
#define VELOCITY_IFACE "org.freedesktop.Geoclue.Velocity"

typedef enum _InterfaceType {
	POSITION_INTERFACE,
	VELOCITY_INTERFACE,
} InterfaceType;

typedef enum _GeoclueRequireFlags {
	GEOCLUE_REQUIRE_FLAGS_NONE = 0,
	GEOCLUE_REQUIRE_FLAGS_NETWORK = 1 << 0,
	GEOCLUE_REQUIRE_FLAGS_GPS = 1 << 1,
} GeoclueRequireFlags;

typedef enum _GeoclueProvideFlags {
	GEOCLUE_PROVIDE_FLAGS_NONE = 0,
	GEOCLUE_PROVIDE_FLAGS_UPDATES = 1 << 0,
	GEOCLUE_PROVIDE_FLAGS_DETAILED = 1 << 1,
	GEOCLUE_PROVIDE_FLAGS_FUZZY = 1 << 2,
} GeoclueProvideFlags;

typedef struct _ProviderInterface {
	InterfaceType type;

	union {
		struct {
			GeocluePosition *position;
			GeocluePositionFields fields;

			int timestamp; /* Last time details was updated */

			double latitude;
			double longitude;
			double altitude;
			
			GeoclueAccuracy *accuracy;
		} position;

		struct {
			GeoclueVelocity *velocity;
			GeoclueVelocityFields fields;

			int timestamp; /* Last time details was updated */

			double speed;
			double direction;
			double climb;
		} velocity;
	} details;
} ProviderInterface;

typedef struct _ProviderDetails {
	char *name;
	char *service;
	char *path;

	GeoclueRequireFlags requires;
	GeoclueProvideFlags provides;

	GeoclueCommon *geoclue;
	GeoclueStatus status;

	GPtrArray *interfaces;
} ProviderDetails;

#define GC_TYPE_MASTER (gc_master_get_type ())
#define GC_MASTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_MASTER, GcMaster))

typedef struct {
	GObject parent;

	GMainLoop *loop;
	DBusGConnection *connection;
} GcMaster;

typedef struct {
	GObjectClass parent_class;
} GcMasterClass;

GType gc_master_get_type (void);
GList *gc_master_get_providers (GeoclueAccuracy *accuracy,
				gboolean         can_update,
				GError         **error);

#endif
