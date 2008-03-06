/*
 * Geoclue
 * geoclue-master-client.c - Client API for accessing the Geoclue Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-master-client
 * @short_description: Geoclue MasterClient API
 *
 * #GeoclueMasterClient is part of the Geoclue public C client API. It uses  
 * D-Bus to communicate with the actual Master service.
 * 
 * #GeoclueMasterClient is the easy way to use Geoclue: There's 
 * no need to choose a specific provider and hope it can provide whatever
 * is needed, just set requirements with 
 * geoclue_master_client_set_requirements() and use the wanted Geoclue 
 * interface. #GeoclueMasterClient will make sure the best possible 
 * provider is used at all times, even when provider availabilities and 
 * accuracies change. In other words, the data may actually come from 
 * different providers in different situations.
 * 
 * <informalexample>
 * <programlisting>
 * GeoclueMaster *master;
 * GeoclueMasterClient *client;
 * GeoclueAddress *address;
 * char* path;
 * GError *error;
 * 
 * ...
 * 
 * master = geoclue_master_get_default ();
 * 
 * client = geoclue_master_create_client (master, &path, &error);
 * if (!client) {
 * 	/ * handle error * /
 * }
 * 
 * if (!geoclue_master_client_set_requirements (client,
 *                                              GEOCLUE_ACCURACY_LEVEL_NONE,
 *                                              0, FALSE,
 *                                              GEOCLUE_RESOURCE_NETWORK,
 *                                              &error)) {
 * 	/ * handle error * /
 * }
 * 
 * address = geoclue_address_new (GEOCLUE_MASTER_DBUS_SERVICE, path);
 * 
 * / * Now we can use address just like we'd use a normal address provider, 
 *     but GeoclueMasterClient makes sure that underneath the provider
 *     that best matches our requirements is used * /
 * </programlisting>
 * </informalexample>
 */

#include <config.h>

#include <glib-object.h>

#include <geoclue/geoclue-marshal.h>
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

enum {
	PROVIDER_CHANGED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL] = {0, };

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

static void
provider_changed (DBusGProxy          *proxy,
                  char                *iface,
                  char                *name,
                  char                *description, 
                  GeoclueMasterClient *client)
{
	g_signal_emit (client, signals[PROVIDER_CHANGED], 0, 
		       iface, name, description);
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
	
	dbus_g_proxy_add_signal (priv->proxy, "ProviderChanged",
	                         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                         G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "ProviderChanged",
	                             G_CALLBACK (provider_changed),
	                             object, NULL);
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
	
	signals[PROVIDER_CHANGED] = g_signal_new ("provider-changed",
	                            G_TYPE_FROM_CLASS (klass),
	                            G_SIGNAL_RUN_FIRST |
	                            G_SIGNAL_NO_RECURSE,
	                            G_STRUCT_OFFSET (GeoclueMasterClientClass, provider_changed), 
	                            NULL, NULL,
	                            geoclue_marshal_VOID__STRING_STRING_STRING,
	                            G_TYPE_NONE, 3,
	                            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}

static void
geoclue_master_client_init (GeoclueMasterClient *client)
{
}

/**
 * geoclue_master_client_set_requirements:
 * @client: A #GeoclueMasterClient
 * @min_accuracy: The required minimum accuracy as a #GeoclueAccuracyLevel.
 * @min_time: The minimum time between update signals
 * @require_updates: Whether the updates (signals) are required. Only applies to interfaces with signals
 * @allowed_resources: The resources that are allowed to be used as a #GeoclueResourceFlags
 * @error: A pointer to returned #GError or %NULL.
 *
 * Sets the criteria that should be used when selecting the used provider
 *
 * Return value: %TRUE on success
 */
gboolean
geoclue_master_client_set_requirements (GeoclueMasterClient   *client,
					GeoclueAccuracyLevel   min_accuracy,
					int                    min_time,
					gboolean               require_updates,
					GeoclueResourceFlags   allowed_resources,
					GError               **error)
{
	GeoclueMasterClientPrivate *priv;

	priv = GET_PRIVATE (client);
	if (!org_freedesktop_Geoclue_MasterClient_set_requirements 
	    (priv->proxy, min_accuracy, min_time, require_updates, allowed_resources, error)) {
		return FALSE;
	}

	return TRUE;
}

/**
 * geoclue_master_client_get_provider:
 * @client: A #GeoclueMasterClient
 * @interface: The D-Bus interface name (e.g. "org.freedesktop.Geoclue.Position")
 * @name: Pointer to the returned provider name or %NULL
 * @description: Pointer to the returned provider description or %NULL
 * @error: Pointer to the returned #GError or %NULL
 * 
 * Gets the name and description of the currently used provider for 
 * given interface.
 * 
 * Return value: %TRUE on success
 */
gboolean geoclue_master_client_get_provider (GeoclueMasterClient  *client,
                                             char                 *interface,
                                             char                **name,
                                             char                **description,
                                             GError              **error)
{
	GeoclueMasterClientPrivate *priv;
	
	priv = GET_PRIVATE (client);
	if (!org_freedesktop_Geoclue_MasterClient_get_provider 
	    (priv->proxy, interface, name, description,error)) {
		return FALSE;
	}
	
	return TRUE;
}

