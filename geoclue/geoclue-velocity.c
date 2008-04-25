/*
 * Geoclue
 * geoclue-velocity.c - Client API for accessing GcIfaceVelocity
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-velocity
 * @short_description: Geoclue velocity client API
 *
 * #GeoclueVelocity contains velocity-related methods and signals. 
 * It is part of the Geoclue public C client API which uses D-Bus 
 * to communicate with the actual provider.
 * 
 * After a #GeoclueVelocity is created with 
 * geoclue_velocity_new(), the 
 * geoclue_velocity_get_velocity() method and the VelocityChanged-signal
 * can be used to obtain the current velocity.
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
	
	/**
	 * GeoclueVelocity::velocity-changed:
	 * @velocity: the #GeoclueVelocity object emitting the signal
	 * @fields: A #GeoclueVelocityFields bitfield representing the validity of the velocity values
	 * @timestamp: Time of velocity measurement (Unix timestamp)
	 * @speed: horizontal speed
	 * @direction: horizontal direction (bearing)
	 * @climb: vertical speed
	 * 
	 * The geoclue-changed signal is emitted each time the velocity changes. 
	 * 
	 * Note that not all providers support signals.
	 */
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

/**
 * geoclue_velocity_new:
 * @service: D-Bus service name
 * @path: D-Bus path name
 *
 * Creates a #GeoclueVelocity with given D-Bus service name and path.
 * 
 * Return value: Pointer to a new #GeoclueVelocity
 */
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
 * @velocity: A #GeoclueVelocity object
 * @timestamp: Pointer to returned time of velocity measurement (unix timestamp) or %NULL
 * @speed: Pointer to returned horizontal speed or %NULL
 * @direction: Pointer to returned horizontal direction (bearing) or %NULL
 * @climb: Pointer to returned vertical speed or %NULL
 * @error: Pointer to returned #GError or %NULL
 *
 * Obtains the current velocity. @timestamp will contain the time of 
 * the actual velocity measurement.
 * 
 * If the caller is not interested in some values, the pointers can be 
 * left %NULL.
 * 
 * Return value: A #GeoclueVelocityFields bitfield representing the 
 * validity of the velocity values.
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

typedef struct _GeoclueVelocityAsyncData {
	GeoclueVelocity *velocity;
	GCallback callback;
	gpointer userdata;
} GeoclueVelocityAsyncData;

static void
get_velocity_async_callback (DBusGProxy               *proxy, 
			     GeoclueVelocityFields     fields,
			     int                       timestamp,
			     double                    speed,
			     double                    direction,
			     double                    climb,
			     GError                   *error,
			     GeoclueVelocityAsyncData *data)
{
	(*(GeoclueVelocityCallback)data->callback) (data->velocity,
	                                            fields,
	                                            timestamp,
	                                            speed,
	                                            direction,
	                                            climb,
	                                            error,
	                                            data->userdata);
	g_free (data);
}

/**
 * GeoclueVelocityCallback:
 * @velocity: A #GeoclueVelocity object
 * @fields: A #GeoclueVelocityFields bitfield representing the validity of the velocity values
 * @timestamp: Time of velocity measurement (unix timestamp)
 * @speed: Horizontal speed
 * @direction: Horizontal direction (bearing)
 * @climb: Vertical speed
 * @error: Error as #GError (may be %NULL)
 * @userdata: User data pointer set in geoclue_velocity_get_velocity_async()
 * 
 * Callback function for geoclue_velocity_get_velocity_async().
 */

/**
 * geoclue_velocity_get_velocity_async:
 * @velocity: A #GeoclueVelocity object
 * @callback: A #GeoclueVelocityCallback function that should be called when return values are available
 * @userdata: pointer for user specified data
 * 
 * Function returns (essentially) immediately and calls @callback when current velocity
 * is available or when D-Bus timeouts.
 */
void 
geoclue_velocity_get_velocity_async (GeoclueVelocity         *velocity,
				     GeoclueVelocityCallback  callback,
				     gpointer                 userdata)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (velocity);
	GeoclueVelocityAsyncData *data;
	
	data = g_new (GeoclueVelocityAsyncData, 1);
	data->velocity = velocity;
	data->callback = G_CALLBACK (callback);
	data->userdata = userdata;
	
	org_freedesktop_Geoclue_Velocity_get_velocity_async
			(provider->proxy,
			 (org_freedesktop_Geoclue_Velocity_get_velocity_reply)get_velocity_async_callback,
			 data);
}
