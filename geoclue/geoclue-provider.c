/*
 * Geoclue
 * geoclue-provider.c - Client object for accessing Geoclue Providers
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *           2008 OpenedHand Ltd
 */

#include <geoclue/geoclue-provider.h>
#include "gc-iface-geoclue-bindings.h"

typedef struct _GeoclueProviderPrivate {
	DBusGProxy *geoclue_proxy;
	
	char *service;
	char *path;
	char *interface;
} GeoclueProviderPrivate;

enum {
	PROP_0,
	PROP_SERVICE,
	PROP_PATH,
	PROP_INTERFACE
};

enum {
	STATUS_CHANGED,
	LAST_SIGNAL
};
static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_PROVIDER, GeoclueProviderPrivate))
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GeoclueProvider, geoclue_provider, G_TYPE_OBJECT, geoclue_types_init (););

#define GEOCLUE_INTERFACE_NAME "org.freedesktop.Geoclue"


static void
status_changed (DBusGProxy      *proxy,
                GeoclueStatus    status,
                GeoclueProvider *provider)
{
	g_signal_emit (provider, signals[STATUS_CHANGED], 0, status);
}

static void
finalize (GObject *object)
{
	GeoclueProviderPrivate *priv = GET_PRIVATE (object);
	
	g_free (priv->service);
	g_free (priv->path);
	g_free (priv->interface);
	
	G_OBJECT_CLASS (geoclue_provider_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (object);
	GeoclueProviderPrivate *priv = GET_PRIVATE (object);
	GError *error = NULL;
	
	if (!org_freedesktop_Geoclue_unref (priv->geoclue_proxy, &error)){
		g_printerr ("Could not unreference provider: %s", error->message);
		g_error_free (error);
	}
	if (priv->geoclue_proxy) {
		g_object_unref (priv->geoclue_proxy);
		priv->geoclue_proxy = NULL;
	}
	
	if (provider->proxy) {
		g_object_unref (provider->proxy);
		provider->proxy = NULL;
	}
	
	G_OBJECT_CLASS (geoclue_provider_parent_class)->dispose (object);
}

static GObject *
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
	GObject *object;
	GeoclueProvider *provider;
	GeoclueProviderPrivate *priv;
	DBusGConnection *connection;
	GError *error = NULL;
	
	object = G_OBJECT_CLASS (geoclue_provider_parent_class)->constructor
		(type, n_props, props);
	provider = GEOCLUE_PROVIDER (object);
	priv = GET_PRIVATE (provider);
	
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
		            error->message);
		g_error_free (error);
		provider->proxy = NULL;
		priv->geoclue_proxy = NULL;
		
		return object;
	}
	
	/* proxy for the requested interface */
	provider->proxy = dbus_g_proxy_new_for_name (connection, 
	                                           priv->service, priv->path, 
	                                           priv->interface);
	
	/* proxy for org.freedesktop.Geoclue */
	priv->geoclue_proxy = dbus_g_proxy_new_for_name (connection, 
	                                                 priv->service, priv->path, 
	                                                 GEOCLUE_INTERFACE_NAME);
	if (!org_freedesktop_Geoclue_ref (priv->geoclue_proxy, &error)){
		g_printerr ("Could not reference provider: %s", error->message);
		g_error_free (error);
	}
	dbus_g_proxy_add_signal (priv->geoclue_proxy, "StatusChanged",
	                         G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->geoclue_proxy, "StatusChanged",
	                             G_CALLBACK (status_changed),
	                             object, NULL);
	
	return object;
}
	
static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
	GeoclueProviderPrivate *priv = GET_PRIVATE (object);
	
	switch (prop_id) {
	case PROP_SERVICE:
		priv->service = g_value_dup_string (value);
		break;
	
	case PROP_PATH:
		priv->path = g_value_dup_string (value);
		break;
	
	case PROP_INTERFACE:
		priv->interface = g_value_dup_string (value);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		break;
	}
}

static void
geoclue_provider_class_init (GeoclueProviderClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;
	
	g_type_class_add_private (klass, sizeof (GeoclueProviderPrivate));
	
	g_object_class_install_property
		(o_class, PROP_SERVICE,
		 g_param_spec_string ("service", "Service",
				      "The D-Bus service this object represents",
				      "", G_PARAM_WRITABLE |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_NICK |
				      G_PARAM_STATIC_BLURB |
				      G_PARAM_STATIC_NAME));
	g_object_class_install_property
		(o_class, PROP_PATH,
		 g_param_spec_string ("path", "Path",
				      "The D-Bus path to this provider",
				      "", G_PARAM_WRITABLE |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_NICK |
				      G_PARAM_STATIC_BLURB |
				      G_PARAM_STATIC_NAME));
	g_object_class_install_property
		(o_class, PROP_INTERFACE,
		 g_param_spec_string ("interface", "Interface",
				      "The interface this object uses",
				      "", G_PARAM_WRITABLE |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_NICK |
				      G_PARAM_STATIC_BLURB |
				      G_PARAM_STATIC_NAME));
	
	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						G_STRUCT_OFFSET (GeoclueProviderClass, status_changed), 
						NULL, NULL,
						g_cclosure_marshal_VOID__INT,
						G_TYPE_NONE, 1, G_TYPE_INT);

}

static void
geoclue_provider_init (GeoclueProvider *provider)
{
	GeoclueProviderPrivate *priv = GET_PRIVATE (provider);
	
	provider->proxy = NULL;
	priv->geoclue_proxy = NULL;
}

/**
 * geoclue_provider_get_status:
 * @provider: A #GeoclueProvider object
 * @status: Pointer for returned status as #GeoclueStatus
 * @error:  Pointer for returned #GError or %NULL
 * 
 * Obtains the current status of the provider.
 * 
 * Return value: %TRUE on success
 */
gboolean
geoclue_provider_get_status (GeoclueProvider  *provider,
                             GeoclueStatus    *status,
                             GError          **error)
{
	GeoclueProviderPrivate *priv = GET_PRIVATE (provider);
	int i;
	
	if (status == NULL) {
		return TRUE;
	}
	
	if (!org_freedesktop_Geoclue_get_status (priv->geoclue_proxy, 
	                                         &i, error)) {
		return FALSE;
	}
	*status = i;
	return TRUE;
}

/**
 * geoclue_provider_set_options:
 * @provider: A #GeoclueProvider object
 * @options: A #GHashTable containing the options
 * @error: Pointer for returned #GError or %NULL
 *
 * Sets the options on the provider.
 *
 * Return value: %TRUE if setting options succeeded
 */
gboolean
geoclue_provider_set_options (GeoclueProvider  *provider,
                              GHashTable       *options,
                              GError          **error)
{
	GeoclueProviderPrivate *priv = GET_PRIVATE (provider);
	
	if (options == NULL) {
		return TRUE;
	}
	
	return org_freedesktop_Geoclue_set_options (priv->geoclue_proxy, 
	                                            options, error);
}

/**
 * geoclue_provider_get_provider_info:
 * @provider: A #GeoclueProvider object
 * @name: Pointer for returned provider name or %NULL
 * @description: Pointer for returned provider description or %NULL
 * @error:  Pointer for returned #GError or %NULL
 * 
 * Obtains name and a short description of the provider.
 * 
 * Return value: %TRUE on success
 */
gboolean
geoclue_provider_get_provider_info (GeoclueProvider  *provider,
                                    char            **name,
                                    char            **description,
                                    GError          **error)
{
	GeoclueProviderPrivate *priv = GET_PRIVATE (provider);
	
	return org_freedesktop_Geoclue_get_provider_info (priv->geoclue_proxy,
	                                                  name, description,
	                                                  error);
}
