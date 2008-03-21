/*
 * Geoclue
 * gc-provider.c - A provider object that handles the basic D-Bus required for
 *                 a provider.
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:gc-provider
 * @short_description: Abstract class to derive Geoclue providers from.
 * 
 * #GcProvider is an abstract class that all Geoclue providers should 
 * derive from. It takes care of setting up the provider D-Bus service,
 * and also implements #GcIfaceGeoclue interface (derived classes still 
 * need to implement the functionality).
 * 
 * Derived classes should define the #GcIfaceGeoclue methods in their 
 * class_init() and call gc_provider_set_details() in init()
 * 
 */
#include <config.h>

#include <string.h>
#include <glib-object.h>

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>

#include <geoclue/geoclue-error.h>
#include <geoclue/gc-provider.h>


typedef struct {
	char *name;
	char *description;
} GcProviderPrivate;

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GC_TYPE_PROVIDER, GcProviderPrivate))

static void gc_provider_geoclue_init (GcIfaceGeoclueClass *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GcProvider, gc_provider, G_TYPE_OBJECT,
				  G_IMPLEMENT_INTERFACE(GC_TYPE_IFACE_GEOCLUE,
							gc_provider_geoclue_init))

static void
finalize (GObject *object)
{
	GcProviderPrivate *priv = GET_PRIVATE (object);

	g_free (priv->name);
	g_free (priv->description);

	((GObjectClass *) gc_provider_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	((GObjectClass *) gc_provider_parent_class)->dispose (object);
}

static void
gc_provider_class_init (GcProviderClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	
	klass->shutdown = NULL;
	
	g_type_class_add_private (klass, sizeof (GcProviderPrivate));
}

static void
gc_provider_init (GcProvider *provider)
{
	GError *error = NULL;

	provider->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (provider->connection == NULL) {
		g_warning ("%s was unable to create a connection to D-Bus: %s",
			   G_OBJECT_TYPE_NAME (provider), error->message);
		g_error_free (error);
	}
}

static gboolean 
get_provider_info (GcIfaceGeoclue *geoclue,
		   gchar          **name,
		   gchar          **description,
		   GError        **error)
{
	GcProvider *provider = GC_PROVIDER (geoclue);
	GcProviderPrivate *priv = GET_PRIVATE (provider);
	
	if (name) {
		*name = g_strdup (priv->name);
	}
	if (description) {
		*description = g_strdup (priv->description);
	}
	
	return TRUE;
}

static gboolean
get_status (GcIfaceGeoclue *geoclue,
	    GeoclueStatus  *status,
	    GError        **error)
{
	GcProviderClass *klass;

	klass = GC_PROVIDER_GET_CLASS (geoclue);
	if (klass->get_status) {
		return klass->get_status (geoclue, status, error);
	} else {
		*error = g_error_new (GEOCLUE_ERROR,
				      GEOCLUE_ERROR_NOT_IMPLEMENTED,
				      "get_status is not implemented");
		return FALSE;
	}
}

static gboolean
set_options (GcIfaceGeoclue *geoclue,
             GHashTable     *options,
             GError        **error)
{
	GcProviderClass *klass;

	klass = GC_PROVIDER_GET_CLASS (geoclue);
	if (klass->set_options) {
		return klass->set_options (geoclue, options, error);
	} 

        /* It is not an error to not have a SetOptions implementation */
        return TRUE;
}

static void
gc_provider_geoclue_init (GcIfaceGeoclueClass *iface)
{
	iface->get_provider_info = get_provider_info;
	iface->get_status = get_status;
	iface->set_options = set_options;
}

static void
gc_provider_shutdown (GcProvider *provider)
{
	GC_PROVIDER_GET_CLASS (provider)->shutdown (provider);
}

static void
name_owner_changed (DBusGProxy  *proxy,
		    const char  *name,
		    const char  *prev_owner,
		    const char  *new_owner,
		    GcProvider  *provider)
{
	static int ref_count = 0;
	
	if (strcmp (new_owner, name) == 0 && strcmp (prev_owner, "") == 0) {
		ref_count++;
	} else if (strcmp (new_owner, "") == 0 && strcmp (prev_owner, name) == 0) {
		ref_count--;
		if (ref_count <= 0) {
			g_debug ("no clients, shutting down");
			gc_provider_shutdown (provider);
		}
	}
}

/**
 * gc_provider_set_details:
 * @provider: A #GcProvider object
 * @service: The service name to be requested
 * @path: The path the object should be registered at
 * @name: The provider name
 * @description: The description of the provider
 *
 * Requests ownership of the @service name, and if that succeeds registers
 * @provider at @path. @name should be the name of the provider (e.g. 
 * "Hostip"), @description should be a short description of the provider 
 * (e.g. "Web service based Position & Address provider (http://hostip.info)").
 */
void
gc_provider_set_details (GcProvider *provider,
			 const char *service,
			 const char *path,
			 const char *name,
			 const char *description)
{
	GcProviderPrivate *priv = GET_PRIVATE (provider);
	GError *error = NULL;
	DBusGProxy *driver;
	guint request_ret;

	g_return_if_fail (GC_IS_PROVIDER (provider));
	g_return_if_fail (provider->connection != NULL);
	g_return_if_fail (service != NULL);
	g_return_if_fail (path != NULL);

	driver = dbus_g_proxy_new_for_name (provider->connection,
					    DBUS_SERVICE_DBUS,
					    DBUS_PATH_DBUS,
					    DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name (driver, service, 0,
						&request_ret, &error)) {
		g_warning ("%s was unable to register service %s: %s", 
			   G_OBJECT_TYPE_NAME (provider), service, 
			   error->message);
		g_error_free (error);
		return;
	}
	
	dbus_g_proxy_add_signal (driver, "NameOwnerChanged",
				 G_TYPE_STRING, G_TYPE_STRING, 
				 G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (driver, "NameOwnerChanged",
				     G_CALLBACK (name_owner_changed),
				     provider, NULL);
	
	dbus_g_connection_register_g_object (provider->connection, 
					     path, G_OBJECT (provider));
	
	priv->name = g_strdup (name);
	priv->description = g_strdup (description);
}
