/*
 * Geoclue
 * gc-provider.c - A provider object that handles the basic D-Bus required for
 *                 a provider.
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <config.h>

#include <glib-object.h>

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>

#include <geoclue/geoclue-error.h>
#include <geoclue/gc-provider.h>

static void gc_provider_geoclue_init (GcIfaceGeoclueClass *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GcProvider, gc_provider, G_TYPE_OBJECT,
				  G_IMPLEMENT_INTERFACE(GC_TYPE_IFACE_GEOCLUE,
							gc_provider_geoclue_init))

static void
finalize (GObject *object)
{
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
get_version (GcIfaceGeoclue *geoclue,
	     int            *major,
	     int            *minor,
	     int            *micro,
	     GError        **error)
{
	GcProviderClass *klass;

	klass = GC_PROVIDER_GET_CLASS (geoclue);
	if (klass->get_version) {
		return klass->get_version (geoclue, major, minor, micro, error);
	} else {
		*error = g_error_new (GEOCLUE_ERROR,
				      GEOCLUE_ERROR_NOT_IMPLEMENTED,
				      "get_version is not implemented");
		return FALSE;
	}
}

static gboolean
get_status (GcIfaceGeoclue *geoclue,
	    gboolean       *available,
	    GError        **error)
{
	GcProviderClass *klass;

	klass = GC_PROVIDER_GET_CLASS (geoclue);
	if (klass->get_status) {
		return klass->get_status (geoclue, available, error);
	} else {
		*error = g_error_new (GEOCLUE_ERROR,
				      GEOCLUE_ERROR_NOT_IMPLEMENTED,
				      "get_status is not implemented");
		return FALSE;
	}
}

static gboolean
shutdown (GcIfaceGeoclue *geoclue,
	  GError        **error)
{
	GcProviderClass *klass;

	klass = GC_PROVIDER_GET_CLASS (geoclue);
	if (klass->shutdown) {
		return klass->shutdown (geoclue, error);
	} else {
		*error = g_error_new (GEOCLUE_ERROR,
				      GEOCLUE_ERROR_NOT_IMPLEMENTED,
				      "shutdown is not implemented");
		return FALSE;
	}
}

static void
gc_provider_geoclue_init (GcIfaceGeoclueClass *iface)
{
	iface->get_version = get_version;
	iface->get_status = get_status;
	iface->shutdown = shutdown;
}

/**
 * gc_provider_set_details:
 * @provider: A #GcProvider object
 * @service: The service that the object implements
 * @path: The path that the object should be registered at
 *
 * Requests ownership of the @service name, and if that succeeds registers
 * @provider at @path.
 */
void
gc_provider_set_details (GcProvider *provider,
			 const char *service,
			 const char *path)
{
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

	dbus_g_connection_register_g_object (provider->connection, 
					     path, G_OBJECT (provider));
}
	
