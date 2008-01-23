/*
 * Geoclue
 * geoclue-master-client.c - Client API for accessing the Geoclue Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <glib-object.h>

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-master-client.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

#include "gc-iface-master-client-bindings.h"

typedef struct _GeoclueMasterClientPrivate {
	DBusGProxy *proxy;
	char *object_path;
} GeoclueMasterClientPrivate;

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_MASTER_CLIENT, GeoclueMasterClientPrivate))

G_DEFINE_TYPE (GeoclueMasterClient, geoclue_master_client, G_TYPE_OBJECT);

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (geoclue_master_client_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	G_OBJECT_CLASS (geoclue_master_client_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GeoclueMasterClientPrivate *priv;

	priv = GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_PATH:
		priv->object_path = g_value_dup_string (value);
		break;

	default:
		break;
	}
}

static void
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
}

static GObject *
constructor (GType                  type,
	     guint                  n_props,
	     GObjectConstructParam *props)
{
	GeoclueMasterClient *client;
	GeoclueMasterClientPrivate *priv;
	DBusGConnection *connection;
	GObject *object;
	GError *error = NULL;

	object = G_OBJECT_CLASS (geoclue_master_client_parent_class)->constructor (type, n_props, props);
	client = GEOCLUE_MASTER_CLIENT (object);
	priv = GET_PRIVATE (client);

	connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (!connection) {
		g_warning ("Failed to open connection to bus: %s",
			   error->message);
		g_error_free (error);

		priv->proxy = NULL;
		return object;
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection,
						 GEOCLUE_MASTER_DBUS_SERVICE,
						 priv->object_path,
						 GEOCLUE_MASTER_CLIENT_DBUS_INTERFACE);

	return object;
}

static void
geoclue_master_client_class_init (GeoclueMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GeoclueMasterClientPrivate));

	g_object_class_install_property 
		(o_class, PROP_PATH,
		 g_param_spec_string ("object-path",
				      "Object path",
				      "The DBus path to the object",
				      "",
				      G_PARAM_WRITABLE |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_NICK |
				      G_PARAM_STATIC_BLURB |
				      G_PARAM_STATIC_NAME));
}

static void
geoclue_master_client_init (GeoclueMasterClient *client)
{
}

gboolean
geoclue_master_client_set_requirements (GeoclueMasterClient *client,
					GeoclueAccuracy     *accuracy,
					int                  min_time,
					gboolean             require_updates,
					GError             **error)
{
	GeoclueMasterClientPrivate *priv;

	priv = GET_PRIVATE (client);
	if (!org_freedesktop_Geoclue_MasterClient_set_requirements 
	    (priv->proxy, accuracy, min_time, require_updates, error)) {
		return FALSE;
	}

	return TRUE;
}
