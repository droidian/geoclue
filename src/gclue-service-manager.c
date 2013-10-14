/* vim: set et ts=8 sw=8: */
/* gclue-service-manager.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <glib/gi18n.h>

#include "gclue-service-manager.h"
#include "gclue-service-client.h"

static void
gclue_service_manager_manager_iface_init (GClueManagerIface *iface);
static void
gclue_service_manager_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceManager,
                         gclue_service_manager,
                         GCLUE_TYPE_MANAGER_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_MANAGER,
                                                gclue_service_manager_manager_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                gclue_service_manager_initable_iface_init))

struct _GClueServiceManagerPrivate
{
        GDBusConnection *connection;
        GHashTable *clients;

        guint num_clients;
};

enum
{
        PROP_0,
        PROP_CONNECTION,
        PROP_CONNECTED_CLIENTS,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
on_peer_vanished (GClueServiceClient *client,
                  gpointer            user_data)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (user_data);

        g_hash_table_remove (manager->priv->clients,
                             gclue_service_client_get_peer (client));
        g_object_notify (G_OBJECT (manager), "connected-clients");
}

static gboolean
gclue_service_manager_handle_get_client (GClueManager          *manager,
                                         GDBusMethodInvocation *invocation)
{
        GClueServiceManager *self = GCLUE_SERVICE_MANAGER (manager);
        GClueServiceManagerPrivate *priv = self->priv;
        GClueServiceClient *client;
        const char *peer;
        char *path;
        GError *error = NULL;

        peer = g_dbus_method_invocation_get_sender (invocation);
        client = g_hash_table_lookup (priv->clients, peer);
        if (client != NULL) {
                const gchar *existing_path;

                existing_path = gclue_service_client_get_path (client);
                gclue_manager_complete_get_client (manager,
                                                   invocation,
                                                   existing_path);
                return TRUE;
        }

        path = g_strdup_printf ("/org/freedesktop/GeoClue2/Client/%u",
                                ++priv->num_clients);

        client = gclue_service_client_new (peer, path, priv->connection, &error);
        if (client == NULL) {
                g_dbus_method_invocation_return_error (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_FAILED,
                                                       "%s", error->message);
                return TRUE;
        }

        g_hash_table_insert (priv->clients, g_strdup (peer), client);
        g_object_notify (G_OBJECT (manager), "connected-clients");

        g_signal_connect (client,
                          "peer-vanished",
                          G_CALLBACK (on_peer_vanished),
                          manager);

        gclue_manager_complete_get_client (manager, invocation, path);

        return TRUE;
}

static void
gclue_service_manager_finalize (GObject *object)
{
        GClueServiceManagerPrivate *priv = GCLUE_SERVICE_MANAGER (object)->priv;

        g_clear_object (&priv->connection);
        g_clear_pointer (&priv->clients, g_hash_table_unref);

        /* Chain up to the parent class */
        G_OBJECT_CLASS (gclue_service_manager_parent_class)->finalize (object);
}

static void
gclue_service_manager_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                g_value_set_object (value, manager->priv->connection);
                break;

        case PROP_CONNECTED_CLIENTS:
        {
                guint num;

                num = gclue_service_manager_get_connected_clients (manager);
                g_value_set_uint (value, num);
                break;
        }

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_manager_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                manager->priv->connection = g_value_dup_object (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}


static void
gclue_service_manager_class_init (GClueServiceManagerClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_manager_finalize;
        object_class->get_property = gclue_service_manager_get_property;
        object_class->set_property = gclue_service_manager_set_property;

        g_type_class_add_private (object_class, sizeof (GClueServiceManagerPrivate));

        gParamSpecs[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                            "Connection",
                                                            "DBus Connection",
                                                            G_TYPE_DBUS_CONNECTION,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_CONNECTION,
                                         gParamSpecs[PROP_CONNECTION]);

        gParamSpecs[PROP_CONNECTED_CLIENTS] =
                g_param_spec_uint ("connected-clients",
                                   "ConnectedClients",
                                   "Number of connected clients",
                                   0,
                                   G_MAXUINT,
                                   0,
                                   G_PARAM_READABLE);
        g_object_class_install_property (object_class,
                                         PROP_CONNECTED_CLIENTS,
                                         gParamSpecs[PROP_CONNECTED_CLIENTS]);
}

static void
gclue_service_manager_init (GClueServiceManager *manager)
{
        manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager,
                                                     GCLUE_TYPE_SERVICE_MANAGER,
                                                     GClueServiceManagerPrivate);

        manager->priv->clients = g_hash_table_new_full (g_str_hash,
                                                        g_str_equal,
                                                        g_free,
                                                        g_object_unref);
}

static gboolean
gclue_service_manager_initable_init (GInitable    *initable,
                                     GCancellable *cancellable,
                                     GError      **error)
{
        return g_dbus_interface_skeleton_export
                (G_DBUS_INTERFACE_SKELETON (initable),
                 GCLUE_SERVICE_MANAGER (initable)->priv->connection,
                 "/org/freedesktop/GeoClue2/Manager",
                 error);
}

static void
gclue_service_manager_manager_iface_init (GClueManagerIface *iface)
{
        iface->handle_get_client = gclue_service_manager_handle_get_client;
}

static void
gclue_service_manager_initable_iface_init (GInitableIface *iface)
{
        iface->init = gclue_service_manager_initable_init;
}

GClueServiceManager *
gclue_service_manager_new (GDBusConnection *connection,
                           GError         **error)
{
        return g_initable_new (GCLUE_TYPE_SERVICE_MANAGER,
                               NULL,
                               error,
                               "connection", connection,
                               NULL);
}

guint
gclue_service_manager_get_connected_clients (GClueServiceManager *manager)
{
        return g_hash_table_size (manager->priv->clients);
}
