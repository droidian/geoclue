/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2014 Red Hat, Inc.
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
#include <nm-client.h>
#include <nm-device-wifi.h>
#include "gclue-wifi.h"
#include "gclue-config.h"
#include "gclue-error.h"
#include "geocode-glib/geocode-location.h"

/**
 * SECTION:gclue-wifi
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-wifi.h
 *
 * Contains functions to get the geolocation based on nearby WiFi networks. It
 * uses <ulink url="https://wiki.mozilla.org/CloudServices/Location">Mozilla
 * Location Service</ulink> to achieve that. The URL is kept in our
 * configuration file so its easy to switch to Google's API.
 **/

struct _GClueWifiPrivate {
        NMClient *client;
        NMDeviceWifi *wifi_device;

        gulong ap_added_id;

        guint refresh_timeout;
};

static SoupMessage *
gclue_wifi_create_query (GClueWebSource *source,
                         GError        **error);
static SoupMessage *
gclue_wifi_create_submit_query (GClueWebSource  *source,
                                GeocodeLocation *location,
                                GError         **error);
static GeocodeLocation *
gclue_wifi_parse_response (GClueWebSource *source,
                           const char     *json,
                           GError        **error);

G_DEFINE_TYPE (GClueWifi, gclue_wifi, GCLUE_TYPE_WEB_SOURCE)

static void
on_device_removed (NMClient *client,
                   NMDevice *device,
                   gpointer  user_data);

static void
gclue_wifi_finalize (GObject *gwifi)
{
        GClueWifi *wifi = (GClueWifi *) gwifi;

        if (wifi->priv->wifi_device != NULL)
                on_device_removed (wifi->priv->client,
                                   NM_DEVICE (wifi->priv->wifi_device),
                                   wifi);
        g_object_unref (wifi->priv->client);

        G_OBJECT_CLASS (gclue_wifi_parent_class)->finalize (gwifi);
}

static void
gclue_wifi_class_init (GClueWifiClass *klass)
{
        GClueWebSourceClass *source_class = GCLUE_WEB_SOURCE_CLASS (klass);
        GObjectClass *gwifi_class = G_OBJECT_CLASS (klass);

        source_class->create_query = gclue_wifi_create_query;
        source_class->create_submit_query = gclue_wifi_create_submit_query;
        source_class->parse_response = gclue_wifi_parse_response;
        gwifi_class->finalize = gclue_wifi_finalize;

        g_type_class_add_private (klass, sizeof (GClueWifiPrivate));
}

static gboolean
on_refresh_timeout (gpointer user_data)
{
        g_debug ("Refreshing location..");
        gclue_web_source_refresh (GCLUE_WEB_SOURCE (user_data));
        GCLUE_WIFI (user_data)->priv->refresh_timeout = 0;

        return FALSE;
}

static void
on_ap_added (NMDeviceWifi  *device,
             NMAccessPoint *ap,
             gpointer       user_data);

static void
on_ap_strength_notify (GObject    *gobject,
                       GParamSpec *pspec,
                       gpointer    user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        NMAccessPoint *ap = NM_ACCESS_POINT (gobject);

        if (nm_access_point_get_strength (ap) <= 20) {
                g_debug ("WiFi AP '%s' still has very low strength (%u)"
                         ", ignoring again..",
                         nm_access_point_get_bssid (ap),
                         nm_access_point_get_strength (ap));
                return;
        }

        g_signal_handlers_disconnect_by_func (G_OBJECT (ap),
                                              on_ap_strength_notify,
                                              user_data);
        on_ap_added (wifi->priv->wifi_device, ap, user_data);
}

static gboolean
should_ignore_ap (NMAccessPoint *ap)
{
        const GByteArray *ssid;

        ssid = nm_access_point_get_ssid (ap);
        if (ssid == NULL ||
            (ssid->len >= 6 &&
             ssid->data[ssid->len - 1] == 'p' &&
             ssid->data[ssid->len - 2] == 'a' &&
             ssid->data[ssid->len - 3] == 'm' &&
             ssid->data[ssid->len - 4] == 'o' &&
             ssid->data[ssid->len - 5] == 'n' &&
             ssid->data[ssid->len - 6] == '_')) {
                g_debug ("SSID for WiFi AP '%s' missing or has '_nomap' suffix."
                         ", Ignoring..",
                         nm_access_point_get_bssid (ap));
                return TRUE;
        }

        return FALSE;
}

static void
on_ap_added (NMDeviceWifi  *device,
             NMAccessPoint *ap,
             gpointer       user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);

        if (wifi->priv->refresh_timeout != 0)
                return;
        g_debug ("WiFi AP '%s' added.", nm_access_point_get_bssid (ap));

        if (should_ignore_ap (ap))
                return;

        if (nm_access_point_get_strength (ap) <= 20) {
                g_debug ("WiFi AP '%s' has very low strength (%u)"
                         ", ignoring for now..",
                         nm_access_point_get_bssid (ap),
                         nm_access_point_get_strength (ap));
                g_signal_connect (G_OBJECT (ap),
                                  "notify::strength",
                                  G_CALLBACK (on_ap_strength_notify),
                                  user_data);
                return;
        }

        /* There could be multiple devices being added/removed at the same time
         * so we don't immediately call refresh but rather wait 1 second.
         */
        wifi->priv->refresh_timeout = g_timeout_add_seconds (1,
                                                             on_refresh_timeout,
                                                             wifi);
}

static void
connect_ap_signals (GClueWifi  *wifi)
{
        if (wifi->priv->ap_added_id != 0)
                return;

        wifi->priv->ap_added_id =
                g_signal_connect (wifi->priv->wifi_device,
                                  "access-point-added",
                                  G_CALLBACK (on_ap_added),
                                  wifi);
}

static void
disconnect_ap_signals (GClueWifi  *wifi)
{
        GClueWifiPrivate *priv = wifi->priv;

        if (priv->ap_added_id == 0)
                return;

        g_signal_handler_disconnect (priv->wifi_device, priv->ap_added_id);
        priv->ap_added_id = 0;

        if (priv->refresh_timeout != 0) {
                g_source_remove (priv->refresh_timeout);
                priv->refresh_timeout = 0;
        }
}

static void
on_device_added (NMClient *client,
                 NMDevice *device,
                 gpointer  user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);

        if (wifi->priv->wifi_device != NULL || !NM_IS_DEVICE_WIFI (device))
                return;

        wifi->priv->wifi_device = NM_DEVICE_WIFI (device);
        g_debug ("WiFi device '%s' added.",
                 nm_device_wifi_get_hw_address (wifi->priv->wifi_device));

        connect_ap_signals (wifi);
}

static void
on_device_removed (NMClient *client,
                   NMDevice *device,
                   gpointer  user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);

        if (wifi->priv->wifi_device == NULL ||
            NM_DEVICE (wifi->priv->wifi_device) != device)
                return;
        g_debug ("WiFi device '%s' removed.",
                 nm_device_wifi_get_hw_address (wifi->priv->wifi_device));

        disconnect_ap_signals (wifi);
        wifi->priv->wifi_device = NULL;
}

static void
gclue_wifi_init (GClueWifi *wifi)
{
        const GPtrArray *devices;
        guint i;

        wifi->priv = G_TYPE_INSTANCE_GET_PRIVATE ((wifi), GCLUE_TYPE_WIFI, GClueWifiPrivate);

        wifi->priv->client = nm_client_new (); /* FIXME: We should be using async variant */
        g_signal_connect (wifi->priv->client,
                          "device-added",
                          G_CALLBACK (on_device_added),
                          wifi);
        g_signal_connect (wifi->priv->client,
                          "device-removed",
                          G_CALLBACK (on_device_removed),
                          wifi);

        devices = nm_client_get_devices (wifi->priv->client);
        for (i = 0; i < devices->len; i++) {
                NMDevice *device = g_ptr_array_index (devices, i);
                if (NM_IS_DEVICE_WIFI (device)) {
                    on_device_added (wifi->priv->client, device, wifi);

                    break;
                }
        }
}

static void
on_wifi_destroyed (gpointer data,
                   GObject *where_the_object_was)
{
        GClueWifi **wifi = (GClueWifi **) data;

        *wifi = NULL;
}

/**
 * gclue_wifi_new:
 *
 * Get the #GClueWifi singleton.
 *
 * Returns: (transfer full): a new ref to #GClueWifi. Use g_object_unref()
 * when done.
 **/
GClueWifi *
gclue_wifi_get_singleton (void)
{
        static GClueWifi *wifi = NULL;

        if (wifi == NULL) {
                wifi = g_object_new (GCLUE_TYPE_WIFI, NULL);
                g_object_weak_ref (G_OBJECT (wifi),
                                   on_wifi_destroyed,
                                   &wifi);
        } else
                g_object_ref (wifi);

        return wifi;
}

static char *
get_url (void)
{
        GClueConfig *config;

        config = gclue_config_get_singleton ();

        return gclue_config_get_wifi_url (config);
}

static const GPtrArray *
get_ap_list (GClueWifi *wifi,
             GError   **error)
{
        const GPtrArray *aps;

        if (wifi->priv->wifi_device == NULL) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_FAILED,
                                     "No WiFi devices available");
                return NULL;
        }

        aps = nm_device_wifi_get_access_points (wifi->priv->wifi_device);
        if (aps == NULL || aps->len == 0) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_FAILED,
                                     "No WiFi access points around");
                return NULL;
        }

        return aps;
}

static SoupMessage *
gclue_wifi_create_query (GClueWebSource *source,
                         GError        **error)
{
        GClueWifi *wifi = GCLUE_WIFI (source);
        SoupMessage *ret = NULL;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data;
        gsize data_len;
        const GPtrArray *aps; /* As in Access Points */
        guint i;
        char *uri;

        aps = get_ap_list (wifi, error);
        if (aps == NULL)
                goto out;

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        json_builder_set_member_name (builder, "wifiAccessPoints");
        json_builder_begin_array (builder);

        for (i = 0; i < aps->len; i++) {
                NMAccessPoint *ap = g_ptr_array_index (aps, i);
                const char *mac;
                gint8 strength_dbm;

                if (should_ignore_ap (ap))
                        continue;

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "macAddress");
                mac = nm_access_point_get_bssid (ap);
                json_builder_add_string_value (builder, mac);

                json_builder_set_member_name (builder, "signalStrength");
                strength_dbm = nm_access_point_get_strength (ap) / 2 - 100;
                json_builder_add_int_value (builder, strength_dbm);
                json_builder_end_object (builder);
        }

        json_builder_end_array (builder);
        json_builder_end_object (builder);

        generator = json_generator_new ();
        root_node = json_builder_get_root (builder);
        json_generator_set_root (generator, root_node);
        data = json_generator_to_data (generator, &data_len);

        json_node_free (root_node);
        g_object_unref (builder);
        g_object_unref (generator);

        uri = get_url ();
        ret = soup_message_new ("POST", uri);
        soup_message_set_request (ret,
                                  "application/json",
                                  SOUP_MEMORY_TAKE,
                                  data,
                                  data_len);
        g_debug ("Sending following request to '%s':\n%s", uri, data);

        g_free (uri);
out:
        return ret;
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        JsonObject *error_obj;
        int code;
        const char *message;

        if (!json_object_has_member (object, "error"))
            return FALSE;

        error_obj = json_object_get_object_member (object, "error");
        code = json_object_get_int_member (error_obj, "code");
        message = json_object_get_string_member (error_obj, "message");

        g_set_error_literal (error, G_IO_ERROR, code, message);

        return TRUE;
}

static GeocodeLocation *
gclue_wifi_parse_response (GClueWebSource *source,
                           const char     *json,
                           GError        **error)
{
        JsonParser *parser;
        JsonNode *node;
        JsonObject *object, *loc_object;
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy;

        parser = json_parser_new ();

        if (!json_parser_load_from_data (parser, json, -1, error))
                return NULL;

        node = json_parser_get_root (parser);
        object = json_node_get_object (node);

        if (parse_server_error (object, error))
                return NULL;

        loc_object = json_object_get_object_member (object, "location");
        latitude = json_object_get_double_member (loc_object, "lat");
        longitude = json_object_get_double_member (loc_object, "lng");

        accuracy = json_object_get_double_member (object, "accuracy");

        location = geocode_location_new (latitude, longitude, accuracy);

        g_object_unref (parser);

        return location;
}

static char *
get_submit_url (void)
{
        GClueConfig *config;

        config = gclue_config_get_singleton ();

        return gclue_config_get_wifi_submit_url (config);
}

static SoupMessage *
gclue_wifi_create_submit_query (GClueWebSource  *source,
                                GeocodeLocation *location,
                                GError         **error)
{
        GClueWifi *wifi = GCLUE_WIFI (source);
        SoupMessage *ret = NULL;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data, *timestamp, *url;
        gsize data_len;
        const GPtrArray *aps; /* As in Access Points */
        guint i, frequency;
        gdouble lat, lon, accuracy, altitude;
        GTimeVal tv;

        url = get_submit_url ();
        if (url == NULL)
                goto out;

        aps = get_ap_list (wifi, error);
        if (aps == NULL)
                goto out;

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        json_builder_set_member_name (builder, "items");
        json_builder_begin_array (builder);

        json_builder_begin_object (builder);

        lat = geocode_location_get_latitude (location);
        json_builder_set_member_name (builder, "lat");
        json_builder_add_double_value (builder, lat);

        lon = geocode_location_get_longitude (location);
        json_builder_set_member_name (builder, "lon");
        json_builder_add_double_value (builder, lon);

        accuracy = geocode_location_get_accuracy (location);
        if (accuracy != GEOCODE_LOCATION_ACCURACY_UNKNOWN) {
                json_builder_set_member_name (builder, "accuracy");
                json_builder_add_double_value (builder, accuracy);
        }

        altitude = geocode_location_get_altitude (location);
        if (altitude != GEOCODE_LOCATION_ALTITUDE_UNKNOWN) {
                json_builder_set_member_name (builder, "altitude");
                json_builder_add_double_value (builder, altitude);
        }

        tv.tv_sec = geocode_location_get_timestamp (location);
        tv.tv_usec = 0;
        timestamp = g_time_val_to_iso8601 (&tv);
        json_builder_set_member_name (builder, "time");
        json_builder_add_string_value (builder, timestamp);
        g_free (timestamp);

        json_builder_set_member_name (builder, "wifi");
        json_builder_begin_array (builder);

        for (i = 0; i < aps->len; i++) {
                NMAccessPoint *ap = g_ptr_array_index (aps, i);
                const char *mac;
                gint8 strength_dbm;

                if (should_ignore_ap (ap))
                        continue;

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "key");
                mac = nm_access_point_get_bssid (ap);
                json_builder_add_string_value (builder, mac);

                json_builder_set_member_name (builder, "signal");
                strength_dbm = nm_access_point_get_strength (ap) / 2 - 100;
                json_builder_add_int_value (builder, strength_dbm);

                json_builder_set_member_name (builder, "frequency");
                frequency = nm_access_point_get_frequency (ap);
                json_builder_add_int_value (builder, frequency);
                json_builder_end_object (builder);
        }

        json_builder_end_array (builder); /* wifi */
        json_builder_end_object (builder);
        json_builder_end_array (builder); /* items */
        json_builder_end_object (builder);

        generator = json_generator_new ();
        root_node = json_builder_get_root (builder);
        json_generator_set_root (generator, root_node);
        data = json_generator_to_data (generator, &data_len);

        json_node_free (root_node);
        g_object_unref (builder);
        g_object_unref (generator);

        ret = soup_message_new ("POST", url);
        soup_message_set_request (ret,
                                  "application/json",
                                  SOUP_MEMORY_TAKE,
                                  data,
                                  data_len);
        g_debug ("Sending following request to '%s':\n%s", url, data);
        g_free (url);

out:
        return ret;
}
