/* vim: set et ts=8 sw=8: */
/*
 * Copyright 2014 Red Hat, Inc.
 * Copyright 2015 Ankit (Verma)
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
 *          Ankit (Verma) <ankitstarski@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "gclue-config.h"
#include "gclue-location.h"
#include "gclue-nmea-utils.h"
#include "gclue-nmea-source.h"
#include "gclue-utils.h"
#include "config.h"
#include "gclue-enum-types.h"

#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-glib/glib-watch.h>
#include <gio/gunixsocketaddress.h>

/* Once we run out of NMEA services to try how long to wait
 * until retrying all of them.
 * In seconds.
 */
#define SERVICE_UNBREAK_TIME 5

typedef struct AvahiServiceInfo AvahiServiceInfo;

struct _GClueNMEASourcePrivate {
        GSocketConnection *connection;
        GDataInputStream *input_stream;

        GSocketClient *client;

        GCancellable *cancellable;

        AvahiGLibPoll *glib_poll;

        AvahiClient *avahi_client;

        AvahiServiceInfo *active_service;

        /* List of services to try but only the most accurate one is used. */
        GList *try_services;

        /* List of known-broken services. */
        GList *broken_services;

        guint accuracy_refresh_source, unbreak_timer;
};

G_DEFINE_TYPE_WITH_CODE (GClueNMEASource,
                         gclue_nmea_source,
                         GCLUE_TYPE_LOCATION_SOURCE,
                         G_ADD_PRIVATE (GClueNMEASource))

static GClueLocationSourceStartResult
gclue_nmea_source_start (GClueLocationSource *source);
static GClueLocationSourceStopResult
gclue_nmea_source_stop (GClueLocationSource *source);

static void
try_connect_to_service (GClueNMEASource *source);

struct AvahiServiceInfo {
    char *identifier;
    char *host_name;
    gboolean is_socket;
    guint16 port;
    GClueAccuracyLevel accuracy;
    gint64 timestamp_add;
};

static void
avahi_service_free (gpointer data)
{
        AvahiServiceInfo *service = (AvahiServiceInfo *) data;

        g_free (service->identifier);
        g_free (service->host_name);
        g_slice_free(AvahiServiceInfo, service);
}

static AvahiServiceInfo *
avahi_service_new (const char        *identifier,
                   const char        *host_name,
                   guint16            port,
                   GClueAccuracyLevel accuracy)
{
        AvahiServiceInfo *service = g_slice_new0 (AvahiServiceInfo);

        service->identifier = g_strdup (identifier);
        service->host_name = g_strdup (host_name);
        service->port = port;
        service->accuracy = accuracy;
        service->timestamp_add = g_get_monotonic_time ();

        return service;
}

static gint
compare_avahi_service_by_identifier (gconstpointer a,
                                     gconstpointer b)
{
        AvahiServiceInfo *first, *second;

        first = (AvahiServiceInfo *) a;
        second = (AvahiServiceInfo *) b;

        return g_strcmp0 (first->identifier, second->identifier);
}

static gint
compare_avahi_service_by_accuracy_n_time (gconstpointer a,
                                          gconstpointer b)
{
        AvahiServiceInfo *first, *second;
        gint diff;
        gint64 tdiff;

        first = (AvahiServiceInfo *) a;
        second = (AvahiServiceInfo *) b;

        diff = second->accuracy - first->accuracy;
        if (diff)
                return diff;

        g_assert (first->timestamp_add >= 0);
        g_assert (second->timestamp_add >= 0);
        tdiff = first->timestamp_add - second->timestamp_add;
        if (tdiff < 0)
                return -1;
        else if (tdiff > 0)
                return 1;
        else
                return 0;
}

static void
disconnect_from_service (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;

        if (!priv->active_service)
                return;

        g_cancellable_cancel (priv->cancellable);

        g_clear_object (&priv->input_stream);
        g_clear_object (&priv->connection);
        g_clear_object (&priv->client);
        g_clear_object (&priv->cancellable);
        priv->active_service = NULL;
}

static gboolean
reconnection_required (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;

        /* Basically, reconnection is required if either
         *
         * 1. service in use went down.
         * 2. a more accurate service than one currently in use, is now
         *    available.
         */
        return priv->active_service == NULL ||
                priv->try_services == NULL ||
                priv->active_service != priv->try_services->data;
}

static void
reconnect_service (GClueNMEASource *source)
{
        if (!reconnection_required (source))
                return;

        disconnect_from_service (source);
        try_connect_to_service (source);
}

static GClueAccuracyLevel get_head_accuracy (GList *list)
{
        AvahiServiceInfo *service;

        if (!list)
                return GCLUE_ACCURACY_LEVEL_NONE;

        service = (AvahiServiceInfo *) list->data;
        return service->accuracy;
}

static gboolean
on_refresh_accuracy_level (gpointer user_data)
{
        GClueNMEASource *source = GCLUE_NMEA_SOURCE (user_data);
        GClueNMEASourcePrivate *priv = source->priv;
        GClueAccuracyLevel new_try, new_broken, new, existing;

        priv->accuracy_refresh_source = 0;

        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (source));

        new_try = get_head_accuracy (priv->try_services);
        new_broken = get_head_accuracy (priv->broken_services);
        new = MAX (new_try, new_broken);

        if (new != existing) {
                g_debug ("Available accuracy level from %s: %u",
                         G_OBJECT_TYPE_NAME (source), new);
                g_object_set (G_OBJECT (source),
                              "available-accuracy-level", new,
                              NULL);
        }

        return G_SOURCE_REMOVE;
}

static void
refresh_accuracy_level (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;

        if (priv->accuracy_refresh_source) {
                return;
        }

        g_debug ("Scheduling NMEA accuracy level refresh");
        priv->accuracy_refresh_source = g_idle_add (on_refresh_accuracy_level,
                                                    source);
}

static gboolean
on_service_unbreak_time (gpointer source)
{
        GClueNMEASourcePrivate *priv = GCLUE_NMEA_SOURCE (source)->priv;

        priv->unbreak_timer = 0;

        if (!priv->try_services && priv->broken_services) {
                g_debug ("Unbreaking existing NMEA services");

                priv->try_services = priv->broken_services;
                priv->broken_services = NULL;

                reconnect_service (source);
        }

        return G_SOURCE_REMOVE;
}

static void
check_unbreak_timer (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;

        if (priv->try_services || !priv->broken_services) {
                if (priv->unbreak_timer) {
                        g_debug ("Removing unnecessary NMEA unbreaking timer");

                        g_source_remove (priv->unbreak_timer);
                        priv->unbreak_timer = 0;
                }

                return;
        }

        if (priv->unbreak_timer) {
                return;
        }

        g_debug ("Scheduling NMEA unbreaking timer");
        priv->unbreak_timer = g_timeout_add_seconds (SERVICE_UNBREAK_TIME,
                                                     on_service_unbreak_time,
                                                     source);
}

static void
service_lists_changed (GClueNMEASource *source)
{
        check_unbreak_timer (source);
        reconnect_service (source);
        refresh_accuracy_level (source);
}

static gboolean
check_service_exists (GClueNMEASource *source,
                      const char *name)
{
        GClueNMEASourcePrivate *priv = source->priv;
        AvahiServiceInfo *service;
        GList *item;
        gboolean ret = FALSE;

        /* only `name` is required here */
        service = avahi_service_new (name,
                                     NULL,
                                     0,
                                     GCLUE_ACCURACY_LEVEL_NONE);

        item = g_list_find_custom (priv->try_services,
                                   service,
                                   compare_avahi_service_by_identifier);
        if (item) {
                ret = TRUE;
        } else {
                item = g_list_find_custom (priv->broken_services,
                                           service,
                                           compare_avahi_service_by_identifier);
                if (item) {
                        ret = TRUE;
                }
        }

        g_clear_pointer (&service, avahi_service_free);

        return ret;
}

static void
add_new_service (GClueNMEASource *source,
                 const char *name,
                 const char *host_name,
                 uint16_t port,
                 gboolean is_socket,
                 AvahiStringList *txt)
{
        GClueAccuracyLevel accuracy = GCLUE_ACCURACY_LEVEL_NONE;
        AvahiServiceInfo *service;
        AvahiStringList *node;
        guint n_services;
        char *key, *value;
        GEnumClass *enum_class;
        GEnumValue *enum_value;

        if (check_service_exists (source, name)) {
                g_debug ("NMEA service %s already exists", name);
                return;
        }

        if (!txt) {
                accuracy = GCLUE_ACCURACY_LEVEL_EXACT;

                goto CREATE_SERVICE;
        }

        node = avahi_string_list_find (txt, "accuracy");

        if (node == NULL) {
                g_warning ("No `accuracy` key inside TXT record");
                accuracy = GCLUE_ACCURACY_LEVEL_EXACT;

                goto CREATE_SERVICE;
        }

        avahi_string_list_get_pair (node, &key, &value, NULL);

        if (value == NULL) {
                g_warning ("There is no value for `accuracy` inside TXT "
                           "record");
                accuracy = GCLUE_ACCURACY_LEVEL_EXACT;

                goto CREATE_SERVICE;
        }

        enum_class = g_type_class_ref (GCLUE_TYPE_ACCURACY_LEVEL);
        enum_value = g_enum_get_value_by_nick (enum_class, value);
        g_type_class_unref (enum_class);

        if (enum_value == NULL) {
                g_warning ("Invalid `accuracy` value `%s` inside TXT records.",
                           value);
                accuracy = GCLUE_ACCURACY_LEVEL_EXACT;

                goto CREATE_SERVICE;
        }

        accuracy = enum_value->value;

CREATE_SERVICE:
        service = avahi_service_new (name, host_name, port, accuracy);
        service->is_socket = is_socket;

        source->priv->try_services = g_list_insert_sorted
                (source->priv->try_services,
                 service,
                 compare_avahi_service_by_accuracy_n_time);

        n_services = g_list_length (source->priv->try_services);
        g_debug ("No. of _nmea-0183._tcp services %u", n_services);

        service_lists_changed (source);
}

static void
add_new_service_avahi (GClueNMEASource *source,
                       const char *name,
                       const char *host_name,
                       uint16_t port,
                       AvahiStringList *txt)
{
        add_new_service (source, name, host_name, port, FALSE, txt);
}

static void
add_new_service_socket (GClueNMEASource *source,
                       const char *name,
                       const char *socket_path)
{
        add_new_service (source, name, socket_path, 0, TRUE, NULL);
}

static void
service_broken (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;
        AvahiServiceInfo *service = priv->active_service;

        g_assert (service);

        disconnect_from_service (source);

        priv->try_services = g_list_remove (priv->try_services,
                                            service);
        priv->broken_services = g_list_insert_sorted
                (priv->broken_services,
                 service,
                compare_avahi_service_by_accuracy_n_time);

        service_lists_changed (source);
}

static void
remove_service_from_list (GList **list,
                          GList *item)
{
        AvahiServiceInfo *service = item->data;

        *list = g_list_delete_link (*list, item);
        avahi_service_free (service);
}

static void
remove_service_by_name (GClueNMEASource *source,
                        const char      *name)
{
        GClueNMEASourcePrivate *priv = source->priv;
        AvahiServiceInfo *service;
        GList *item;

        /* only `name` is required here */
        service = avahi_service_new (name,
                                     NULL,
                                     0,
                                     GCLUE_ACCURACY_LEVEL_NONE);

        item = g_list_find_custom (priv->try_services,
                                   service,
                                   compare_avahi_service_by_identifier);
        if (item) {
                if (item->data == priv->active_service) {
                        g_debug ("Active NMEA service removed, disconnecting.");
                        disconnect_from_service (source);
                }

                remove_service_from_list (&priv->try_services,
                                          item);
        } else {
                item = g_list_find_custom (priv->broken_services,
                                           service,
                                           compare_avahi_service_by_identifier);
                if (item) {
                        g_assert (item->data != priv->active_service);
                        remove_service_from_list (&priv->broken_services,
                                                  item);
                }
        }

        g_clear_pointer (&service, avahi_service_free);

        service_lists_changed (source);
}

static void
resolve_callback (AvahiServiceResolver  *service_resolver,
                  AvahiIfIndex           interface G_GNUC_UNUSED,
                  AvahiProtocol          protocol G_GNUC_UNUSED,
                  AvahiResolverEvent     event,
                  const char            *name,
                  const char            *type,
                  const char            *domain,
                  const char            *host_name,
                  const AvahiAddress    *address,
                  uint16_t               port,
                  AvahiStringList       *txt,
                  AvahiLookupResultFlags flags,
                  void                  *user_data)
{
        const char *errorstr;

        /* FIXME: check with Avahi devs whether this is really needed. */
        g_return_if_fail (service_resolver != NULL);

        switch (event) {
        case AVAHI_RESOLVER_FAILURE: {
                AvahiClient *avahi_client = avahi_service_resolver_get_client
                        (service_resolver);

                errorstr = avahi_strerror (avahi_client_errno (avahi_client));

                g_warning ("(Resolver) Failed to resolve service '%s' "
                           "of type '%s' in domain '%s': %s",
                           name,
                           type,
                           domain,
                           errorstr);

                break;
        }

        case AVAHI_RESOLVER_FOUND:
                g_debug ("Service '%s' of type '%s' in domain '%s' resolved to %s:%u",
                         name,
                         type,
                         domain,
                         host_name,
                         (unsigned int)port);

                add_new_service_avahi (GCLUE_NMEA_SOURCE (user_data),
                                       name,
                                       host_name,
                                       port,
                                       txt);

                break;
        }

    avahi_service_resolver_free (service_resolver);
}

static void
client_callback (AvahiClient     *avahi_client,
                 AvahiClientState state,
                 void            *user_data)
{
        g_return_if_fail (avahi_client != NULL);

        if (state == AVAHI_CLIENT_FAILURE) {
                const char *errorstr = avahi_strerror
                        (avahi_client_errno (avahi_client));
                g_warning ("Avahi client failure: %s",
                           errorstr);
        }
}

static void
browse_callback (AvahiServiceBrowser   *service_browser,
                 AvahiIfIndex           interface,
                 AvahiProtocol          protocol,
                 AvahiBrowserEvent      event,
                 const char            *name,
                 const char            *type,
                 const char            *domain,
                 AvahiLookupResultFlags flags G_GNUC_UNUSED,
                 void                  *user_data)
{
        GClueNMEASourcePrivate *priv = GCLUE_NMEA_SOURCE (user_data)->priv;
        const char *errorstr;

        /* FIXME: check with Avahi devs whether this is really needed. */
        g_return_if_fail (service_browser != NULL);

        switch (event) {
        case AVAHI_BROWSER_FAILURE:
                errorstr = avahi_strerror (avahi_client_errno
                        (avahi_service_browser_get_client (service_browser)));

                g_warning ("Avahi service browser Error %s", errorstr);

                return;

        case AVAHI_BROWSER_NEW: {
                AvahiServiceResolver *service_resolver;

                g_debug ("Service '%s' of type '%s' found in domain '%s'",
                         name, type, domain);

                service_resolver = avahi_service_resolver_new
                        (priv->avahi_client,
                         interface, protocol,
                         name, type,
                         domain,
                         AVAHI_PROTO_UNSPEC,
                         0,
                         resolve_callback,
                         user_data);

                if (service_resolver == NULL) {
                        errorstr = avahi_strerror
                                (avahi_client_errno (priv->avahi_client));

                        g_warning ("Failed to resolve service '%s': %s",
                                   name,
                                   errorstr);
                }

                break;
        }

        case AVAHI_BROWSER_REMOVE:
                g_debug ("Service '%s' of type '%s' in domain '%s' removed "
                         "from the list of available NMEA services",
                         name,
                         type,
                         domain);

                remove_service_by_name (GCLUE_NMEA_SOURCE (user_data), name);

                break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
                g_debug ("Avahi Service Browser's %s event occurred",
                         event == AVAHI_BROWSER_CACHE_EXHAUSTED ?
                         "CACHE_EXHAUSTED" :
                         "ALL_FOR_NOW");

                break;
        }
}

#define NMEA_LINE_END "\r\n"
#define NMEA_LINE_END_CTR (sizeof (NMEA_LINE_END) - 1)

static void nmea_skip_delim (GBufferedInputStream *stream,
                             GCancellable *cancellable)
{
        const char *buf;
        gsize buf_size;
        size_t delim_skip;
        g_autoptr(GError) error = NULL;

        buf = (const char *) g_buffered_input_stream_peek_buffer (stream,
                                                                  &buf_size);

        delim_skip = strnspn (buf, NMEA_LINE_END, buf_size);
        for (size_t ctr = 0; ctr < delim_skip; ctr++) {
                if (g_buffered_input_stream_read_byte (stream, cancellable, &error) < 0) {
                        if (error && !g_error_matches (error, G_IO_ERROR,
                                                       G_IO_ERROR_CANCELLED)) {
                                g_warning ("Failed to skip %zu / %zu NMEA delimiter: %s",
                                           ctr, delim_skip, error->message);
                        }
                        break;
                }
        }
}

static gboolean nmea_check_delim (GBufferedInputStream *stream)
{
        const char *buf;
        gsize buf_size;

        buf = (const char *) g_buffered_input_stream_peek_buffer (stream,
                                                                  &buf_size);

        return strnpbrk (buf, NMEA_LINE_END, buf_size) != NULL;
}

#define NMEA_STR_LEN 128
static void
on_read_nmea_sentence (GObject      *object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
        GClueNMEASource *source = NULL;
        GDataInputStream *data_input_stream = G_DATA_INPUT_STREAM (object);
        g_autoptr(GError) error = NULL;
        GClueLocation *prev_location;
        g_autoptr(GClueLocation) location = NULL;
        gsize data_size = 0 ;
        g_autofree char *message = NULL;
        gint i;
        const gchar *sentences[3];
        gchar gga[NMEA_STR_LEN];
        gchar rmc[NMEA_STR_LEN];

        message = g_data_input_stream_read_upto_finish (data_input_stream,
                                                        result,
                                                        &data_size,
                                                        &error);

        gga[0] = '\0';
        rmc[0] = '\0';

        do {
                if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                        return;

                if (!source)
                        source = GCLUE_NMEA_SOURCE (user_data);

                if (message == NULL) {
                        if (error != NULL) {
                                if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CLOSED)) {
                                        g_debug ("NMEA socket closed.");
                                } else {
                                        g_warning ("Error when receiving message: %s",
                                                   error->message);
                                }
                                service_broken (source);
                                return;
                        } else {
                                g_debug ("NMEA empty read");
                                /* GLib has a bug where g_data_input_stream_read_upto_finish
                                 * returns NULL when reading a line with only stop chars.
                                 * Convert this NULL to a zero-length message. See:
                                 * https://gitlab.gnome.org/GNOME/glib/-/issues/655
                                 */
                                message = g_strdup ("");
                        }

                }
                g_debug ("Network source sent: \"%s\"", message);

                if (gclue_nmea_type_is (message, "GGA")) {
                        g_strlcpy (gga, message, NMEA_STR_LEN);
                } else if (gclue_nmea_type_is (message, "RMC")) {
                        g_strlcpy (rmc, message, NMEA_STR_LEN);
                }

                nmea_skip_delim (G_BUFFERED_INPUT_STREAM (data_input_stream),
                                 source->priv->cancellable);

                if (nmea_check_delim (G_BUFFERED_INPUT_STREAM (data_input_stream))) {
                    g_clear_pointer (&message, g_free);
                    message = g_data_input_stream_read_upto
                            (data_input_stream,
                             NMEA_LINE_END, NMEA_LINE_END_CTR,
                             &data_size, NULL, &error);
                } else {
                    break;
                }
        } while (TRUE);

        i = 0;
        if (gga[0])
                sentences[i++] = gga;
        if (rmc[0])
                sentences[i++] = rmc;
        sentences[i] = NULL;

        if (i > 0) {
                prev_location = gclue_location_source_get_location
                        (GCLUE_LOCATION_SOURCE (source));
                location = gclue_location_create_from_nmeas (sentences,
                                                             prev_location);
                if (location) {
                        gclue_location_source_set_location
                                (GCLUE_LOCATION_SOURCE (source), location);
                }
        }

        g_data_input_stream_read_upto_async (data_input_stream,
                                             NMEA_LINE_END,
                                             NMEA_LINE_END_CTR,
                                             G_PRIORITY_DEFAULT,
                                             source->priv->cancellable,
                                             on_read_nmea_sentence,
                                             source);
}

static void
on_connection_to_location_server (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
        GSocketClient *client = G_SOCKET_CLIENT (object);
        GClueNMEASource *source;
        g_autoptr(GSocketConnection) connection = NULL;
        g_autoptr(GError) error = NULL;

        connection = g_socket_client_connect_to_host_finish
                (client,
                 result,
                 &error);

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                return;
        }

        source = GCLUE_NMEA_SOURCE (user_data);

        if (error != NULL) {
                g_warning ("Failed to connect to NMEA service: %s", error->message);
                service_broken (source);
                return;
        }

        g_assert (connection);
        g_debug ("NMEA service connected.");

        g_assert (!source->priv->connection);
        source->priv->connection = g_steal_pointer (&connection);

        g_assert (!source->priv->input_stream);
        source->priv->input_stream = g_data_input_stream_new
                (g_io_stream_get_input_stream (G_IO_STREAM (source->priv->connection)));

        g_data_input_stream_read_upto_async (source->priv->input_stream,
                                             NMEA_LINE_END,
                                             NMEA_LINE_END_CTR,
                                             G_PRIORITY_DEFAULT,
                                             source->priv->cancellable,
                                             on_read_nmea_sentence,
                                             source);
}

static void
try_connect_to_service (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;

        if (!gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (source))) {
                g_warn_if_fail (!priv->active_service);

                return;
        }

        if (priv->active_service)
                return;

        if (priv->try_services == NULL)
                return;

        g_assert (!priv->cancellable);
        priv->cancellable = g_cancellable_new ();

        g_assert (!priv->client);
        priv->client = g_socket_client_new ();

        /* The service with the highest accuracy will be stored in the beginning
         * of the list.
         */
        priv->active_service = (AvahiServiceInfo *) priv->try_services->data;

        g_debug ("Trying to connect to NMEA %sservice %s:%u.",
                 priv->active_service->is_socket ? "socket " : "",
                 priv->active_service->host_name,
                 (unsigned int) priv->active_service->port);

        if (!priv->active_service->is_socket) {
                g_socket_client_connect_to_host_async
                        (priv->client,
                         priv->active_service->host_name,
                         priv->active_service->port,
                         priv->cancellable,
                         on_connection_to_location_server,
                         source);
        } else {
                g_autoptr(GSocketAddress) addr = NULL;

                addr = g_unix_socket_address_new (priv->active_service->host_name);
                g_socket_client_connect_async (priv->client,
                               G_SOCKET_CONNECTABLE (addr),
                               priv->cancellable,
                               on_connection_to_location_server,
                               source);
        }
}

static gboolean
remove_avahi_services_from_list (GClueNMEASource *source, GList **list)
{
        GClueNMEASourcePrivate *priv = source->priv;
        gboolean removed_active = FALSE;
        GList *l = *list;

        while (l != NULL) {
                GList *next = l->next;
                AvahiServiceInfo *service = l->data;

                if (!service->is_socket) {
                        if (service == priv->active_service) {
                                g_debug ("Active NMEA service was Avahi-provided, disconnecting.");
                                disconnect_from_service (source);
                                removed_active = TRUE;
                        }

                        remove_service_from_list (list, l);
                }

                l = next;
        }

        return removed_active;
}

static void
disconnect_avahi_client (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv = source->priv;

        remove_avahi_services_from_list (source, &priv->try_services);
        if (remove_avahi_services_from_list (source, &priv->broken_services)) {
                g_warn_if_reached ();
        }

        g_clear_pointer (&priv->avahi_client, avahi_client_free);

        service_lists_changed (source);
}

static void
gclue_nmea_source_finalize (GObject *gnmea)
{
        GClueNMEASource *source = GCLUE_NMEA_SOURCE (gnmea);
        GClueNMEASourcePrivate *priv = source->priv;

        G_OBJECT_CLASS (gclue_nmea_source_parent_class)->finalize (gnmea);

        disconnect_avahi_client (source);
        disconnect_from_service (source);

        if (priv->accuracy_refresh_source) {
                g_source_remove (priv->accuracy_refresh_source);
                priv->accuracy_refresh_source = 0;
        }

        if (priv->unbreak_timer) {
                g_source_remove (priv->unbreak_timer);
                priv->unbreak_timer = 0;
        }

        g_clear_pointer (&priv->glib_poll, avahi_glib_poll_free);

        g_list_free_full (g_steal_pointer (&priv->try_services),
                          avahi_service_free);
        g_list_free_full (g_steal_pointer (&priv->broken_services),
                          avahi_service_free);
}

static void
gclue_nmea_source_class_init (GClueNMEASourceClass *klass)
{
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *gnmea_class = G_OBJECT_CLASS (klass);

        gnmea_class->finalize = gclue_nmea_source_finalize;

        source_class->start = gclue_nmea_source_start;
        source_class->stop = gclue_nmea_source_stop;
}

static void
try_connect_avahi_client (GClueNMEASource *source)
{
        AvahiServiceBrowser *service_browser;
        GClueNMEASourcePrivate *priv = source->priv;
        const AvahiPoll *poll_api;
        int error;

        if (priv->avahi_client) {
                AvahiClientState avahi_state;

                avahi_state = avahi_client_get_state (priv->avahi_client);
                if (avahi_state != AVAHI_CLIENT_FAILURE) {
                        return;
                }

                g_debug ("Avahi client in failure state, trying to reinit.");
                disconnect_avahi_client (source);
        }

        g_assert (priv->glib_poll);
        poll_api = avahi_glib_poll_get (priv->glib_poll);

        priv->avahi_client = avahi_client_new (poll_api,
                                               0,
                                               client_callback,
                                               source,
                                               &error);
        if (priv->avahi_client == NULL) {
                g_warning ("Failed to connect to avahi service: %s",
                           avahi_strerror (error));
                return;
        }

        service_browser = avahi_service_browser_new
                (priv->avahi_client,
                 AVAHI_IF_UNSPEC,
                 AVAHI_PROTO_UNSPEC,
                 "_nmea-0183._tcp",
                 NULL,
                 0,
                 browse_callback,
                 source);
        if (service_browser == NULL) {
                const char *errorstr;

                error = avahi_client_errno (priv->avahi_client);
                errorstr = avahi_strerror (error);
                g_warning ("Failed to browse avahi services: %s", errorstr);
                goto fail_client;
        }

        return;

fail_client:
        disconnect_avahi_client (source);
}

static void
gclue_nmea_source_init (GClueNMEASource *source)
{
        GClueNMEASourcePrivate *priv;
        const char *nmea_socket;
        GClueConfig *config;

        source->priv = gclue_nmea_source_get_instance_private (source);
        priv = source->priv;

        priv->glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);

        config = gclue_config_get_singleton ();

        nmea_socket = gclue_config_get_nmea_socket (config);
        if (nmea_socket != NULL) {
                add_new_service_socket (source,
                                        "nmea-socket",
                                        nmea_socket);
        }

        try_connect_avahi_client (source);
}

/**
 * gclue_nmea_source_get_singleton:
 *
 * Get the #GClueNMEASource singleton.
 *
 * Returns: (transfer full): a new ref to #GClueNMEASource. Use g_object_unref()
 * when done.
 **/
GClueNMEASource *
gclue_nmea_source_get_singleton (void)
{
        static GClueNMEASource *source = NULL;

        if (source == NULL) {
                source = g_object_new (GCLUE_TYPE_NMEA_SOURCE,
                                       "priority-source", TRUE,
                                       NULL);
                g_object_add_weak_pointer (G_OBJECT (source),
                                           (gpointer) &source);
        } else {
                g_object_ref (source);
                try_connect_avahi_client (source);
        }

        return source;
}

static GClueLocationSourceStartResult
gclue_nmea_source_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueLocationSourceStartResult base_result;

        g_return_val_if_fail (GCLUE_IS_NMEA_SOURCE (source),
                              GCLUE_LOCATION_SOURCE_START_RESULT_FAILED);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_nmea_source_parent_class);
        base_result = base_class->start (source);
        if (base_result == GCLUE_LOCATION_SOURCE_START_RESULT_FAILED)
                return base_result;

        try_connect_avahi_client (GCLUE_NMEA_SOURCE (source));
        reconnect_service (GCLUE_NMEA_SOURCE (source));

        return base_result;
}

static GClueLocationSourceStopResult
gclue_nmea_source_stop (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueLocationSourceStopResult base_result;

        g_return_val_if_fail (GCLUE_IS_NMEA_SOURCE (source), FALSE);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_nmea_source_parent_class);
        base_result = base_class->stop (source);
        if (base_result == GCLUE_LOCATION_SOURCE_STOP_RESULT_STILL_USED)
                return base_result;

        disconnect_from_service (GCLUE_NMEA_SOURCE (source));

        return base_result;
}
