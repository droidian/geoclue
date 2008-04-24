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
 * #GeoclueMasterClient is used to control the client specific behaviour 
 * of Geoclue Master. Chapter "Master provider: simple example in C" contains a
 * more complete example, but here are the main parts:
 * 
 * <informalexample>
 * <programlisting>
 * GeoclueMaster *master;
 * GeoclueMasterClient *client;
 * GeoclueAddress *address;
 *
 * ...
 * 
 * master = geoclue_master_get_default ();
 * 
 * client = geoclue_master_create_client (master, NULL, NULL);
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
 * address = geoclue_master_client_create_address (client, error);
 * if (!address) {
 * 	/ * handle error * /
 * }
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
	ADDRESS_PROVIDER_CHANGED,
	POSITION_PROVIDER_CHANGED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_MASTER_CLIENT, GeoclueMasterClientPrivate))

G_DEFINE_TYPE_WITH_CODE (GeoclueMasterClient, geoclue_master_client, G_TYPE_OBJECT, geoclue_types_init (););

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
address_provider_changed (DBusGProxy          *proxy,
                          char                *name,
                          char                *description, 
                          char                *service, 
                          char                *path, 
                          GeoclueMasterClient *client)
{
	g_signal_emit (client, signals[ADDRESS_PROVIDER_CHANGED], 0,
	               name, description, service, path);
}

static void
position_provider_changed (DBusGProxy          *proxy,
                           char                *name,
                           char                *description, 
                           char                *service, 
                           char                *path, 
                           GeoclueMasterClient *client)
{
	g_signal_emit (client, signals[POSITION_PROVIDER_CHANGED], 0, 
	               name, description, service, path);
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
	
	dbus_g_proxy_add_signal (priv->proxy, "AddressProviderChanged",
	                         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                         G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "AddressProviderChanged",
	                             G_CALLBACK (address_provider_changed),
	                             object, NULL);
	
	dbus_g_proxy_add_signal (priv->proxy, "PositionProviderChanged",
	                         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                         G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "PositionProviderChanged",
	                             G_CALLBACK (position_provider_changed),
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
	
	/**
	* GeoclueMasterClient::address-provider-changed:
	* @client: the #GeoclueMasterClient object emitting the signal
	* @name: name of the new provider (e.g. "Hostip") or %NULL if there is no provider
	* @description: a short description of the new provider or %NULL if there is no provider
	* @service: D-Bus service name of the new provider or %NULL if there is no provider
	* @path: D-Bus object path name of the new provider or %NULL if there is no provider
	* 
	* The address-provider-changed signal is emitted each time the used address provider
	* changes.
	**/
	signals[ADDRESS_PROVIDER_CHANGED] = 
		g_signal_new ("address-provider-changed",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (GeoclueMasterClientClass, address_provider_changed), 
		              NULL, NULL,
		              geoclue_marshal_VOID__STRING_STRING_STRING_STRING,
		              G_TYPE_NONE, 4,
		              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	
	/**
	* GeoclueMasterClient::position-provider-changed:
	* @client: the #GeoclueMasterClient object emitting the signal
	* @name: name of the new provider (e.g. "Hostip") or %NULL if there is no provider
	* @description: a short description of the new provider or %NULL if there is no provider
	* @service: D-Bus service name of the new provider or %NULL if there is no provider
	* @path: D-Bus object path name of the new provider or %NULL if there is no provider
	* 
	* The position-provider-changed signal is emitted each time the used position provider
	* changes.
	**/
	signals[POSITION_PROVIDER_CHANGED] = 
		g_signal_new ("position-provider-changed",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (GeoclueMasterClientClass, position_provider_changed), 
		              NULL, NULL,
		              geoclue_marshal_VOID__STRING_STRING_STRING_STRING,
		              G_TYPE_NONE, 4,
		              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}

static void
geoclue_master_client_init (GeoclueMasterClient *client)
{
}

/**
 * geoclue_master_client_set_requirements:
 * @client: A #GeoclueMasterClient
 * @min_accuracy: The required minimum accuracy as a #GeoclueAccuracyLevel.
 * @min_time: The minimum time between update signals (currently not implemented)
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
 * geoclue_master_client_create_address:
 * @client: A #GeoclueMasterClient
 * @error: A pointer to returned #GError or %NULL.
 *
 * Starts the GeoclueMasterClient address provider and returns 
 * a #GeoclueAddress that uses the same D-Bus object as the #GeoclueMasterClient.
 *
 * Return value: New #GeoclueAddress or %NULL on error
 */
GeoclueAddress *
geoclue_master_client_create_address (GeoclueMasterClient *client, 
                                      GError **error)
{
	GeoclueMasterClientPrivate *priv;
	
	priv = GET_PRIVATE (client);
	
	if (!org_freedesktop_Geoclue_MasterClient_address_start (priv->proxy, error)) {
		return NULL;
	}
	
	return geoclue_address_new (GEOCLUE_MASTER_DBUS_SERVICE, priv->object_path);
}

typedef struct _GeoclueMasterClientAsyncData {
	GeoclueMasterClient *client;
	GCallback callback;
	gpointer userdata;
} GeoclueMasterClientAsyncData;

static void
address_start_async_callback (DBusGProxy                   *proxy, 
			      GError                       *error,
			      GeoclueMasterClientAsyncData *data)
{
	GeoclueMasterClientPrivate *priv = GET_PRIVATE (data->client);
	GeoclueAddress *address = NULL;
	
	if (!error) {
		address = geoclue_address_new (GEOCLUE_MASTER_DBUS_SERVICE, priv->object_path);
	}
	
	(*(CreateAddressCallback)data->callback) (data->client,
	                                          address,
	                                          error,
	                                          data->userdata);
	g_free (data);
}

/**
 * GreateAddressCallback:
 * @client: A #GeoclueMasterClient object
 * @address: returned #GeoclueAddress
 * @error: Error as #Gerror (may be %NULL)
 * @userdata: User data pointer set in geoclue_master_client_create_address_async()
 * 
 * Callback function for geoclue_master_client_create_address_async().
 */

/**
 * geoclue_master_client_create_address_async:
 * @client: A #GeoclueMasterClient object
 * @callback: A #CreateAddressCallback function that should be called when return values are available
 * @userdata: pointer for user specified data
 * 
 * Function returns (essentially) immediately and calls @callback when an address provider is available.
 */
void 
geoclue_master_client_create_address_async (GeoclueMasterClient  *client,
					    CreateAddressCallback callback,
					    gpointer              userdata)
{
	GeoclueMasterClientPrivate *priv = GET_PRIVATE (client);
	GeoclueMasterClientAsyncData *data;
	
	data = g_new (GeoclueMasterClientAsyncData, 1);
	data->client = client;
	data->callback = G_CALLBACK (callback);
	data->userdata = userdata;
	
	org_freedesktop_Geoclue_MasterClient_address_start_async
			(priv->proxy,
			 (org_freedesktop_Geoclue_MasterClient_address_start_reply)address_start_async_callback,
			 data);
}


/**
 * geoclue_master_client_create_position:
 * @client: A #GeoclueMasterClient
 * @error: A pointer to returned #GError or %NULL.
 *
 * Starts the GeoclueMasterClient position provider and returns 
 * a #GeocluePosition that uses the same D-Bus object as the #GeoclueMasterClient.
 *
 * Return value: New #GeocluePosition or %NULL on error
 */
GeocluePosition *
geoclue_master_client_create_position (GeoclueMasterClient *client,
                                       GError **error)
{
	GeoclueMasterClientPrivate *priv;
	
	priv = GET_PRIVATE (client);
	
	if (!org_freedesktop_Geoclue_MasterClient_position_start (priv->proxy, error)) {
		return NULL;
	}
	return geoclue_position_new (GEOCLUE_MASTER_DBUS_SERVICE, priv->object_path);
}


static void
position_start_async_callback (DBusGProxy                   *proxy, 
			       GError                       *error,
			       GeoclueMasterClientAsyncData *data)
{
	GeoclueMasterClientPrivate *priv = GET_PRIVATE (data->client);
	GeocluePosition *position = NULL;
	
	if (!error) {
		position = geoclue_position_new (GEOCLUE_MASTER_DBUS_SERVICE, priv->object_path);
	}
	
	(*(CreatePositionCallback)data->callback) (data->client,
	                                          position,
	                                          error,
	                                          data->userdata);
	g_free (data);
}

/**
 * GreatePositionCallback:
 * @client: A #GeoclueMasterClient object
 * @position: returned #GeocluePosition
 * @error: Error as #Gerror (may be %NULL)
 * @userdata: User data pointer set in geoclue_master_client_create_position_async()
 * 
 * Callback function for geoclue_master_client_create_position_async().
 */

/**
 * geoclue_master_client_create_position_async:
 * @client: A #GeoclueMasterClient object
 * @callback: A #CreatePositionCallback function that should be called when return values are available
 * @userdata: pointer for user specified data
 * 
 * Function returns (essentially) immediately and calls @callback when a position provider is available.
 */
void 
geoclue_master_client_create_position_async (GeoclueMasterClient    *client,
					     CreatePositionCallback  callback,
					     gpointer                userdata)
{
	GeoclueMasterClientPrivate *priv = GET_PRIVATE (client);
	GeoclueMasterClientAsyncData *data;
	
	data = g_new (GeoclueMasterClientAsyncData, 1);
	data->client = client;
	data->callback = G_CALLBACK (callback);
	data->userdata = userdata;
	
	org_freedesktop_Geoclue_MasterClient_position_start_async
			(priv->proxy,
			 (org_freedesktop_Geoclue_MasterClient_position_start_reply)position_start_async_callback,
			 data);
}


/**
 * geoclue_master_client_get_address_provider:
 * @client: A #GeoclueMasterClient
 * @name: Pointer to returned provider name or %NULL
 * @description: Pointer to returned provider description or %NULL
 * @service: Pointer to returned D-Bus service name or %NULL
 * @path: Pointer to returned D-Bus object path or %NULL
 * @error: Pointer to returned #GError or %NULL
 * 
 * Gets name and other information for the currently used address provider.
 * 
 * Return value: %TRUE on success
 */
gboolean geoclue_master_client_get_address_provider (GeoclueMasterClient  *client,
                                                     char                **name,
                                                     char                **description,
                                                     char                **service,
                                                     char                **path,
                                                     GError              **error)
{
	GeoclueMasterClientPrivate *priv;
	
	priv = GET_PRIVATE (client);
	if (!org_freedesktop_Geoclue_MasterClient_get_address_provider 
	    (priv->proxy, name, description, service, path, error)) {
		return FALSE;
	}
	
	return TRUE;
}

/**
 * geoclue_master_client_get_position_provider:
 * @client: A #GeoclueMasterClient
 * @name: Pointer to returned provider name or %NULL
 * @description: Pointer to returned provider description or %NULL
 * @service: Pointer to returned D-Bus service name or %NULL
 * @path: Pointer to returned D-Bus object path or %NULL
 * @error: Pointer to returned #GError or %NULL
 * 
 * Gets name and other information for the currently used position provider.
 * 
 * Return value: %TRUE on success
 */
gboolean geoclue_master_client_get_position_provider (GeoclueMasterClient  *client,
                                                      char                **name,
                                                      char                **description,
                                                      char                **service,
                                                      char                **path,
                                                      GError              **error)
{
	GeoclueMasterClientPrivate *priv;
	
	priv = GET_PRIVATE (client);
	if (!org_freedesktop_Geoclue_MasterClient_get_position_provider 
	    (priv->proxy, name, description, service, path, error)) {
		return FALSE;
	}
	
	return TRUE;
}

