/* vim: set et ts=8 sw=8: */
/* gclue-service-agent.c
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
#include <libnotify/notify.h>

#include "gclue-service-agent.h"

static void
gclue_service_agent_agent_iface_init (GClueAgentIface *iface);
static void
gclue_service_agent_async_initable_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceAgent,
                         gclue_service_agent,
                         GCLUE_TYPE_AGENT_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_AGENT,
                                                gclue_service_agent_agent_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                gclue_service_agent_async_initable_init))

struct _GClueServiceAgentPrivate
{
        GDBusConnection *connection;
};

enum
{
        PROP_0,
        PROP_CONNECTION,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_service_agent_finalize (GObject *object)
{
        GClueServiceAgentPrivate *priv = GCLUE_SERVICE_AGENT (object)->priv;

        g_clear_object (&priv->connection);

        /* Chain up to the parent class */
        G_OBJECT_CLASS (gclue_service_agent_parent_class)->finalize (object);
}

static void
gclue_service_agent_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GClueServiceAgent *agent = GCLUE_SERVICE_AGENT (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                g_value_set_object (value, agent->priv->connection);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_agent_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
        GClueServiceAgent *agent = GCLUE_SERVICE_AGENT (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                agent->priv->connection = g_value_dup_object (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_agent_class_init (GClueServiceAgentClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_agent_finalize;
        object_class->get_property = gclue_service_agent_get_property;
        object_class->set_property = gclue_service_agent_set_property;

        g_type_class_add_private (object_class, sizeof (GClueServiceAgentPrivate));

        gParamSpecs[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                            "Connection",
                                                            "DBus Connection",
                                                            G_TYPE_DBUS_CONNECTION,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_CONNECTION,
                                         gParamSpecs[PROP_CONNECTION]);
}

static void
gclue_service_agent_init (GClueServiceAgent *agent)
{
        agent->priv = G_TYPE_INSTANCE_GET_PRIVATE (agent,
                                                   GCLUE_TYPE_SERVICE_AGENT,
                                                   GClueServiceAgentPrivate);
}

static void
on_add_agent_ready (GObject      *source_object,
                    GAsyncResult *res,
                    gpointer      user_data)
{
        GTask *task = G_TASK (user_data);
        GVariant *results;
        GError *error = NULL;

        results = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object),
                                            res,
                                            &error);
        if (results == NULL) {
                g_task_return_error (task, error);
                g_object_unref (task);
                return;
        }

        g_task_return_boolean (task, TRUE);

        g_object_unref (task);
}

static void
on_manager_proxy_ready (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
        GTask *task = G_TASK (user_data);
        GDBusProxy *proxy;
        GError *error = NULL;

        proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
        if (proxy == NULL) {
                g_task_return_error (task, error);
                g_object_unref (task);
                return;
        }

        g_dbus_proxy_call (proxy,
                           "AddAgent",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           g_task_get_cancellable (task),
                           on_add_agent_ready,
                           task);
}

static void
gclue_service_agent_init_async (GAsyncInitable     *initable,
                                int                 io_priority,
                                GCancellable       *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer            user_data)
{
        GTask *task;
        char *path;
        GError *error = NULL;

        task = g_task_new (initable, cancellable, callback, user_data);

        path = g_strdup_printf ("/org/freedesktop/GeoClue2/Agent/%u", getuid ());
        if (!g_dbus_interface_skeleton_export
                (G_DBUS_INTERFACE_SKELETON (initable),
                 GCLUE_SERVICE_AGENT (initable)->priv->connection,
                 path,
                 &error)) {
                g_task_return_error (task, error);
                g_object_unref (task);
                g_free (path);
                return;
        }
        g_free (path);

        g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  NULL,
                                  "org.freedesktop.GeoClue2",
                                  "/org/freedesktop/GeoClue2/Manager",
                                  "org.freedesktop.GeoClue2.Manager",
                                  cancellable,
                                  on_manager_proxy_ready,
                                  task);
}

static gboolean
gclue_service_agent_init_finish (GAsyncInitable *initable,
                                 GAsyncResult   *result,
                                 GError        **error)
{
        return g_task_propagate_boolean (G_TASK (result), error);
}

typedef struct
{
        GClueAgent *agent;
        GDBusMethodInvocation *invocation;
        NotifyNotification *notification;
        char *desktop_id;
        gboolean authorized;
} NotificationData;

static void
notification_data_free (NotificationData *data)
{
        g_free (data->desktop_id);
        g_object_unref (data->notification);
        g_slice_free (NotificationData, data);
}

#define ACTION_YES "yes"
#define ACTION_NO "NO"

static void
on_notify_action (NotifyNotification *notification,
                  char               *action,
                  gpointer            user_data)
{
        NotificationData *data = (NotificationData *) user_data;
        GError *error = NULL;

        data->authorized = (g_strcmp0 (action, ACTION_YES) == 0);

        if (!notify_notification_close (notification, &error)) {
                g_dbus_method_invocation_take_error (data->invocation, error);
                notification_data_free (data);
        }
}

static void
on_notify_closed (NotifyNotification *notification,
                  gpointer            user_data)
{
        NotificationData *data = (NotificationData *) user_data;

        if (data->authorized)
                g_debug ("Authorized %s", data->desktop_id);
        else
                g_debug ("%s not authorized", data->desktop_id);
        gclue_agent_complete_authorize_app (data->agent,
                                            data->invocation,
                                            data->authorized);
        notification_data_free (data);
}

static gboolean
gclue_service_agent_handle_authorize_app (GClueAgent            *agent,
                                          GDBusMethodInvocation *invocation,
                                          const char            *desktop_id)
{
        NotifyNotification *notification;
        NotificationData *data;
        GError *error = NULL;
        char *msg;

        msg = g_strdup_printf (_("Allow %s to access your location information?"),
                               desktop_id);
        notification = notify_notification_new (_("Geolocation"), msg, "dialog-question");
        g_free (msg);

        data = g_slice_new0 (NotificationData);
        data->invocation = invocation;
        data->notification = notification;
        data->desktop_id = g_strdup (desktop_id);

        notify_notification_add_action (notification,
                                        ACTION_YES,
                                        _("Yes"),
                                        on_notify_action,
                                        data,
                                        NULL);
        notify_notification_add_action (notification,
                                        ACTION_NO,
                                        _("No"),
                                        on_notify_action,
                                        data,
                                        NULL);
        g_signal_connect (notification,
                          "closed",
                          G_CALLBACK (on_notify_closed),
                          data);

        if (!notify_notification_show (notification, &error)) {
                g_dbus_method_invocation_take_error (invocation, error);
                notification_data_free (data);

                return TRUE;
        }

        return TRUE;
}

static void
gclue_service_agent_agent_iface_init (GClueAgentIface *iface)
{
        iface->handle_authorize_app = gclue_service_agent_handle_authorize_app;
}

static void
gclue_service_agent_async_initable_init (GAsyncInitableIface *iface)
{
        iface->init_async = gclue_service_agent_init_async;
        iface->init_finish = gclue_service_agent_init_finish;
}

void
gclue_service_agent_new_async (GDBusConnection    *connection,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data)
{
        g_async_initable_new_async (GCLUE_TYPE_SERVICE_AGENT,
                                    G_PRIORITY_DEFAULT,
                                    cancellable,
                                    callback,
                                    user_data,
                                    "connection", connection,
                                    NULL);
}

GClueServiceAgent *
gclue_service_agent_new_finish (GAsyncResult *res,
                                GError      **error)
{
        GObject *object;
        GObject *source_object;

        source_object = g_async_result_get_source_object (res);
        object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
                                              res,
                                              error);
        g_object_unref (source_object);
        if (object != NULL)
                return GCLUE_SERVICE_AGENT (object);
        else
                return NULL;
}
