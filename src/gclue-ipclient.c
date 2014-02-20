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

#define GEOIP_SERVER "https://geoip.fedoraproject.org/city"

/**
 * SECTION:gclue-ipclient
 * @short_description: GeoIP client
 * @include: gclue-glib/gclue-ipclient.h
 *
 * Contains functions to get the geolocation corresponding to IP addresses from a server.
 **/

struct _GClueIpclientPrivate {
        char *ip;
};

static SoupMessage *
gclue_ipclient_create_query (GClueWebSource *source,
                             GError        **error);
static GeocodeLocation *
gclue_ipclient_parse_response (GClueWebSource *source,
                               const char     *json,
                               GError        **error);

G_DEFINE_TYPE (GClueIpclient, gclue_ipclient, GCLUE_TYPE_WEB_SOURCE)

static void
gclue_ipclient_finalize (GObject *gipclient)
{
        GClueIpclient *ipclient = (GClueIpclient *) gipclient;

        g_free (ipclient->priv->ip);

        G_OBJECT_CLASS (gclue_ipclient_parent_class)->finalize (gipclient);
}

static void
gclue_ipclient_class_init (GClueIpclientClass *klass)
{
        GClueWebSourceClass *source_class = GCLUE_WEB_SOURCE_CLASS (klass);
        GObjectClass *gipclient_class = G_OBJECT_CLASS (klass);

        source_class->create_query = gclue_ipclient_create_query;
        source_class->parse_response = gclue_ipclient_parse_response;
        gipclient_class->finalize = gclue_ipclient_finalize;

        g_type_class_add_private (klass, sizeof (GClueIpclientPrivate));
}

static void
gclue_ipclient_init (GClueIpclient *ipclient)
{
        ipclient->priv = G_TYPE_INSTANCE_GET_PRIVATE ((ipclient), GCLUE_TYPE_IPCLIENT, GClueIpclientPrivate);
}

static void
on_ipclient_destroyed (gpointer data,
                       GObject *where_the_object_was)
{
        GClueIpclient **ipclient = (GClueIpclient **) data;

        *ipclient = NULL;
}

/**
 * gclue_ipclient_get_singleton:
 *
 * Get the #GClueIpclient singleton.
 *
 * Returns: (transfer full): a new ref to #GClueIpclient. Use g_object_unref()
 * when done.
 **/
GClueIpclient *
gclue_ipclient_get_singleton (void)
{
        static GClueIpclient *ipclient = NULL;

        if (ipclient == NULL) {
                ipclient = g_object_new (GCLUE_TYPE_IPCLIENT, NULL);
                g_object_weak_ref (G_OBJECT (ipclient),
                                   on_ipclient_destroyed,
                                   &ipclient);
        } else
                g_object_ref (ipclient);

        return ipclient;
}

static SoupMessage *
gclue_ipclient_create_query (GClueWebSource *source,
                             GError        **error)
{
        GClueIpclientPrivate *priv;
        SoupMessage *ret;
        char *uri;

        g_return_val_if_fail (GCLUE_IS_IPCLIENT (source), NULL);
        priv = GCLUE_IPCLIENT (source)->priv;

        if (priv->ip) {
                GHashTable *ht;
                char *query_string;

                ht = g_hash_table_new (g_str_hash, g_str_equal);
                g_hash_table_insert (ht, "ip", g_strdup (priv->ip));
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
gclue_ipclient_parse_response (GClueWebSource *source,
                               const char     *json,
                               GError        **error)
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
