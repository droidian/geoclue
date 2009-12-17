/*
 * Geoclue
 * geoclue-master.c - Client API for accessing the Geoclue Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * SECTION:geoclue-master
 * @short_description: Geoclue Master API
 * @see_also: #GeoclueMasterClient
 * 
 * #GeoclueMaster is part of the Geoclue public C client API. It uses  
 * D-Bus to communicate with the actual Master service.
 * 
 * #GeoclueMaster is a singleton service, so it should not be created
 * explicitly: instead one should use geoclue_master_get_default() to
 * get a reference to it. It can be used to 
 * create a #GeoclueMasterClient object.
 *
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


typedef struct _GeoclueMasterAsyncData {
	GeoclueMaster *master;
	GCallback callback;
	gpointer userdata;
} GeoclueMasterAsyncData;


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

/**
 * geoclue_master_get_default:
 *
 * Returns the default #GeoclueMaster object. Should be unreferenced once
 * the client is finished with it.
 *
 * Return value: A reference to the default #GeoclueMaster object
 */
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

/**
 * geoclue_master_create_client:
 * @master: A #GeoclueMaster object
 * @object_path: Pointer to returned #GeoclueMasterClient D-Bus object path or %NULL
 * @error: Pointer to returned #GError or %NULL
 *
 * Creates a #GeoclueMasterClient and puts the D-Bus object path in
 * @object_path.
 *
 * Return Value: A new #GeoclueMasterClient or %NULL on error.
 */
 
GeoclueMasterClient *
geoclue_master_create_client (GeoclueMaster *master,
			      char         **object_path,
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
	
	if (object_path) {
		*object_path = path;
	} else {
		g_free (path);
	}
	
	return client;
}

static void
create_client_callback (DBusGProxy             *proxy, 
			char                   *path, 
			GError                 *error, 
			GeoclueMasterAsyncData *data)
{
	GeoclueMasterClient *client;
	
	client = NULL;
	
	if (!error) {
		client = g_object_new (GEOCLUE_TYPE_MASTER_CLIENT,
		                       "object-path", path,
		                       NULL);
	}
	
	(*(GeoclueCreateClientCallback)data->callback) (data->master,
	                                                client,
	                                                path,
	                                                error,
	                                                data->userdata);
	
	g_free (data);
}

void 
geoclue_master_create_client_async (GeoclueMaster              *master,
				    GeoclueCreateClientCallback callback,
				    gpointer                    userdata)
{
	GeoclueMasterPrivate *priv;
	GeoclueMasterAsyncData *data;
	
	g_return_if_fail (GEOCLUE_IS_MASTER (master));
	
	priv = GET_PRIVATE (master);
	data = g_new (GeoclueMasterAsyncData, 1);
	data->master = master;
	data->callback = G_CALLBACK (callback);
	data->userdata = userdata;
	
	org_freedesktop_Geoclue_Master_create_async
			(priv->proxy,
			 (org_freedesktop_Geoclue_Master_create_reply)create_client_callback,
			 data);
}
