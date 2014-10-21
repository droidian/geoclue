/* vim: set et ts=8 sw=8: */
/* gclue-service-client.c
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

#include "gclue-service-client.h"
#include "gclue-service-location.h"
#include "gclue-locator.h"
#include "public-api/gclue-enum-types.h"
#include "gclue-config.h"

#define DEFAULT_ACCURACY_LEVEL GCLUE_ACCURACY_LEVEL_CITY

static void
gclue_service_client_client_iface_init (GClueClientIface *iface);
static void
gclue_service_client_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceClient,
                         gclue_service_client,
                         GCLUE_TYPE_CLIENT_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_CLIENT,
                                                gclue_service_client_client_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                gclue_service_client_initable_iface_init));

struct _GClueServiceClientPrivate
{
        GClueClientInfo *client_info;
        const char *path;
        GDBusConnection *connection;
        GClueAgent *agent_proxy;

        GClueServiceLocation *location;
        GClueServiceLocation *prev_location;
        guint threshold;

        GClueLocator *locator;

        /* Number of times location has been updated */
        guint locations_updated;

        gboolean agent_stopped; /* Agent stopped client, not the app */
};

enum
{
        PROP_0,
        PROP_CLIENT_INFO,
        PROP_PATH,
        PROP_CONNECTION,
        PROP_AGENT_PROXY,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static char *
next_location_path (GClueServiceClient *client)
{
        GClueServiceClientPrivate *priv = client->priv;
        char *path, *index_str;

        index_str = g_strdup_printf ("%u", (priv->locations_updated)++),
        path = g_strjoin ("/", priv->path, "Location", index_str, NULL);
        g_free (index_str);

        return path;
}

/* We don't use the gdbus-codegen provided gclue_client_emit_location_updated()
 * as that sends the signal to all listeners on the bus
 */
static gboolean
emit_location_updated (GClueServiceClient *client,
                       const char         *old,
                       const char         *new,
                       GError            **error)
{
        GClueServiceClientPrivate *priv = client->priv;
        GVariant *variant;
        const char *peer;

        variant = g_variant_new ("(oo)", old, new);
        peer = gclue_client_info_get_bus_name (priv->client_info);

        return g_dbus_connection_emit_signal (priv->connection,
                                              peer,
                                              priv->path,
                                              "org.freedesktop.GeoClue2.Client",
                                              "LocationUpdated",
                                              variant,
                                              error);
}

static gboolean
below_threshold (GClueServiceClient *client,
                 GeocodeLocation    *location)
{
        GClueServiceClientPrivate *priv = client->priv;
        GeocodeLocation *cur_location;
        gdouble distance;
        gdouble threshold_km;

        if (priv->threshold == 0)
                return FALSE;

        g_object_get (priv->location,
                      "location", &cur_location,
                      NULL);
        distance = geocode_location_get_distance_from (cur_location,
                                                       location);
        g_object_unref (cur_location);

        threshold_km = priv->threshold / 1000.0;
        if (distance < threshold_km)
                return TRUE;

        return FALSE;
}

static void
on_locator_location_changed (GObject    *gobject,
                             GParamSpec *pspec,
                             gpointer    user_data)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (user_data);
        GClueServiceClientPrivate *priv = client->priv;
        GClueLocationSource *locator = GCLUE_LOCATION_SOURCE (gobject);
        GeocodeLocation *location_info;
        char *path = NULL;
        const char *prev_path;
        GError *error = NULL;

        location_info = gclue_location_source_get_location (locator);
        if (location_info == NULL)
                return; /* No location found yet */

        if (priv->location != NULL && below_threshold (client, location_info)) {
                g_debug ("Updating location, below threshold");
                g_object_set (priv->location,
                              "location", location_info,
                              NULL);
                return;
        }

        g_clear_object (&priv->prev_location);
        priv->prev_location = priv->location;

        path = next_location_path (client);
        priv->location = gclue_service_location_new (priv->client_info,
                                                     path,
                                                     priv->connection,
                                                     location_info,
                                                     &error);
        if (priv->location == NULL)
                goto error_out;

        if (priv->prev_location != NULL)
                prev_path = gclue_service_location_get_path (priv->prev_location);
        else
                prev_path = "/";

        gclue_client_set_location (GCLUE_CLIENT (client), path);

        if (!emit_location_updated (client, prev_path, path, &error))
                goto error_out;
        goto out;

error_out:
        g_warning ("Failed to update location info: %s", error->message);
        g_error_free (error);
out:
        g_free (path);
}

static void
start_client (GClueServiceClient *client, GClueAccuracyLevel accuracy_level)
{
        GClueServiceClientPrivate *priv = client->priv;

        gclue_client_set_active (GCLUE_CLIENT (client), TRUE);
        priv->locator = gclue_locator_new (accuracy_level);
        g_signal_connect (priv->locator,
                          "notify::location",
                          G_CALLBACK (on_locator_location_changed),
                          client);

        gclue_location_source_start (GCLUE_LOCATION_SOURCE (priv->locator));
}

static void
stop_client (GClueServiceClient *client)
{
        g_clear_object (&client->priv->locator);
        gclue_client_set_active (GCLUE_CLIENT (client), FALSE);
}

static void
on_agent_props_changed (GDBusProxy *agent_proxy,
                        GVariant   *changed_properties,
                        GStrv       invalidated_properties,
                        gpointer    user_data)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (user_data);
        GVariantIter *iter;
        GVariant *value;
        gchar *key;
        
        if (g_variant_n_children (changed_properties) < 0)
                return;

        g_variant_get (changed_properties, "a{sv}", &iter);
        while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
                GClueAccuracyLevel max_accuracy;
                GClueConfig *config;
                const char *id;

                if (strcmp (key, "MaxAccuracyLevel") != 0)
                        continue;

                config = gclue_config_get_singleton ();
                id = gclue_client_get_desktop_id (GCLUE_CLIENT (client));
                max_accuracy = g_variant_get_uint32 (value);
                /* FIXME: We should be handling all values of max accuracy
                 *        level here, not just 0 and non-0.
                 */
                if (max_accuracy != 0 && client->priv->agent_stopped) {
                        GClueAccuracyLevel accuracy;

                        client->priv->agent_stopped = FALSE;
                        accuracy = gclue_client_get_requested_accuracy_level
                                (GCLUE_CLIENT (client));
                        accuracy = CLAMP (accuracy, 0, max_accuracy);

                        start_client (client, accuracy);
                        g_debug ("Re-started '%s'.", id);
                } else if (max_accuracy == 0 &&
                           gclue_client_get_active (GCLUE_CLIENT (client)) &&
                           !gclue_config_is_system_component (config, id)) {
                        stop_client (client);
                        client->priv->agent_stopped = TRUE;
                        g_debug ("Stopped '%s'.", id);
                }

                break;
        }
        g_variant_iter_free (iter);
}

typedef struct
{
        GClueServiceClient *client;
        GDBusMethodInvocation *invocation;
} StartData;

static void
start_data_free (StartData *data)
{
        g_object_unref (data->client);
        g_object_unref (data->invocation);
        g_slice_free (StartData, data);
}

static void
complete_start (StartData *data, GClueAccuracyLevel accuracy_level)
{
        start_client (data->client, accuracy_level);

        gclue_client_complete_start (GCLUE_CLIENT (data->client),
                                     data->invocation);
        g_debug ("'%s' started.",
                 gclue_client_get_desktop_id (GCLUE_CLIENT (data->client)));
        start_data_free (data);
}

static void
on_authorize_app_ready (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
        StartData *data = (StartData *) user_data;
        GError *error = NULL;
        gboolean authorized = FALSE;
        GClueAccuracyLevel accuracy_level;

        accuracy_level = gclue_client_get_requested_accuracy_level
                                (GCLUE_CLIENT (data->client));
        if (!gclue_agent_call_authorize_app_finish (GCLUE_AGENT (source_object),
                                                    &authorized,
                                                    &accuracy_level,
                                                    res,
                                                    &error))
                goto error_out;

        if (!authorized) {
                g_set_error_literal (&error,
                                     G_DBUS_ERROR,
                                     G_DBUS_ERROR_ACCESS_DENIED,
                                     "Access denied");
                goto error_out;
        }

        complete_start (data, accuracy_level);

        return;

error_out:
        g_dbus_method_invocation_take_error (data->invocation, error);
        start_data_free (data);
}

static gboolean
gclue_service_client_handle_start (GClueClient           *client,
                                   GDBusMethodInvocation *invocation)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (client)->priv;
        GClueConfig *config;
        StartData *data;
        const char *desktop_id;
        GClueAccuracyLevel accuracy_level, max_accuracy;
        GClueAppPerm app_perm;
        guint32 uid;

        if (priv->locator != NULL)
                /* Already started */
                return TRUE;

        desktop_id = gclue_client_get_desktop_id (client);
        if (desktop_id == NULL) {
                g_dbus_method_invocation_return_error (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "'DesktopId' property must be set");
                return TRUE;
        }

        config = gclue_config_get_singleton ();
        uid = gclue_client_info_get_user_id (priv->client_info);
        app_perm = gclue_config_get_app_perm (config,
                                              desktop_id,
                                              priv->client_info);
        if (app_perm == GCLUE_APP_PERM_DISALLOWED) {
                g_dbus_method_invocation_return_error (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "'%s' disallowed by "
                                                       "configuration for UID %u",
                                                       desktop_id,
                                                       uid);
                return TRUE;
        }

        data = g_slice_new (StartData);
        data->client = g_object_ref (client);
        data->invocation =  g_object_ref (invocation);

        accuracy_level = gclue_client_get_requested_accuracy_level (client);

        /* No agent == No authorization needed */
        if (priv->agent_proxy == NULL ||
            gclue_config_is_system_component (config, desktop_id) ||
            app_perm == GCLUE_APP_PERM_ALLOWED) {
                complete_start (data, accuracy_level);

                return TRUE;
        }

        max_accuracy = gclue_agent_get_max_accuracy_level (priv->agent_proxy);
        if (max_accuracy == 0) {
                g_dbus_method_invocation_return_error (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "Geolocation disabled for"
                                                       " UID %u",
                                                       uid);
                start_data_free (data);
                return TRUE;
        }
        g_debug ("requested accuracy level: %u. "
                 "Accuracy level allowed by agent: %u",
                 accuracy_level, max_accuracy);
        accuracy_level = CLAMP (accuracy_level, 0, max_accuracy);

        gclue_agent_call_authorize_app (priv->agent_proxy,
                                        desktop_id,
                                        accuracy_level,
                                        NULL,
                                        on_authorize_app_ready,
                                        data);

        return TRUE;
}

static gboolean
gclue_service_client_handle_stop (GClueClient           *client,
                                  GDBusMethodInvocation *invocation)
{
        stop_client (GCLUE_SERVICE_CLIENT (client));
        gclue_client_complete_stop (client, invocation);
        g_debug ("'%s' stopped.", gclue_client_get_desktop_id (client));

        return TRUE;
}

static void
gclue_service_client_finalize (GObject *object)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (object)->priv;

        g_clear_pointer (&priv->path, g_free);
        g_clear_object (&priv->connection);
        g_signal_handlers_disconnect_by_func (priv->agent_proxy,
                                              G_CALLBACK (on_agent_props_changed),
                                              object);
        g_clear_object (&priv->agent_proxy);
        g_clear_object (&priv->locator);
        g_clear_object (&priv->location);
        g_clear_object (&priv->prev_location);
        g_clear_object (&priv->client_info);

        /* Chain up to the parent class */
        G_OBJECT_CLASS (gclue_service_client_parent_class)->finalize (object);
}

static void
gclue_service_client_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (object);

        switch (prop_id) {
        case PROP_CLIENT_INFO:
                g_value_set_object (value, client->priv->client_info);
                break;

        case PROP_PATH:
                g_value_set_string (value, client->priv->path);
                break;

        case PROP_CONNECTION:
                g_value_set_object (value, client->priv->connection);
                break;

        case PROP_AGENT_PROXY:
                g_value_set_object (value, client->priv->agent_proxy);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_client_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (object);

        switch (prop_id) {
        case PROP_CLIENT_INFO:
                client->priv->client_info = g_value_dup_object (value);
                break;

        case PROP_PATH:
                client->priv->path = g_value_dup_string (value);
                break;

        case PROP_CONNECTION:
                client->priv->connection = g_value_dup_object (value);
                break;

        case PROP_AGENT_PROXY:
                client->priv->agent_proxy = g_value_dup_object (value);
                g_signal_connect (client->priv->agent_proxy,
                                  "g-properties-changed",
                                  G_CALLBACK (on_agent_props_changed),
                                  object);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_client_handle_method_call (GDBusConnection       *connection,
                                         const gchar           *sender,
                                         const gchar           *object_path,
                                         const gchar           *interface_name,
                                         const gchar           *method_name,
                                         GVariant              *parameters,
                                         GDBusMethodInvocation *invocation,
                                         gpointer               user_data)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (user_data)->priv;
        GDBusInterfaceSkeletonClass *skeleton_class;
        GDBusInterfaceVTable *skeleton_vtable;

        if (!gclue_client_info_check_bus_name (priv->client_info, sender)) {
                g_dbus_method_invocation_return_error (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "Access denied");
                return;
        }

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (gclue_service_client_parent_class);
        skeleton_vtable = skeleton_class->get_vtable (G_DBUS_INTERFACE_SKELETON (user_data));
        skeleton_vtable->method_call (connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      method_name,
                                      parameters,
                                      invocation,
                                      user_data);
}

static GVariant *
gclue_service_client_handle_get_property (GDBusConnection *connection,
                                          const gchar     *sender,
                                          const gchar     *object_path,
                                          const gchar     *interface_name,
                                          const gchar     *property_name,
                                          GError         **error,
                                          gpointer        user_data)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (user_data)->priv;
        GDBusInterfaceSkeletonClass *skeleton_class;
        GDBusInterfaceVTable *skeleton_vtable;

        if (!gclue_client_info_check_bus_name (priv->client_info, sender)) {
                g_set_error (error,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_ACCESS_DENIED,
                             "Access denied");
                return NULL;
        }

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (gclue_service_client_parent_class);
        skeleton_vtable = skeleton_class->get_vtable (G_DBUS_INTERFACE_SKELETON (user_data));
        return skeleton_vtable->get_property (connection,
                                              sender,
                                              object_path,
                                              interface_name,
                                              property_name,
                                              error,
                                              user_data);
}

static gboolean
gclue_service_client_handle_set_property (GDBusConnection *connection,
                                          const gchar     *sender,
                                          const gchar     *object_path,
                                          const gchar     *interface_name,
                                          const gchar     *property_name,
                                          GVariant        *variant,
                                          GError         **error,
                                          gpointer        user_data)
{
        GClueClient *client = GCLUE_CLIENT (user_data);
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (client)->priv;
        GDBusInterfaceSkeletonClass *skeleton_class;
        GDBusInterfaceVTable *skeleton_vtable;
        gboolean ret;

        if (!gclue_client_info_check_bus_name (priv->client_info, sender)) {
                g_set_error (error,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_ACCESS_DENIED,
                             "Access denied");
                return FALSE;
        }

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (gclue_service_client_parent_class);
        skeleton_vtable = skeleton_class->get_vtable (G_DBUS_INTERFACE_SKELETON (user_data));
        ret = skeleton_vtable->set_property (connection,
                                             sender,
                                             object_path,
                                             interface_name,
                                             property_name,
                                             variant,
                                             error,
                                             user_data);
        if (ret && strcmp (property_name, "DistanceThreshold") == 0) {
                priv->threshold = gclue_client_get_distance_threshold (client);
                g_debug ("New distance threshold: %u", priv->threshold);
        }

        return ret;
}

static const GDBusInterfaceVTable gclue_service_client_vtable =
{
        gclue_service_client_handle_method_call,
        gclue_service_client_handle_get_property,
        gclue_service_client_handle_set_property,
        {NULL}
};

static GDBusInterfaceVTable *
gclue_service_client_get_vtable (GDBusInterfaceSkeleton *skeleton G_GNUC_UNUSED)
{
        return (GDBusInterfaceVTable *) &gclue_service_client_vtable;
}

static void
gclue_service_client_class_init (GClueServiceClientClass *klass)
{
        GObjectClass *object_class;
        GDBusInterfaceSkeletonClass *skeleton_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_client_finalize;
        object_class->get_property = gclue_service_client_get_property;
        object_class->set_property = gclue_service_client_set_property;

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (klass);
        skeleton_class->get_vtable = gclue_service_client_get_vtable;

        g_type_class_add_private (object_class, sizeof (GClueServiceClientPrivate));

        gParamSpecs[PROP_CLIENT_INFO] = g_param_spec_object ("client-info",
                                                             "ClientInfo",
                                                             "Information on client",
                                                             GCLUE_TYPE_CLIENT_INFO,
                                                             G_PARAM_READWRITE |
                                                             G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_CLIENT_INFO,
                                         gParamSpecs[PROP_CLIENT_INFO]);

        gParamSpecs[PROP_PATH] = g_param_spec_string ("path",
                                                      "Path",
                                                      "Path",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_PATH,
                                         gParamSpecs[PROP_PATH]);

        gParamSpecs[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                            "Connection",
                                                            "DBus Connection",
                                                            G_TYPE_DBUS_CONNECTION,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_CONNECTION,
                                         gParamSpecs[PROP_CONNECTION]);

        gParamSpecs[PROP_AGENT_PROXY] = g_param_spec_object ("agent-proxy",
                                                             "AgentProxy",
                                                             "Proxy to app authorization agent",
                                                             GCLUE_TYPE_AGENT_PROXY,
                                                             G_PARAM_READWRITE |
                                                             G_PARAM_CONSTRUCT);
        g_object_class_install_property (object_class,
                                         PROP_AGENT_PROXY,
                                         gParamSpecs[PROP_AGENT_PROXY]);
}

static void
gclue_service_client_client_iface_init (GClueClientIface *iface)
{
        iface->handle_start = gclue_service_client_handle_start;
        iface->handle_stop = gclue_service_client_handle_stop;
}

static gboolean
gclue_service_client_initable_init (GInitable    *initable,
                                    GCancellable *cancellable,
                                    GError      **error)
{
        if (!g_dbus_interface_skeleton_export
                                (G_DBUS_INTERFACE_SKELETON (initable),
                                 GCLUE_SERVICE_CLIENT (initable)->priv->connection,
                                 GCLUE_SERVICE_CLIENT (initable)->priv->path,
                                 error))
                return FALSE;

        return TRUE;
}

static void
gclue_service_client_initable_iface_init (GInitableIface *iface)
{
        iface->init = gclue_service_client_initable_init;
}

static void
gclue_service_client_init (GClueServiceClient *client)
{
        client->priv = G_TYPE_INSTANCE_GET_PRIVATE (client,
                                                    GCLUE_TYPE_SERVICE_CLIENT,
                                                    GClueServiceClientPrivate);
        gclue_client_set_requested_accuracy_level (GCLUE_CLIENT (client),
                                                   DEFAULT_ACCURACY_LEVEL);
}

GClueServiceClient *
gclue_service_client_new (GClueClientInfo *info,
                          const char      *path,
                          GDBusConnection *connection,
                          GClueAgent      *agent_proxy,
                          GError         **error)
{
        return g_initable_new (GCLUE_TYPE_SERVICE_CLIENT,
                               NULL,
                               error,
                               "client-info", info,
                               "path", path,
                               "connection", connection,
                               "agent-proxy", agent_proxy,
                               NULL);
}

const gchar *
gclue_service_client_get_path (GClueServiceClient *client)
{
        g_return_val_if_fail (GCLUE_IS_SERVICE_CLIENT(client), NULL);

        return client->priv->path;
}

GClueClientInfo *
gclue_service_client_get_client_info (GClueServiceClient *client)
{
        g_return_val_if_fail (GCLUE_IS_SERVICE_CLIENT(client), NULL);

        return client->priv->client_info;
}
