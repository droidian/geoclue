/*
 * Geoclue
 * gc-backend.c - A backend object that handles the basic D-Bus required for
 *                a backend.
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <config.h>

#include <glib-object.h>

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>

#include <geoclue/gc-backend.h>

G_DEFINE_ABSTRACT_TYPE (GcBackend, gc_backend, G_TYPE_OBJECT)

static void
finalize (GObject *object)
{
	((GObjectClass *) gc_backend_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	((GObjectClass *) gc_backend_parent_class)->dispose (object);
}

static void
gc_backend_class_init (GcBackendClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
}

static void
gc_backend_init (GcBackend *backend)
{
	GError *error = NULL;

	backend->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (backend->connection == NULL) {
		g_warning ("%s was unable to create a connection to D-Bus: %s",
			   G_OBJECT_TYPE_NAME (backend), error->message);
		g_error_free (error);
	}
}

/**
 * gc_backend_set_details:
 * @backend: A #GcBackend object
 * @service: The service that the object implements
 * @path: The path that the object should be registered at
 *
 * Requests ownership of the @service name, and if that succeeds registers
 * @backend at @path.
 */
void
gc_backend_set_details (GcBackend  *backend,
			const char *service,
			const char *path)
{
	GError *error = NULL;
	DBusGProxy *driver;
	guint request_ret;

	g_return_if_fail (GC_IS_BACKEND (backend));
	g_return_if_fail (backend->connection != NULL);
	g_return_if_fail (service != NULL);
	g_return_if_fail (path != NULL);

	driver = dbus_g_proxy_new_for_name (backend->connection,
					    DBUS_SERVICE_DBUS,
					    DBUS_PATH_DBUS,
					    DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name (driver, service, 0,
						&request_ret, &error)) {
		g_warning ("%s was unable to register service %s: %s", 
			   G_OBJECT_TYPE_NAME (backend), service, 
			   error->message);
		g_error_free (error);
		return;
	}

	dbus_g_connection_register_g_object (backend->connection, 
					     path, G_OBJECT (backend));
}
	
