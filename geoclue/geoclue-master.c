/*
 * Geoclue
 * geoclue-master.c - Client API for accessing the Geoclue Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-accuracy.h>

#include "gc-iface-master-bindings.h"

typedef struct _GeoclueMasterPrivate {
	DBusGProxy *proxy;
} GeoclueMasterPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_MASTER, GeoclueMasterPrivate))

G_DEFINE_TYPE (GeoclueMaster, geoclue_master, G_TYPE_OBJECT);

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (geoclue_master_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	G_OBJECT_CLASS (geoclue_master_parent_class)->dispose (object);
}

static void
geoclue_master_class_init (GeoclueMasterClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;

	g_type_class_add_private (klass, sizeof (GeoclueMasterPrivate));
}

static void
geoclue_master_init (GeoclueMaster *master)
{
	GeoclueMasterPrivate *priv;
	DBusGConnection *connection;
	GError *error = NULL;

	priv = GET_PRIVATE (master);

	connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (!connection) {
		g_warning ("Unable to get connection to Geoclue bus.\n%s",
			   error->message);
		g_error_free (error);
		return;
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection,
						 GEOCLUE_MASTER_DBUS_SERVICE,
						 GEOCLUE_MASTER_DBUS_PATH,
						 GEOCLUE_MASTER_DBUS_INTERFACE);
}

GeoclueMaster *
geoclue_master_get_default (void)
{
	static GeoclueMaster *master = NULL;

	if (G_UNLIKELY (master == NULL)) {
		master = g_object_new (GEOCLUE_TYPE_MASTER, NULL);
		g_object_add_weak_pointer (G_OBJECT (master), 
					   (gpointer) &master);
		return master;
	}

	return g_object_ref (master);
}

GeoclueMasterClient *
geoclue_master_create_client (GeoclueMaster *master,
			      GError       **error)
{
	GeoclueMasterPrivate *priv;
	GeoclueMasterClient *client;
	char *path;

	g_return_val_if_fail (GEOCLUE_IS_MASTER (master), NULL);

	priv = GET_PRIVATE (master);

	if (!org_freedesktop_Geoclue_Master_create (priv->proxy, 
						    &path, error)){
		return NULL;
	}
	
	client = g_object_new (GEOCLUE_TYPE_MASTER_CLIENT,
			       "object-path", path,
			       NULL);
	g_free (path);
	
	return client;
}
	
	
