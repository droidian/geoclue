/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2013 Satabdi Das
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
 * Authors: Satabdi Das <satabdidas@gmail.com>
 *          Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <stdlib.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include "gclue-ipclient.h"
#include "gclue-error.h"
#include "geoip-server/geoip-server.h"
#include "geocode-location.h"

#define GEOIP_SERVER "http://geoip.fedoraproject.org/city"

/**
 * SECTION:gclue-ipclient
 * @short_description: GeoIP client
 * @include: gclue-glib/gclue-ipclient.h
 *
 * Contains functions to get the geolocation corresponding to IP addresses from a server.
 **/

enum {
        PROP_0,
        PROP_SERVER,
        PROP_COMPAT_MODE,
        N_PROPERTIES
};

struct _GClueIpclientPrivate {
        SoupSession *soup_session;

        char *ip;

};

G_DEFINE_TYPE (GClueIpclient, gclue_ipclient, G_TYPE_OBJECT)

static void
gclue_ipclient_finalize (GObject *gipclient)
{
        GClueIpclient *ipclient = (GClueIpclient *) gipclient;

        g_clear_object (&ipclient->priv->soup_session);
        g_free (ipclient->priv->ip);

        G_OBJECT_CLASS (gclue_ipclient_parent_class)->finalize (gipclient);
}

static void
gclue_ipclient_class_init (GClueIpclientClass *klass)
{
        GObjectClass *gipclient_class = G_OBJECT_CLASS (klass);

        gipclient_class->finalize = gclue_ipclient_finalize;

        g_type_class_add_private (klass, sizeof (GClueIpclientPrivate));
}

static void
gclue_ipclient_init (GClueIpclient *ipclient)
{
        ipclient->priv = G_TYPE_INSTANCE_GET_PRIVATE ((ipclient), GCLUE_TYPE_IPCLIENT, GClueIpclientPrivate);

        ipclient->priv->soup_session = soup_session_new ();
}

/**
 * gclue_ipclient_new_for_ip:
 * @str: The IP address
 *
 * Creates a new #GClueIpclient to fetch the geolocation data
 * Use gclue_ipclient_search_async() to query the server
 *
 * Returns: a new #GClueIpclient. Use g_object_unref() when done.
 **/
GClueIpclient *
gclue_ipclient_new_for_ip (const char *ip)
{
        GClueIpclient *ipclient;

        ipclient = g_object_new (GCLUE_TYPE_IPCLIENT, NULL);
        ipclient->priv->ip = g_strdup (ip);

        return ipclient;
}

/**
 * gclue_ipclient_new:
 *
 * Creates a new #GClueIpclient to fetch the geolocation data.
 * Here the IP address is not provided the by client, hence the server
 * will try to get the IP address from various proxy variables.
 * Use gclue_ipclient_search_async() to query the server
 *
 * Returns: a new #GClueIpclient. Use g_object_unref() when done.
 **/
GClueIpclient *
gclue_ipclient_new (void)
{
        return gclue_ipclient_new_for_ip (NULL);
}

static SoupMessage *
get_search_query (GClueIpclient *ipclient)
{
        SoupMessage *ret;
        char *uri;

        if (ipclient->priv->ip) {
                GHashTable *ht;
                char *query_string;

                ht = g_hash_table_new (g_str_hash, g_str_equal);
                g_hash_table_insert (ht, "ip", g_strdup (ipclient->priv->ip));
                query_string = soup_form_encode_hash (ht);
                g_hash_table_destroy (ht);

                uri = g_strdup_printf ("%s?%s", GEOIP_SERVER, query_string);
                g_free (query_string);
        } else
                uri = g_strdup (GEOIP_SERVER);

        ret = soup_message_new ("GET", uri);
        g_free (uri);

        return ret;
}

typedef struct
{
        GSimpleAsyncResult *simple;
        SoupMessage *message;
        GCancellable *cancellable;

        gulong cancelled_id;
} QueryCallbackData;

static void
query_callback_data_free (QueryCallbackData *data)
{
        g_object_unref (data->simple);
        g_clear_object (&data->cancellable);
        g_slice_free (QueryCallbackData, data);
}

static void
search_cancelled_callback (GCancellable      *cancellable,
                           QueryCallbackData *data)
{
        GClueIpclient *ipclient = GCLUE_IPCLIENT
                (g_async_result_get_source_object
                        (G_ASYNC_RESULT (data->simple)));
        g_debug ("Cancelling query");
        soup_session_cancel_message (ipclient->priv->soup_session,
                                     data->message,
                                     SOUP_STATUS_CANCELLED);
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        QueryCallbackData *data = (QueryCallbackData *) user_data;
        GSimpleAsyncResult *simple = data->simple;
        GError *error = NULL;
        char *contents;

        if (data->cancellable != NULL)
                g_signal_handler_disconnect (data->cancellable,
                                             data->cancelled_id);

        if (query->status_code != SOUP_STATUS_OK) {
                GIOErrorEnum code;

                if (query->status_code == SOUP_STATUS_CANCELLED)
                        code = G_IO_ERROR_CANCELLED;
                else
                        code = G_IO_ERROR_FAILED;
		g_set_error_literal (&error, G_IO_ERROR, code,
                                     query->reason_phrase ? query->reason_phrase : "Query failed");
                g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete (simple);
		query_callback_data_free (data);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
        g_simple_async_result_set_op_res_gpointer (simple, contents, NULL);

        g_simple_async_result_complete (simple);
        query_callback_data_free (data);
}

/**
 * gclue_ipclient_search_async:
 * @ipclient: a #GClueIpclient representing a query
 * @cancellable: optional #GCancellable forward, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously performs a query to get the geolocation information
 * from the server. Use gclue_ipclient_search() to do the same
 * thing synchronously.
 *
 * When the operation is finished, @callback will be called. You can then call
 * gclue_ipclient_search_finish() to get the result of the operation.
 **/
void
gclue_ipclient_search_async (GClueIpclient      *ipclient,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
        QueryCallbackData *data;

        g_return_if_fail (GCLUE_IS_IPCLIENT (ipclient));

        data = g_slice_new0 (QueryCallbackData);
        data->simple = g_simple_async_result_new (g_object_ref (ipclient),
                                                  callback,
                                                  user_data,
                                                  gclue_ipclient_search_async);
        if (cancellable != NULL)
                data->cancellable = g_object_ref (cancellable);
        data->message = get_search_query (ipclient);

        if (cancellable != NULL)
                data->cancelled_id = g_signal_connect
                        (cancellable,
                         "cancelled",
                         G_CALLBACK (search_cancelled_callback),
                         data);
        soup_session_queue_message (ipclient->priv->soup_session,
                                    data->message,
                                    query_callback,
                                    data);
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        GeoipServerError server_error_code;
        int error_code;
        const char *error_message;

        if (!json_object_has_member (object, "error_code"))
            return FALSE;

        server_error_code = json_object_get_int_member (object, "error_code");
        switch (server_error_code) {
                case INVALID_IP_ADDRESS_ERR:
                        error_code = GCLUE_ERROR_INVALID_ARGUMENTS;
                        break;
                case INVALID_ENTRY_ERR:
                        error_code = GCLUE_ERROR_NO_MATCHES;
                        break;
                case DATABASE_ERR:
                        error_code = GCLUE_ERROR_INTERNAL_SERVER;
                        break;
                default:
                        g_assert_not_reached ();
        }

        error_message = json_object_get_string_member (object, "error_message");

        g_set_error_literal (error,
                             GCLUE_ERROR,
                             error_code,
                             error_message);

        return TRUE;
}

static gdouble
get_accuracy_from_string (const char *str)
{
        if (strcmp (str, "street") == 0)
                return GEOCODE_LOCATION_ACCURACY_STREET;
        else if (strcmp (str, "city") == 0)
                return GEOCODE_LOCATION_ACCURACY_CITY;
        else if (strcmp (str, "region") == 0)
                return GEOCODE_LOCATION_ACCURACY_REGION;
        else if (strcmp (str, "country") == 0)
                return GEOCODE_LOCATION_ACCURACY_COUNTRY;
        else if (strcmp (str, "continent") == 0)
                return GEOCODE_LOCATION_ACCURACY_CONTINENT;
        else
                return GEOCODE_LOCATION_ACCURACY_UNKNOWN;
}

static gdouble
get_accuracy_from_json_location (JsonObject *object)
{
        if (json_object_has_member (object, "accuracy")) {
                const char *str;

                str = json_object_get_string_member (object, "accuracy");
                return get_accuracy_from_string (str);
        } else if (json_object_has_member (object, "street")) {
                return GEOCODE_LOCATION_ACCURACY_STREET;
        } else if (json_object_has_member (object, "city")) {
                return GEOCODE_LOCATION_ACCURACY_CITY;
        } else if (json_object_has_member (object, "region_name")) {
                return GEOCODE_LOCATION_ACCURACY_REGION;
        } else if (json_object_has_member (object, "country_name")) {
                return GEOCODE_LOCATION_ACCURACY_COUNTRY;
        } else if (json_object_has_member (object, "continent")) {
                return GEOCODE_LOCATION_ACCURACY_CONTINENT;
        } else {
                return GEOCODE_LOCATION_ACCURACY_UNKNOWN;
        }
}

static GeocodeLocation *
_gclue_ip_json_to_location (const char *json,
                            GError    **error)
{
        JsonParser *parser;
        JsonNode *node;
        JsonObject *object;
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy;
        char *desc = NULL;

        parser = json_parser_new ();

        if (!json_parser_load_from_data (parser, json, -1, error))
                return NULL;

        node = json_parser_get_root (parser);
        object = json_node_get_object (node);

        if (parse_server_error (object, error))
                return NULL;

        latitude = json_object_get_double_member (object, "latitude");
        longitude = json_object_get_double_member (object, "longitude");
        accuracy = get_accuracy_from_json_location (object);

        location = geocode_location_new (latitude, longitude, accuracy);

        if (json_object_has_member (object, "country_name")) {
                if (json_object_has_member (object, "region_name")) {
                        if (json_object_has_member (object, "city")) {
                                desc = g_strdup_printf ("%s, %s, %s",
                                                        json_object_get_string_member (object, "city"),
                                                        json_object_get_string_member (object, "region_name"),
                                                        json_object_get_string_member (object, "country_name"));
                        } else {
                                desc = g_strdup_printf ("%s, %s",
                                                        json_object_get_string_member (object, "region_name"),
                                                        json_object_get_string_member (object, "country_name"));
                        }
                } else {
                        desc = g_strdup_printf ("%s",
                                                json_object_get_string_member (object, "country_name"));
                }
        }

        if (desc != NULL) {
                geocode_location_set_description (location, desc);
                g_free (desc);
        }

        g_object_unref (parser);

        return location;
}

/**
 * gclue_ipclient_search_finish:
 * @ipclient: a #GClueIpclient representing a query
 * @res: a #GAsyncResult
 * @error: a #GError
 *
 * Finishes a geolocation search operation. See gclue_ipclient_search_async().
 *
 * Returns: (transfer full): A #GeocodeLocation object or %NULL in case of
 * errors. Free the returned object with g_object_unref() when done.
 **/
GeocodeLocation *
gclue_ipclient_search_finish (GClueIpclient *ipclient,
                              GAsyncResult  *res,
                              GError       **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
        char *contents = NULL;
        GeocodeLocation *location;

        g_return_val_if_fail (GCLUE_IS_IPCLIENT (ipclient), NULL);

        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gclue_ipclient_search_async);

        if (g_simple_async_result_propagate_error (simple, error))
                return NULL;

        contents = g_simple_async_result_get_op_res_gpointer (simple);
        location = _gclue_ip_json_to_location (contents, error);
        g_free (contents);
        g_object_unref (ipclient);

        return location;
}

/**
 * gclue_ipclient_search:
 * @ipclient: a #GClueIpclient representing a query
 * @error: a #GError
 *
 * Gets the geolocation data for an IP address from the server.
 *
 * Returns: (transfer full): A #GeocodeLocation object or %NULL in case of
 * errors. Free the returned object with g_object_unref() when done.
 **/
GeocodeLocation *
gclue_ipclient_search (GClueIpclient *ipclient,
                       GError       **error)
{
        char *contents;
        SoupMessage *query;
        GeocodeLocation *location;

        g_return_val_if_fail (GCLUE_IS_IPCLIENT (ipclient), NULL);

        query = get_search_query (ipclient);

        if (soup_session_send_message (ipclient->priv->soup_session,
                                       query) != SOUP_STATUS_OK) {
                g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     query->reason_phrase ? query->reason_phrase : "Query failed");
                g_object_unref (query);
                return NULL;
        }

        contents = g_strndup (query->response_body->data, query->response_body->length);
        g_object_unref (query);

        location = _gclue_ip_json_to_location (contents, error);
        g_free (contents);

        return location;
}
