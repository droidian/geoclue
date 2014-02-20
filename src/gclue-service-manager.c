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
#include <config.h>

#include "gclue-service-manager.h"
#include "gclue-service-client.h"
#include "gclue-client-info.h"
#include "geoclue-agent-interface.h"
#include "gclue-enums.h"
#include "gclue-config.h"

#define AGENT_WAIT_TIMEOUT      100    /* milliseconds */
#define AGENT_WAIT_TIMEOUT_USEC 100000 /* microseconds */

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
        GHashTable *agents;

        guint num_clients;
        gint64 init_time;
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
on_peer_vanished (GClueClientInfo *info,
                  gpointer         user_data)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (user_data);

        g_hash_table_remove (manager->priv->clients,
                             gclue_client_info_get_bus_name (info));
        g_object_notify (G_OBJECT (manager), "connected-clients");
        if (g_hash_table_size (manager->priv->clients) == 0)
                gclue_manager_set_in_use (GCLUE_MANAGER (manager), FALSE);
}

typedef struct
{
        GClueManager *manager;
        GDBusMethodInvocation *invocation;
        GClueClientInfo *client_info;
} OnClientInfoNewReadyData;

static gboolean
complete_get_client (OnClientInfoNewReadyData *data)
{
        GClueServiceManagerPrivate *priv = GCLUE_SERVICE_MANAGER (data->manager)->priv;
        GClueServiceClient *client;
        GClueClientInfo *info = data->client_info;
        GClueAgent *agent_proxy = NULL;
        GError *error = NULL;
        char *path;
        guint32 user_id;

        user_id = gclue_client_info_get_user_id (info);
        agent_proxy = g_hash_table_lookup (priv->agents,
                                           GINT_TO_POINTER (user_id));

        path = g_strdup_printf ("/org/freedesktop/GeoClue2/Client/%u",
                                ++priv->num_clients);

        client = gclue_service_client_new (info,
                                           path,
                                           priv->connection,
                                           agent_proxy,
                                           &error);
        if (client == NULL)
                goto error_out;

        g_hash_table_insert (priv->clients,
                             g_strdup (gclue_client_info_get_bus_name (info)),
                             client);
        g_object_notify (G_OBJECT (data->manager), "connected-clients");
        if (g_hash_table_size (priv->clients) == 1)
                gclue_manager_set_in_use (data->manager, TRUE);

        g_signal_connect (info,
                          "peer-vanished",
                          G_CALLBACK (on_peer_vanished),
                          data->manager);

        gclue_manager_complete_get_client (data->manager, data->invocation, path);
        goto out;

error_out:
        g_dbus_method_invocation_return_error (data->invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "%s", error->message);
out:
        g_clear_error (&error);
        g_clear_object (&info);
        g_slice_free (OnClientInfoNewReadyData, data);

        return FALSE;
}

static void
on_client_info_new_ready (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
        OnClientInfoNewReadyData *data = (OnClientInfoNewReadyData *) user_data;
        GClueServiceManagerPrivate *priv = GCLUE_SERVICE_MANAGER (data->manager)->priv;
        GClueClientInfo *info = NULL;
        GClueAgent *agent_proxy;
        GError *error = NULL;
        guint32 user_id;
        gint64 now;

        info = gclue_client_info_new_finish (res, &error);
        if (info == NULL) {
                g_dbus_method_invocation_return_error (data->invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_FAILED,
                                                       "%s", error->message);
                g_error_free (error);
                g_slice_free (OnClientInfoNewReadyData, data);

                return;
        }

        data->client_info = info;

        user_id = gclue_client_info_get_user_id (info);
        agent_proxy = g_hash_table_lookup (priv->agents,
                                           GINT_TO_POINTER (user_id));
        now = g_get_monotonic_time ();
        if (agent_proxy == NULL &&
            now < (priv->init_time + AGENT_WAIT_TIMEOUT_USEC)) {
                /* Its possible that geoclue was just launched on GetClient
                 * call, in which case agents need some time to register
                 * themselves to us.
                 */
                g_timeout_add (AGENT_WAIT_TIMEOUT,
                               (GSourceFunc) complete_get_client,
                               user_data);
                return;
        }

        complete_get_client (data);
}

static gboolean
gclue_service_manager_handle_get_client (GClueManager          *manager,
                                         GDBusMethodInvocation *invocation)
{
        GClueServiceManager *self = GCLUE_SERVICE_MANAGER (manager);
        GClueServiceManagerPrivate *priv = self->priv;
        GClueServiceClient *client;
        const char *peer;
        OnClientInfoNewReadyData *data;

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

        data = g_slice_new (OnClientInfoNewReadyData);
        data->manager = manager;
        data->invocation = invocation;
        gclue_client_info_new_async (peer,
                                     priv->connection,
                                     NULL,
                                     on_client_info_new_ready,
                                     data);
        return TRUE;
}

typedef struct
{
        GClueManager *manager;
        GDBusMethodInvocation *invocation;
        GClueClientInfo *info;
        char *desktop_id;
} AddAgentData;

static void
add_agent_data_free (AddAgentData *data)
{
        g_clear_pointer (&data->desktop_id, g_free);
        g_slice_free (AddAgentData, data);
}

static void
on_agent_vanished (GClueClientInfo *info,
                   gpointer         user_data)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (user_data);
        guint32 user_id;

        user_id = gclue_client_info_get_user_id (info);
        g_debug ("Agent for user '%u' vanished", user_id);
        g_hash_table_remove (manager->priv->agents, GINT_TO_POINTER (user_id));
        g_object_unref (info);
}

static void
on_agent_proxy_ready (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
        AddAgentData *data = (AddAgentData *) user_data;
        GClueServiceManagerPrivate *priv = GCLUE_SERVICE_MANAGER (data->manager)->priv;
        guint32 user_id;
        GClueAgent *agent;
        GError *error = NULL;

        agent = gclue_agent_proxy_new_for_bus_finish (res, &error);
        if (agent == NULL)
            goto error_out;

        user_id = gclue_client_info_get_user_id (data->info);
        g_debug ("New agent for user ID '%u'", user_id);
        g_hash_table_replace (priv->agents, GINT_TO_POINTER (user_id), agent);

        g_signal_connect (data->info,
                          "peer-vanished",
                          G_CALLBACK (on_agent_vanished),
                          data->manager);

        gclue_manager_complete_add_agent (data->manager, data->invocation);

        goto out;

error_out:
        g_dbus_method_invocation_return_error (data->invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "%s", error->message);
out:
        g_clear_error (&error);
        add_agent_data_free (data);
}

#define AGENT_PATH "/org/freedesktop/GeoClue2/Agent/%u"

static void
on_agent_info_new_ready (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        AddAgentData *data = (AddAgentData *) user_data;
        GError *error = NULL;
        char *path;
        GClueConfig *config;

        data->info = gclue_client_info_new_finish (res, &error);
        if (data->info == NULL) {
                g_dbus_method_invocation_return_error (data->invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_FAILED,
                                                       "%s", error->message);
                g_error_free (error);
                add_agent_data_free (data);

                return;
        }

        config = gclue_config_get_singleton ();
        if (!gclue_config_is_agent_allowed (config,
                                            data->desktop_id,
                                            data->info)) {
                g_dbus_method_invocation_return_error (data->invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "Not whitelisted");
                add_agent_data_free (data);

                return;
        }

        path = g_strdup_printf (AGENT_PATH,
                                gclue_client_info_get_user_id (data->info));
        gclue_agent_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       gclue_client_info_get_bus_name (data->info),
                                       path,
                                       NULL,
                                       on_agent_proxy_ready,
                                       user_data);
        g_free (path);
}

static gboolean
gclue_service_manager_handle_add_agent (GClueManager          *manager,
                                        GDBusMethodInvocation *invocation,
                                        const char            *id)
{
        GClueServiceManager *self = GCLUE_SERVICE_MANAGER (manager);
        GClueServiceManagerPrivate *priv = self->priv;
        const char *peer;
        AddAgentData *data;

        peer = g_dbus_method_invocation_get_sender (invocation);

        data = g_slice_new0 (AddAgentData);
        data->manager = manager;
        data->invocation = invocation;
        data->desktop_id = g_strdup (id);
        gclue_client_info_new_async (peer,
                                     priv->connection,
                                     NULL,
                                     on_agent_info_new_ready,
                                     data);
        return TRUE;
}

static void
gclue_service_manager_finalize (GObject *object)
{
        GClueServiceManagerPrivate *priv = GCLUE_SERVICE_MANAGER (object)->priv;

        g_clear_object (&priv->connection);
        g_clear_pointer (&priv->clients, g_hash_table_unref);
        g_clear_pointer (&priv->agents, g_hash_table_unref);

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
gclue_service_manager_constructed (GObject *object)
{
        /* FIXME: We need to probe the sources, somehow */
        gclue_manager_set_available_accuracy_level (GCLUE_MANAGER (object),
                                                    GCLUE_ACCURACY_LEVEL_EXACT);
}

static void
gclue_service_manager_class_init (GClueServiceManagerClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_manager_finalize;
        object_class->get_property = gclue_service_manager_get_property;
        object_class->set_property = gclue_service_manager_set_property;
        object_class->constructed = gclue_service_manager_constructed;

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
        manager->priv->agents = g_hash_table_new_full (g_direct_hash,
                                                       g_direct_equal,
                                                       NULL,
                                                       g_object_unref);
        manager->priv->init_time = g_get_monotonic_time ();
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
        iface->handle_add_agent = gclue_service_manager_handle_add_agent;
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
