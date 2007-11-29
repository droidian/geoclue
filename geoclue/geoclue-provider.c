/*
 * Geoclue
 * geoclue-provider.c - Client object for accessing Geoclue Providers
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <geoclue/geoclue-provider.h>

typedef struct _GeoclueProviderPrivate {
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

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_PROVIDER, GeoclueProviderPrivate))

G_DEFINE_ABSTRACT_TYPE (GeoclueProvider, geoclue_provider, G_TYPE_OBJECT);

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
	GeoclueProvider *provider = (GeoclueProvider *) object;

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

		return object;
	}

	provider->proxy = dbus_g_proxy_new_for_name (connection, priv->service,
						     priv->path, 
						     priv->interface);

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
}

static void
geoclue_provider_init (GeoclueProvider *provider)
{
	provider->proxy = NULL;
}
