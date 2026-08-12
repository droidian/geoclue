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
#include "gclue-mozilla.h"
#include "config.h"

/**
 * SECTION:gclue-web-source
 * @short_description: Web-based geolocation
 * @include: gclue-glib/gclue-web-source.h
 *
 * Baseclass for all sources that solely use a web resource for geolocation.
 **/

struct _GClueWebSourcePrivate {
        GCancellable *cancellable;

        GClueAccuracyLevel accuracy_level;

        SoupSession *soup_session;

        SoupMessage *query;
        const char *query_data_description;

        gulong network_changed_id;
        gulong connectivity_changed_id;

        guint64 last_submitted;

        const char *locate_url;
        const char *submit_url;
        gboolean locate_url_reachable;
        gboolean submit_url_reachable;
        gboolean refresh_needed;
};

enum
{
        PROP_0,
        PROP_ACCURACY_LEVEL,
        LAST_PROP
};
static GParamSpec *gParamSpecs[LAST_PROP];

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GClueWebSource,
                                  gclue_web_source,
                                  GCLUE_TYPE_LOCATION_SOURCE,
                                  G_ADD_PRIVATE (GClueWebSource))

static void refresh_callback (SoupSession  *session,
                              GAsyncResult *result,
                              gpointer      user_data);

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

        gclue_web_source_refresh_available_accuracy_level (source);

        if (!gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (source))) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                                         "Source is inactive");
                return;
        }

        if (!source->priv->locate_url_reachable) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NETWORK_UNREACHABLE,
                                         "Cannot reach locate URL");
                return;
        }

        if (source->priv->query != NULL) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_PENDING,
                                         "Refresh already in progress");
                return;
        }

        source->priv->query = GCLUE_WEB_SOURCE_GET_CLASS (source)->create_query
                (source, &source->priv->query_data_description, &local_error);
        if (source->priv->query == NULL) {
                g_task_return_error (task, g_steal_pointer (&local_error));
                return;
        }

        soup_session_send_and_read_async (source->priv->soup_session,
                                          source->priv->query,
                                          G_PRIORITY_DEFAULT,
                                          cancellable,
                                          (GAsyncReadyCallback)refresh_callback,
                                          g_steal_pointer (&task));
}

static void
refresh_callback (SoupSession  *session,
                  GAsyncResult *result,
                  gpointer      user_data)
{
        g_autoptr(GTask) task = g_steal_pointer (&user_data);
        GClueWebSource *web;
        g_autoptr(SoupMessage) query = NULL;
        g_autoptr(GBytes) body = NULL;
        g_autoptr(GError) local_error = NULL;
        g_autofree char *contents = NULL;
        g_autofree char *str = NULL;
        g_autofree char *short_contents = NULL;
        g_autoptr(GClueLocation) location = NULL;
        GUri *uri;

        web = GCLUE_WEB_SOURCE (g_task_get_source_object (task));
        query = g_steal_pointer (&web->priv->query);

        body = soup_session_send_and_read_finish (session, result, &local_error);
        if (!body) {
                g_task_return_error (task, g_steal_pointer (&local_error));
                return;
        }

        if (soup_message_get_status (query) != SOUP_STATUS_OK) {
                g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                         "Query location SOUP error: %s",
                                         soup_message_get_reason_phrase (query));
                return;
        }

        contents = g_strndup (g_bytes_get_data (body, NULL), g_bytes_get_size (body));
        uri = soup_message_get_uri (query);
        str = g_uri_to_string (uri);
        short_contents = g_strndup (contents, 256);
        g_debug ("Got a response of %" G_GSIZE_FORMAT " bytes from '%s' starting with:\n%s",
                 strlen(contents), str, short_contents);
        location = GCLUE_WEB_SOURCE_GET_CLASS (web)->parse_response (web,
                                                                     contents,
                                                                     &local_error);
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

        /* Ignore returned location */
        GClueLocation *location = GCLUE_WEB_SOURCE_GET_CLASS (web)->refresh_finish (web, result, &local_error);
        if (location)
                g_object_unref (location);

        if (local_error != NULL &&
            !g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                if (!g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED) &&
                    !g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_PENDING)) {
                        g_warning ("Failed to query location: %s",
                                   local_error->message);
                } else {
                        g_debug ("Failed to query location: %s",
                                 local_error->message);
                }
        }
}

void
gclue_web_source_refresh_available_accuracy_level (GClueWebSource *web)
{
        GClueAccuracyLevel new, existing;

        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (web));
        new = GCLUE_WEB_SOURCE_GET_CLASS (web)->get_available_accuracy_level
                        (web, web->priv->locate_url_reachable);
        if (new != existing) {
                g_debug ("Available accuracy level from %s: %u",
                         G_OBJECT_TYPE_NAME (web), new);
                g_object_set (G_OBJECT (web),
                              "available-accuracy-level", new,
                              NULL);
        }
}

static gboolean
get_internet_available (void)
{
        GNetworkMonitor *monitor = g_network_monitor_get_default ();

        return g_network_monitor_get_connectivity (monitor) ==
                G_NETWORK_CONNECTIVITY_FULL;
}

/* This should be equal to WIFI_SCAN_TIMEOUT_HIGH_ACCURACY in gclue-wifi.c */
#define WEB_LOCATION_TIMEOUT 10

static void
locate_url_checked_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
        GNetworkMonitor *mon = G_NETWORK_MONITOR (source_object);
        GClueWebSource *web;
        GClueLocation *current_location;
        gboolean reachable, last_reachable;
        g_autoptr(GError) error = NULL;

        reachable = g_network_monitor_can_reach_finish (mon, result, &error);
        if (error && g_error_matches (error, G_IO_ERROR,
                                      G_IO_ERROR_CANCELLED)) {
                return; /* WebSource instance is finalized */
        }

        if (!reachable && get_internet_available ()) {
                g_debug ("Locate URL not reachable, but Internet is available, overriding");
                reachable = TRUE;
        }

        web = GCLUE_WEB_SOURCE (user_data);
        last_reachable = web->priv->locate_url_reachable;
        web->priv->locate_url_reachable = reachable;
        if (last_reachable != reachable) {
                g_debug ("Network changed: %s",
                         reachable ? "Enabling locate URL queries" :
                                     "Disabling locate URL queries");
        }
        if (!reachable)
                return;

        current_location = gclue_location_source_get_location (GCLUE_LOCATION_SOURCE (web));
        if (!current_location || (g_get_real_time () / G_USEC_PER_SEC)
            > (gclue_location_get_timestamp (current_location) + WEB_LOCATION_TIMEOUT)) {
                g_debug ("Network changed: Refreshing");
                gclue_web_source_refresh_available_accuracy_level (web);
                if (gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (web)))
                        gclue_web_source_refresh (web);
                else
                        web->priv->refresh_needed = TRUE; /* postpone to start */
        }
}

static void
submit_url_checked_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
        GNetworkMonitor *mon = G_NETWORK_MONITOR (source_object);
        GClueWebSource *web;
        gboolean reachable, last_reachable;
        g_autoptr(GError) error = NULL;

        reachable = g_network_monitor_can_reach_finish (mon, result, &error);
        if (error && g_error_matches (error, G_IO_ERROR,
                                      G_IO_ERROR_CANCELLED)) {
                return; /* WebSource instance is finalized */
        }

        if (!reachable && get_internet_available ()) {
                g_debug ("Submit URL not reachable, but Internet is available, overriding");
                reachable = TRUE;
        }

        web = GCLUE_WEB_SOURCE (user_data);
        last_reachable = web->priv->submit_url_reachable;
        web->priv->submit_url_reachable = reachable;
        if (last_reachable == reachable) {
                return;
        }
        g_debug ("Network changed: %s",
                 reachable ? "Enabling submit URL queries" :
                             "Disabling submit URL queries");
}

static void
cancellable_cancel_recreate (GClueWebSource *source)
{
        GClueWebSourcePrivate *priv = source->priv;

        g_cancellable_cancel (priv->cancellable);

        g_clear_object (&priv->cancellable);
        priv->cancellable = g_cancellable_new ();
}

static void
on_network_changed (GNetworkMonitor *unused_monitor G_GNUC_UNUSED,
                    gboolean         available G_GNUC_UNUSED,
                    gpointer         user_data)
{
        GNetworkMonitor *monitor = g_network_monitor_get_default ();
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        g_autoptr(GSocketConnectable) submit_addr = NULL;
        g_autoptr(GSocketConnectable) locate_addr = NULL;

        cancellable_cancel_recreate (web);

        if (web->priv->submit_url) {
                submit_addr = g_network_address_parse_uri (web->priv->submit_url,
                                                           80, NULL);
                if (submit_addr) {
                        g_network_monitor_can_reach_async (monitor,
                                                           submit_addr,
                                                           web->priv->cancellable,
                                                           submit_url_checked_cb,
                                                           web);
                } else {
                        g_warning ("Could not parse submit URL '%s'",
                                   web->priv->submit_url);
                        web->priv->submit_url_reachable = FALSE;
                }
        } else {
                web->priv->submit_url_reachable = FALSE;
        }

        if (web->priv->locate_url) {
                locate_addr = g_network_address_parse_uri (web->priv->locate_url,
                                                           80, NULL);
                if (locate_addr) {
                        g_network_monitor_can_reach_async (monitor,
                                                           locate_addr,
                                                           web->priv->cancellable,
                                                           locate_url_checked_cb,
                                                           web);
                } else {
                        g_warning ("Could not parse locate URL '%s'",
                                   web->priv->locate_url);
                        web->priv->locate_url_reachable = FALSE;
                }
        } else {
                web->priv->locate_url_reachable = FALSE;
        }
}

static void
on_connectivity_changed (GObject    *gobject,
                         GParamSpec *pspec,
                         gpointer    user_data)
{
        on_network_changed (NULL, FALSE, user_data);
}

static GClueLocationSourceStartResult
gclue_web_source_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueWebSource *web;
        GClueLocationSourceStartResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source),
                              GCLUE_LOCATION_SOURCE_START_RESULT_FAILED);
        web = GCLUE_WEB_SOURCE (source);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_web_source_parent_class);
        base_result = base_class->start (source);
        if (base_result != GCLUE_LOCATION_SOURCE_START_RESULT_OK)
                return base_result;

        if (web->priv->refresh_needed) {
                web->priv->refresh_needed = FALSE;
                gclue_web_source_refresh (web);
        }

        return base_result;
}

static void
gclue_web_source_finalize (GObject *gsource)
{
        GClueWebSourcePrivate *priv = GCLUE_WEB_SOURCE (gsource)->priv;

        g_cancellable_cancel (priv->cancellable);

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

        if (priv->soup_session) {
                soup_session_abort (priv->soup_session);
                g_clear_object (&priv->soup_session);
        }

        g_clear_object (&priv->query);
        g_clear_object (&priv->cancellable);

        G_OBJECT_CLASS (gclue_web_source_parent_class)->finalize (gsource);
}

static char *
get_os_info (void)
{
        g_autofree char *pretty_name = NULL;
        g_autofree char *os_name = g_get_os_info (G_OS_INFO_KEY_NAME);
        g_autofree char *os_version = g_get_os_info (G_OS_INFO_KEY_VERSION);

        if (os_name && os_version)
                return g_strdup_printf ("%s; %s", os_name, os_version);

        pretty_name = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);
        if (pretty_name)
                return g_steal_pointer (&pretty_name);

        /* Translators: Not marked as translatable as debug output should stay English */
        return g_strdup ("Unknown");
}

#define USER_AGENT (PACKAGE_NAME "/" PACKAGE_VERSION)

static char *
get_user_agent (void)
{
        g_autofree char *os_info = get_os_info ();
        return g_strdup_printf ("%s (%s)", USER_AGENT, os_info);
}

static void
gclue_web_source_constructed (GObject *object)
{
        GNetworkMonitor *monitor;
        GClueWebSourcePrivate *priv = GCLUE_WEB_SOURCE (object)->priv;
        g_autofree char *user_agent = get_user_agent ();

        G_OBJECT_CLASS (gclue_web_source_parent_class)->constructed (object);

        priv->soup_session = soup_session_new ();
        soup_session_set_proxy_resolver (priv->soup_session, NULL);
        soup_session_set_user_agent (priv->soup_session, user_agent);

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
gclue_web_source_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                g_value_set_enum (value, web->priv->accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_web_source_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                web->priv->accuracy_level = g_value_get_enum (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_web_source_class_init (GClueWebSourceClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);

        klass->refresh_async = gclue_web_source_real_refresh_async;
        klass->refresh_finish = gclue_web_source_real_refresh_finish;

        source_class->start = gclue_web_source_start;

        object_class->get_property = gclue_web_source_get_property;
        object_class->set_property = gclue_web_source_set_property;
        object_class->finalize = gclue_web_source_finalize;
        object_class->constructed = gclue_web_source_constructed;

        gParamSpecs[PROP_ACCURACY_LEVEL] = g_param_spec_enum ("accuracy-level",
                                                              "AccuracyLevel",
                                                              "Max accuracy level",
                                                              GCLUE_TYPE_ACCURACY_LEVEL,
                                                              GCLUE_ACCURACY_LEVEL_CITY,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_ACCURACY_LEVEL]);
}

static void
gclue_web_source_init (GClueWebSource *web)
{
        web->priv = gclue_web_source_get_instance_private (web);
        web->priv->cancellable = g_cancellable_new ();
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
submit_query_callback (SoupSession  *session,
                       GAsyncResult *result,
                       gpointer      user_data)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        g_autoptr(GBytes) body = NULL;
        g_autoptr(GError) local_error = NULL;
        g_autofree char *contents = NULL;
        SoupMessage *query;
        g_autofree char *uri_str = NULL;
        gint status_code;

        query = soup_session_get_async_result_message (session, result);
        uri_str = g_uri_to_string (soup_message_get_uri (query));

        body = soup_session_send_and_read_finish (session, result, &local_error);
        if (!body) {
                g_warning ("Failed to submit location data to '%s' (no body): %s",
                           uri_str, local_error->message);
                return;
        }
        contents = g_strndup (g_bytes_get_data (body, NULL), g_bytes_get_size (body));

        status_code = soup_message_get_status (query);

        if (!GCLUE_WEB_SOURCE_GET_CLASS (web)->parse_submit_response
                        (web, contents, status_code, &local_error)) {
                g_warning ("Failed to submit location data to '%s': %s",
                           uri_str, soup_message_get_reason_phrase (query));
                return;
        }

        g_debug ("Successfully submitted location data to '%s'", uri_str);
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
        g_autoptr(SoupMessage) query = NULL;
        g_autoptr(GError) error = NULL;

        if (!web->priv->submit_url_reachable)
                return;

        location = gclue_location_source_get_location (source);
        if (location == NULL ||
            gclue_location_get_accuracy (location) >
            SUBMISSION_ACCURACY_THRESHOLD ||
            gclue_location_get_accuracy (location) ==
            GCLUE_LOCATION_ACCURACY_UNKNOWN ||
            gclue_location_get_timestamp (location) <
            web->priv->last_submitted + SUBMISSION_TIME_THRESHOLD)
                return;

        web->priv->last_submitted = gclue_location_get_timestamp (location);

        query = GCLUE_WEB_SOURCE_GET_CLASS (web)->create_submit_query
                                        (web,
                                         location,
                                         &error);
        if (query == NULL) {
                if (error != NULL) {
                        g_warning ("Failed to create submission query: %s",
                                   error->message);
                }

                return;
        }

        soup_session_send_and_read_async (web->priv->soup_session,
                                          query,
                                          G_PRIORITY_DEFAULT,
                                          NULL,
                                          (GAsyncReadyCallback)submit_query_callback,
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

const char *
gclue_web_source_get_locate_url (GClueWebSource      *source)
{
        return source->priv->locate_url;
}

void
gclue_web_source_set_locate_url (GClueWebSource *source,
                                 const char     *url)
{
        source->priv->locate_url = url;
}

const char *
gclue_web_source_get_submit_url (GClueWebSource      *source)
{
        return source->priv->submit_url;
}

void
gclue_web_source_set_submit_url (GClueWebSource *source,
                                 const char     *url)
{
        source->priv->submit_url = url;
}

const char *gclue_web_source_get_query_data_description
                                        (GClueWebSource      *source)
{
        return source->priv->query_data_description;
}
