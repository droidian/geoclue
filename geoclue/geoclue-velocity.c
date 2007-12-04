/*
 * Geoclue
 * geoclue-velocity.c - Client API for accessing GcIfaceVelocity
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <geoclue/geoclue-velocity.h>
#include <geoclue/geoclue-marshal.h>

#include "gc-iface-velocity-bindings.h"

typedef struct _GeoclueVelocityPrivate {
	int dummy;
} GeoclueVelocityPrivate;

enum {
	VELOCITY_CHANGED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVASTE ((o), GEOCLUE_TYPE_VELOCITY, GeoclueVelocityPrivate))

G_DEFINE_TYPE (GeoclueVelocity, geoclue_velocity, GEOCLUE_TYPE_PROVIDER);

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (geoclue_velocity_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	G_OBJECT_CLASS (geoclue_velocity_parent_class)->dispose (object);
}

static void
velocity_changed (DBusGProxy      *proxy,
		  int              fields,
		  int              timestamp,
		  double           speed,
		  double           direction,
		  double           climb,
		  GeoclueVelocity *velocity)
{
	g_signal_emit (velocity, signals[VELOCITY_CHANGED], 0, fields,
		       timestamp, speed, direction, climb);
}

static GObject *
constructor (GType                  type,
	     guint                  n_props,
	     GObjectConstructParam *props)
{
	GObject *object;
	GeoclueProvider *provider;

	object = G_OBJECT_CLASS (geoclue_velocity_parent_class)->constructor 
		(type, n_props, props);
	provider = GEOCLUE_PROVIDER (object);

	dbus_g_proxy_add_signal (provider->proxy, "VelocityChanged",
				 G_TYPE_INT, G_TYPE_INT, G_TYPE_DOUBLE,
				 G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (provider->proxy, "VelocityChanged",
				     G_CALLBACK (velocity_changed),
				     object, NULL);

	return object;
}

static void
geoclue_velocity_class_init (GeoclueVelocityClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;

	g_type_class_add_private (klass, sizeof (GeoclueVelocityPrivate));

	signals[VELOCITY_CHANGED] = g_signal_new ("velocity-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GeoclueVelocityClass, velocity_changed),
						  NULL, NULL, 
						  geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE,
						  G_TYPE_NONE, 5,
						  G_TYPE_INT, G_TYPE_INT,
						  G_TYPE_DOUBLE, G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE);
}

static void
geoclue_velocity_init (GeoclueVelocity *velocity)
{
}

GeoclueVelocity *
geoclue_velocity_new (const char *service,
		      const char *path)
{
	return g_object_new (GEOCLUE_TYPE_VELOCITY,
			     "service", service,
			     "path", path,
			     "interface", GEOCLUE_VELOCITY_INTERFACE_NAME,
			     NULL);
}

/**
 * geoclue_velocity_get_velocity:
 * @velocity:
 * @timestamp:
 * @speed:
 * @direction:
 * @climb:
 * @error:
 *
 *
 * Return value:
 */
GeoclueVelocityFields
geoclue_velocity_get_velocity (GeoclueVelocity *velocity,
			       int             *timestamp,
			       double          *speed,
			       double          *direction,
			       double          *climb,
			       GError         **error)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (velocity);
	double sp, di, cl;
	int ts, fields;

	if (!org_freedesktop_Geoclue_Velocity_get_velocity (provider->proxy,
							    &fields, &ts,
							    &sp, &di, &cl,
							    error)) {
		return GEOCLUE_VELOCITY_FIELDS_NONE;
	}

	if (timestamp != NULL) {
		*timestamp = ts;
	}

	if (speed != NULL && (fields & GEOCLUE_VELOCITY_FIELDS_SPEED)) {
		*speed = sp;
	}

	if (direction != NULL && (fields & GEOCLUE_VELOCITY_FIELDS_DIRECTION)) {
		*direction = di;
	}

	if (climb != NULL && (fields & GEOCLUE_VELOCITY_FIELDS_CLIMB)) {
		*climb = cl;
	}

	return fields;
}
