/* vim: set et ts=8 sw=8: */
/*
 * Copyright 2014 Red Hat, Inc.
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
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <stdlib.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include "gclue-web-source.h"
#include "gclue-error.h"
#include "gclue-location.h"

/**
 * SECTION:gclue-web-source
 * @short_description: Web-based geolocation
 * @include: gclue-glib/gclue-web-source.h
 *
 * Baseclass for all sources that solely use a web resource for geolocation.
 **/

static gboolean
get_internet_available (void);
static void
refresh_accuracy_level (GClueWebSource *web);

struct _GClueWebSourcePrivate {
        SoupSession *soup_session;

        SoupMessage *query;

        gulong network_changed_id;
        gulong connectivity_changed_id;

        guint64 last_submitted;

        gboolean internet_available;
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GClueWebSource,
                                  gclue_web_source,
                                  GCLUE_TYPE_LOCATION_SOURCE,
                                  G_ADD_PRIVATE (GClueWebSource))

static void refresh_callback (SoupSession *session,
                              SoupMessage *query,
                              gpointer     user_data);

static void
gclue_web_source_real_refresh_async (GClueWebSource      *source,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
        g_autoptr(GTask) task = NULL;
        g_autoptr(GError) local_error = NULL;

        task = g_task_new (source, cancellable, callback, user_data);
        g_task_set_source_tag (task, gclue_web_source_real_refresh_async);

        refresh_accuracy_level (source);

        if (!gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (source))) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                                         "Source is inactive");
                return;
        }

        if (!get_internet_available ()) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NETWORK_UNREACHABLE,
                                         "Network unavailable");
                return;
        }
        g_debug ("Network available");

        if (source->priv->query != NULL) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_PENDING,
                                         "Refresh already in progress");
                return;
        }

        source->priv->query = GCLUE_WEB_SOURCE_GET_CLASS (source)->create_query (source, &local_error);

        if (source->priv->query == NULL) {
                g_task_return_error (task, g_steal_pointer (&local_error));
                return;
        }

        /* TODO handle cancellation */
        soup_session_queue_message (source->priv->soup_session,
                                    source->priv->query,
                                    refresh_callback,
                                    g_steal_pointer (&task));
}

static void
refresh_callback (SoupSession *session,
                  SoupMessage *query,
                  gpointer     user_data)
{
        g_autoptr(GTask) task = g_steal_pointer (&user_data);
        GClueWebSource *web;
        g_autoptr(GError) local_error = NULL;
        g_autofree char *contents = NULL;
        g_autofree char *str = NULL;
        g_autoptr(GClueLocation) location = NULL;
        SoupURI *uri;

        if (query->status_code == SOUP_STATUS_CANCELLED) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                         "Operation cancelled");
                return;
        }

        web = GCLUE_WEB_SOURCE (g_task_get_source_object (task));
        web->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                         "Failed to query location: %s", query->reason_phrase);
                return;
        }

        contents = g_strndup (query->response_body->data, query->response_body->length);
        uri = soup_message_get_uri (query);
        str = soup_uri_to_string (uri, FALSE);
        g_debug ("Got following response from '%s':\n%s", str, contents);
        location = GCLUE_WEB_SOURCE_GET_CLASS (web)->parse_response (web, contents, &local_error);
        if (local_error != NULL) {
                g_task_return_error (task, g_steal_pointer (&local_error));
                return;
        }

        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (web),
                                            location);

        g_task_return_pointer (task, g_steal_pointer (&location), g_object_unref);
}


static GClueLocation *
gclue_web_source_real_refresh_finish (GClueWebSource  *source,
                                      GAsyncResult    *result,
                                      GError         **error)
{
        GTask *task = G_TASK (result);

        return g_task_propagate_pointer (task, error);
}

static void
query_callback (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (source_object);
        g_autoptr(GError) local_error = NULL;
        g_autoptr(GClueLocation) location = NULL;

        location = GCLUE_WEB_SOURCE_GET_CLASS (web)->refresh_finish (web, result, &local_error);

        if (local_error != NULL &&
            !g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED)) {
                g_warning ("Failed to query location: %s", local_error->message);
                return;
        }
}

static gboolean
get_internet_available (void)
{
        GNetworkMonitor *monitor = g_network_monitor_get_default ();
        gboolean available;

        available = (g_network_monitor_get_connectivity (monitor) ==
                     G_NETWORK_CONNECTIVITY_FULL);

        return available;
}

static void
refresh_accuracy_level (GClueWebSource *web)
{
        GClueAccuracyLevel new, existing;

        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (web));
        new = GCLUE_WEB_SOURCE_GET_CLASS (web)->get_available_accuracy_level
                        (web, web->priv->internet_available);
        if (new != existing) {
                g_debug ("Available accuracy level from %s: %u",
                         G_OBJECT_TYPE_NAME (web), new);
                g_object_set (G_OBJECT (web),
                              "available-accuracy-level", new,
                              NULL);
        }
}

static void
on_network_changed (GNetworkMonitor *monitor G_GNUC_UNUSED,
                    gboolean         available G_GNUC_UNUSED,
                    gpointer         user_data)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        gboolean last_available = web->priv->internet_available;

        web->priv->internet_available = get_internet_available ();
        if (last_available == web->priv->internet_available)
                return; /* We already reacted to network change */

        GCLUE_WEB_SOURCE_GET_CLASS (web)->refresh_async (web, NULL, query_callback, NULL);
}

static void
on_connectivity_changed (GObject    *gobject,
                         GParamSpec *pspec,
                         gpointer    user_data)
{
        on_network_changed (NULL, FALSE, user_data);
}

static void
gclue_web_source_finalize (GObject *gsource)
{
        GClueWebSourcePrivate *priv = GCLUE_WEB_SOURCE (gsource)->priv;

        if (priv->network_changed_id) {
                g_signal_handler_disconnect (g_network_monitor_get_default (),
                                             priv->network_changed_id);
                priv->network_changed_id = 0;
        }

        if (priv->connectivity_changed_id) {
                g_signal_handler_disconnect (g_network_monitor_get_default (),
                                             priv->connectivity_changed_id);
                priv->connectivity_changed_id = 0;
        }

        if (priv->query != NULL) {
                g_debug ("Cancelling query");
                soup_session_cancel_message (priv->soup_session,
                                             priv->query,
                                             SOUP_STATUS_CANCELLED);
                priv->query = NULL;
        }

        g_clear_object (&priv->soup_session);

        G_OBJECT_CLASS (gclue_web_source_parent_class)->finalize (gsource);
}

static void
gclue_web_source_constructed (GObject *object)
{
        GNetworkMonitor *monitor;
        GClueWebSourcePrivate *priv = GCLUE_WEB_SOURCE (object)->priv;

        G_OBJECT_CLASS (gclue_web_source_parent_class)->constructed (object);

        priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);

        monitor = g_network_monitor_get_default ();
        priv->network_changed_id =
                g_signal_connect (monitor,
                                  "network-changed",
                                  G_CALLBACK (on_network_changed),
                                  object);
        priv->connectivity_changed_id =
                g_signal_connect (monitor,
                                  "notify::connectivity",
                                  G_CALLBACK (on_connectivity_changed),
                                  object);
        on_network_changed (NULL,
                            TRUE,
                            object);
}

static void
gclue_web_source_class_init (GClueWebSourceClass *klass)
{
        GObjectClass *gsource_class = G_OBJECT_CLASS (klass);

        klass->refresh_async = gclue_web_source_real_refresh_async;
        klass->refresh_finish = gclue_web_source_real_refresh_finish;

        gsource_class->finalize = gclue_web_source_finalize;
        gsource_class->constructed = gclue_web_source_constructed;
}

static void
gclue_web_source_init (GClueWebSource *web)
{
        web->priv = gclue_web_source_get_instance_private (web);
}

/**
 * gclue_web_source_refresh:
 * @source: a #GClueWebSource
 *
 * Causes @source to refresh location and available accuracy level. Its meant
 * to be used by subclasses if they have reason to suspect location and/or
 * available accuracy level might have changed.
 **/
void
gclue_web_source_refresh (GClueWebSource *source)
{
        g_return_if_fail (GCLUE_IS_WEB_SOURCE (source));

        GCLUE_WEB_SOURCE_GET_CLASS (source)->refresh_async (source, NULL, query_callback, NULL);
}

static void
submit_query_callback (SoupSession *session,
                       SoupMessage *query,
                       gpointer     user_data)
{
        SoupURI *uri;
        g_autofree char *str = NULL;

        uri = soup_message_get_uri (query);
        str = soup_uri_to_string (uri, FALSE);
        if (query->status_code != SOUP_STATUS_OK &&
            query->status_code != SOUP_STATUS_NO_CONTENT) {
                g_warning ("Failed to submit location data to '%s': %s",
                           str,
                           query->reason_phrase);
		return;
	}

        g_debug ("Successfully submitted location data to '%s'",
                 str);
}

#define SUBMISSION_ACCURACY_THRESHOLD 100
#define SUBMISSION_TIME_THRESHOLD     60  /* seconds */

static void
on_submit_source_location_notify (GObject    *source_object,
                                  GParamSpec *pspec,
                                  gpointer    user_data)
{
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (source_object);
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        GClueLocation *location;
        SoupMessage *query;
        GError *error = NULL;

        location = gclue_location_source_get_location (source);
        if (location == NULL ||
            gclue_location_get_accuracy (location) >
            SUBMISSION_ACCURACY_THRESHOLD ||
            gclue_location_get_timestamp (location) <
            web->priv->last_submitted + SUBMISSION_TIME_THRESHOLD)
                return;

        web->priv->last_submitted = gclue_location_get_timestamp (location);

        if (!get_internet_available ())
                return;

        query = GCLUE_WEB_SOURCE_GET_CLASS (web)->create_submit_query
                                        (web,
                                         location,
                                         &error);
        if (query == NULL) {
                if (error != NULL) {
                        g_warning ("Failed to create submission query: %s",
                                   error->message);
                        g_error_free (error);
                }

                return;
        }

        soup_session_queue_message (web->priv->soup_session,
                                    query,
                                    submit_query_callback,
                                    web);
}

/**
 * gclue_web_source_set_submit_source:
 * @source: a #GClueWebSource
 *
 * Use this function to provide a location source to @source that is used
 * for submitting location data to resource being used by @source. This will be
 * a #GClueModemGPS but we don't assume that here, in case we later add a
 * non-modem GPS source and would like to pass that instead.
 **/
void
gclue_web_source_set_submit_source (GClueWebSource      *web,
                                    GClueLocationSource *submit_source)
{
        /* Not implemented by subclass */
        if (GCLUE_WEB_SOURCE_GET_CLASS (web)->create_submit_query == NULL)
                return;

        g_signal_connect_object (G_OBJECT (submit_source),
                                 "notify::location",
                                 G_CALLBACK (on_submit_source_location_notify),
                                 G_OBJECT (web),
                                 0);

        on_submit_source_location_notify (G_OBJECT (submit_source), NULL, web);
}
