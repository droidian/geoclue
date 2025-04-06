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
#include <json-glib/json-glib.h>
#include <string.h>
#include <config.h>
#include "gclue-mozilla.h"
#include "gclue-3g-tower.h"
#include "gclue-config.h"
#include "gclue-error.h"
#include "gclue-wifi.h"

/**
 * SECTION:gclue-mozilla
 * @short_description: Helpers to create queries for and parse response of,
 * Mozilla Location Service.
 *
 * Contains API to get the geolocation based on IP address, nearby WiFi networks
 * and 3GPP cell tower info. It uses
 * <ulink url="https://wiki.mozilla.org/CloudServices/Location">Mozilla Location
 * Service</ulink> to achieve that. The URL is kept in our configuration file so
 * its easy to switch to Google's API.
 **/

struct _GClueMozillaPrivate
{
        GClueWifi *wifi;

        GClue3GTower tower;
        gboolean tower_valid;
        gboolean tower_submitted;

        gboolean bss_submitted;
};

G_DEFINE_TYPE_WITH_CODE (GClueMozilla,
                         gclue_mozilla,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GClueMozilla))

#define BSSID_LEN 6
#define BSSID_STR_LEN 17
#define MAX_SSID_LEN 32

static guint
variant_to_string (GVariant *variant, guint max_len, char *ret)
{
        guint i;
        guint len;

        len = g_variant_n_children (variant);
        if (len == 0)
                return 0;
        g_return_val_if_fail(len <= max_len, 0);
        ret[len] = '\0';

        for (i = 0; i < len; i++)
                g_variant_get_child (variant,
                                     i,
                                     "y",
                                     &ret[i]);

        return len;
}

static guint
get_ssid_from_bss (WPABSS *bss, char *ssid)
{
        GVariant *variant = wpa_bss_get_ssid (bss);
        if (variant == NULL)
                return 0;

        return variant_to_string (variant, MAX_SSID_LEN, ssid);
}

static gboolean
get_bssid_from_bss (WPABSS *bss, char *bssid)
{
        GVariant *variant;
        char raw_bssid[BSSID_LEN + 1] = { 0 };
        guint raw_len, i;

        variant = wpa_bss_get_bssid (bss);
        if (variant == NULL)
                return FALSE;

        raw_len = variant_to_string (variant, BSSID_LEN, raw_bssid);
        g_return_val_if_fail (raw_len == BSSID_LEN, FALSE);

        for (i = 0; i < BSSID_LEN; i++) {
                unsigned char c = (unsigned char) raw_bssid[i];

                if (i == BSSID_LEN - 1) {
                        g_snprintf (bssid + (i * 3), 3, "%02x", c);
                } else {
                        g_snprintf (bssid + (i * 3), 4, "%02x:", c);
                }
        }

        return TRUE;
}

const char *
gclue_mozilla_get_locate_url (GClueMozilla *mozilla)
{
        GClueConfig *config = gclue_config_get_singleton ();

        return gclue_config_get_wifi_url (config);
}

static gboolean
operator_code_to_mcc_mnc (const gchar *opc,
                          gint64      *mcc_p,
                          gint64      *mnc_p)
{
        gchar *end;
        gchar mcc_str[GCLUE_3G_TOWER_COUNTRY_CODE_STR_LEN + 1] = { 0 };

        g_strlcpy (mcc_str, opc, GCLUE_3G_TOWER_COUNTRY_CODE_STR_LEN + 1);
        *mcc_p = g_ascii_strtoll (mcc_str, &end, 10);
        if (*end != '\0')
                goto error;

        *mnc_p = g_ascii_strtoll (opc + GCLUE_3G_TOWER_COUNTRY_CODE_STR_LEN,
                                  &end, 10);
        if (*end != '\0')
                goto error;

        return TRUE;
error:
        g_warning ("Operator code conversion failed");
        return FALSE;
}

static gboolean
towertec_to_radiotype (GClueTowerTec tec,
                       const char **radiotype_p)
{
        switch (tec) {
        case GCLUE_TOWER_TEC_2G:
            *radiotype_p = "gsm";
            break;
        case GCLUE_TOWER_TEC_3G:
            *radiotype_p = "wcdma";
            break;
        case GCLUE_TOWER_TEC_4G:
            *radiotype_p = "lte";
            break;
        default:
            *radiotype_p = NULL;
            return FALSE;
        }

        return TRUE;
}

#define USER_AGENT (PACKAGE_NAME "/" PACKAGE_VERSION)

SoupMessage *
gclue_mozilla_create_query (GClueMozilla  *mozilla,
                            gboolean skip_tower,
                            gboolean skip_bss,
                            const char **query_data_description,
                            GError      **error)
{
        gboolean has_tower = FALSE, has_bss = FALSE;
        SoupMessage *ret = NULL;
        SoupMessageHeaders *request_headers;
        JsonBuilder *builder;
        g_autoptr(GList) bss_list = NULL;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data;
        gsize data_len;
        const char *uri, *radiotype;
        guint n_non_ignored_bsss;
        GList *iter;
        gint64 mcc, mnc;
        g_autoptr(GBytes) body = NULL;

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        if (mozilla->priv->wifi && !skip_bss) {
                bss_list = gclue_wifi_get_bss_list (mozilla->priv->wifi);
        }
        /* We send pure geoip query using empty object if both bss_list and
         * tower are NULL.
         *
         * If the list of non-ignored BSSs is <2, don’t bother submitting the
         * BSS list as MLS will only do a geoip lookup anyway.
         * See https://ichnaea.readthedocs.io/en/latest/api/geolocate.html#field-definition
         */
        n_non_ignored_bsss = 0;
        for (iter = bss_list; iter != NULL; iter = iter->next) {
                WPABSS *bss = WPA_BSS (iter->data);

                if (gclue_mozilla_should_ignore_bss (bss))
                        continue;

                n_non_ignored_bsss++;
        }

        if (mozilla->priv->tower_valid && !skip_tower &&
            towertec_to_radiotype (mozilla->priv->tower.tec, &radiotype) &&
            operator_code_to_mcc_mnc (mozilla->priv->tower.opc, &mcc, &mnc)) {
                json_builder_set_member_name (builder, "radioType");
                json_builder_add_string_value (builder, radiotype);

                json_builder_set_member_name (builder, "cellTowers");
                json_builder_begin_array (builder);

                json_builder_begin_object (builder);

                json_builder_set_member_name (builder, "cellId");
                json_builder_add_int_value (builder, mozilla->priv->tower.cell_id);
                json_builder_set_member_name (builder, "mobileCountryCode");
                json_builder_add_int_value (builder, mcc);
                json_builder_set_member_name (builder, "mobileNetworkCode");
                json_builder_add_int_value (builder, mnc);
                json_builder_set_member_name (builder, "locationAreaCode");
                json_builder_add_int_value (builder, mozilla->priv->tower.lac);
                json_builder_set_member_name (builder, "radioType");
                json_builder_add_string_value (builder, radiotype);

                json_builder_end_object (builder);

                json_builder_end_array (builder);

                has_tower = TRUE;
        }

        if (n_non_ignored_bsss >= 2) {
                json_builder_set_member_name (builder, "wifiAccessPoints");
                json_builder_begin_array (builder);

                for (iter = bss_list; iter != NULL; iter = iter->next) {
                        WPABSS *bss = WPA_BSS (iter->data);
                        char mac[BSSID_STR_LEN + 1] = { 0 };
                        gint16 strength_dbm;
                        guint age_ms;

                        if (gclue_mozilla_should_ignore_bss (bss))
                                continue;

                        json_builder_begin_object (builder);

                        json_builder_set_member_name (builder, "macAddress");
                        get_bssid_from_bss (bss, mac);
                        json_builder_add_string_value (builder, mac);

                        json_builder_set_member_name (builder, "signalStrength");
                        strength_dbm = wpa_bss_get_signal (bss);
                        json_builder_add_int_value (builder, strength_dbm);

                        json_builder_set_member_name (builder, "age");
                        age_ms = 1000 * wpa_bss_get_age (bss);
                        json_builder_add_int_value (builder, age_ms);

                        json_builder_end_object (builder);
                        has_bss = TRUE;
                }
                json_builder_end_array (builder);
        }
        json_builder_end_object (builder);

        generator = json_generator_new ();
        root_node = json_builder_get_root (builder);
        json_generator_set_root (generator, root_node);
        data = json_generator_to_data (generator, &data_len);

        json_node_free (root_node);
        g_object_unref (builder);
        g_object_unref (generator);

        uri = gclue_mozilla_get_locate_url (mozilla);
        ret = soup_message_new ("POST", uri);
        request_headers = soup_message_get_request_headers (ret);
        soup_message_headers_append (request_headers, "User-Agent", USER_AGENT);
        body = g_bytes_new_take (data, data_len);
        soup_message_set_request_body_from_bytes (ret, "application/json", body);
        g_debug ("Sending following request to '%s':\n%s", uri, data);

        if (query_data_description) {
                if (has_tower && has_bss) {
                        *query_data_description = "3GPP + WiFi";
                } else if (has_tower) {
                        *query_data_description = "3GPP";
                } else if (has_bss) {
                        *query_data_description = "WiFi";
                } else {
                        *query_data_description = "GeoIP";
                }
        }

        return ret;
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        JsonObject *error_obj;
        const char *message;

        if (!json_object_has_member (object, "error"))
            return FALSE;

        error_obj = json_object_get_object_member (object, "error");
        if (json_object_has_member (error_obj, "message")) {
                message = json_object_get_string_member (error_obj, "message");
        } else {
                message = "Unknown error";
        }

        g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, message);

        return TRUE;
}

GClueLocation *
gclue_mozilla_parse_response (const char *json,
                              const char *location_description,
                              GError    **error)
{
        g_autoptr(JsonParser) parser = NULL;
        JsonNode *node;
        JsonObject *object, *loc_object;
        g_autofree char *desc_new = NULL;
        GClueLocation *location;
        gdouble latitude, longitude, accuracy;

        parser = json_parser_new ();

        if (!json_parser_load_from_data (parser, json, -1, error))
                return NULL;

        node = json_parser_get_root (parser);
        object = json_node_get_object (node);

        if (parse_server_error (object, error))
                return NULL;

        if (json_object_has_member (object, "fallback")) {
                const char *fallback;

                fallback = json_object_get_string_member (object, "fallback");
                if (fallback && strlen (fallback)) {
                        desc_new = g_strdup_printf ("%s fallback (from %s data)",
                                                    fallback, location_description);
                        location_description = desc_new;
                }
        }

        loc_object = json_object_get_object_member (object, "location");
        latitude = json_object_get_double_member (loc_object, "lat");
        longitude = json_object_get_double_member (loc_object, "lng");

        accuracy = json_object_get_double_member (object, "accuracy");

        location = gclue_location_new (latitude, longitude, accuracy,
                                       location_description);

        return location;
}

const char *
gclue_mozilla_get_submit_url (GClueMozilla *mozilla)
{
        GClueConfig *config = gclue_config_get_singleton ();

        if (gclue_config_get_wifi_submit_data (config))
                return gclue_config_get_wifi_submit_url (config);
        else
                return NULL;
}

SoupMessage *
gclue_mozilla_create_submit_query (GClueMozilla  *mozilla,
                                   GClueLocation   *location,
                                   GError         **error)
{
        SoupMessage *ret = NULL;
        SoupMessageHeaders *request_headers;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data;
        g_autoptr(GList) bss_list = NULL;
        const char *url, *nick, *radiotype;
        gsize data_len;
        GList *iter;
        gdouble lat, lon, accuracy, altitude, speed;
        guint64 time_ms;
        gint64 mcc, mnc;
        GClueConfig *config;
        g_autoptr(GBytes) body = NULL;

        if (mozilla->priv->bss_submitted &&
            (!mozilla->priv->tower_valid ||
             mozilla->priv->tower_submitted))
        {
                g_debug ("Already created submit req for this data (bss submitted %d; tower: valid %d submitted %d)",
                         (int)mozilla->priv->bss_submitted,
                         (int)mozilla->priv->tower_valid,
                         (int)mozilla->priv->tower_submitted);
                goto out;
        }


        url = gclue_mozilla_get_submit_url (mozilla);
        if (url == NULL)
                goto out;
        config = gclue_config_get_singleton ();
        nick = gclue_config_get_wifi_submit_nick (config);

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        json_builder_set_member_name (builder, "items");
        json_builder_begin_array (builder);

        json_builder_begin_object (builder);

        json_builder_set_member_name (builder, "timestamp");
        time_ms = 1000 * gclue_location_get_timestamp (location);
        json_builder_add_int_value (builder, time_ms);

        json_builder_set_member_name (builder, "position");
        json_builder_begin_object (builder);

        lat = gclue_location_get_latitude (location);
        json_builder_set_member_name (builder, "latitude");
        json_builder_add_double_value (builder, lat);

        lon = gclue_location_get_longitude (location);
        json_builder_set_member_name (builder, "longitude");
        json_builder_add_double_value (builder, lon);

        accuracy = gclue_location_get_accuracy (location);
        if (accuracy != GCLUE_LOCATION_ACCURACY_UNKNOWN) {
                json_builder_set_member_name (builder, "accuracy");
                json_builder_add_double_value (builder, accuracy);
        }

        altitude = gclue_location_get_altitude (location);
        if (altitude != GCLUE_LOCATION_ALTITUDE_UNKNOWN) {
                json_builder_set_member_name (builder, "altitude");
                json_builder_add_double_value (builder, altitude);
        }

        speed = gclue_location_get_speed (location);
        if (speed != GCLUE_LOCATION_SPEED_UNKNOWN) {
                json_builder_set_member_name (builder, "speed");
                json_builder_add_double_value (builder, speed);
        }

        json_builder_end_object (builder); /* position */

        if (mozilla->priv->wifi) {
                bss_list = gclue_wifi_get_bss_list (mozilla->priv->wifi);
        }
        if (bss_list != NULL) {
                json_builder_set_member_name (builder, "wifiAccessPoints");
                json_builder_begin_array (builder);

                for (iter = bss_list; iter != NULL; iter = iter->next) {
                        WPABSS *bss = WPA_BSS (iter->data);
                        char mac[BSSID_STR_LEN + 1] = { 0 };
                        gint16 strength_dbm;
                        guint16 frequency;
                        guint age_ms;

                        if (gclue_mozilla_should_ignore_bss (bss))
                                continue;

                        json_builder_begin_object (builder);

                        json_builder_set_member_name (builder, "macAddress");
                        get_bssid_from_bss (bss, mac);
                        json_builder_add_string_value (builder, mac);

                        json_builder_set_member_name (builder, "signalStrength");
                        strength_dbm = wpa_bss_get_signal (bss);
                        json_builder_add_int_value (builder, strength_dbm);

                        json_builder_set_member_name (builder, "frequency");
                        frequency = wpa_bss_get_frequency (bss);
                        json_builder_add_int_value (builder, frequency);

                        json_builder_set_member_name (builder, "age");
                        age_ms = 1000 * wpa_bss_get_age (bss);
                        json_builder_add_int_value (builder, age_ms);

                        json_builder_end_object (builder);
                }

                json_builder_end_array (builder); /* wifiAccessPoints */
        }

        if (mozilla->priv->tower_valid &&
            towertec_to_radiotype (mozilla->priv->tower.tec, &radiotype) &&
            operator_code_to_mcc_mnc (mozilla->priv->tower.opc, &mcc, &mnc)) {
                json_builder_set_member_name (builder, "cellTowers");
                json_builder_begin_array (builder);

                json_builder_begin_object (builder);

                json_builder_set_member_name (builder, "radioType");
                json_builder_add_string_value (builder, radiotype);
                json_builder_set_member_name (builder, "cellId");
                json_builder_add_int_value (builder, mozilla->priv->tower.cell_id);
                json_builder_set_member_name (builder, "mobileCountryCode");
                json_builder_add_int_value (builder, mcc);
                json_builder_set_member_name (builder, "mobileNetworkCode");
                json_builder_add_int_value (builder, mnc);
                json_builder_set_member_name (builder, "locationAreaCode");
                json_builder_add_int_value (builder, mozilla->priv->tower.lac);

                json_builder_end_object (builder);

                json_builder_end_array (builder); /* cellTowers */
        }

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
        request_headers = soup_message_get_request_headers (ret);
        soup_message_headers_append (request_headers, "User-Agent", USER_AGENT);
        if (nick != NULL && nick[0] != '\0')
                soup_message_headers_append (request_headers,
                                             "X-Nickname",
                                             nick);
        body = g_bytes_new_take (data, data_len);
        soup_message_set_request_body_from_bytes (ret, "application/json", body);
        g_debug ("Sending following request to '%s':\n%s", url, data);

        mozilla->priv->bss_submitted = TRUE;
        mozilla->priv->tower_submitted = TRUE;

out:
        return ret;
}

gboolean
gclue_mozilla_should_ignore_bss (WPABSS *bss)
{
        char ssid[MAX_SSID_LEN + 1] = { 0 };
        char bssid[BSSID_STR_LEN + 1] = { 0 };
        guint len;

        if (!get_bssid_from_bss (bss, bssid)) {
                g_debug ("Ignoring WiFi AP with unknown BSSID..");
                return TRUE;
        }

        len = get_ssid_from_bss (bss, ssid);
        if (len == 0 || g_str_has_suffix (ssid, "_nomap")) {
                g_debug ("SSID for WiFi AP '%s' missing or has '_nomap' suffix."
                         ", Ignoring..",
                         bssid);
                return TRUE;
        }

        return FALSE;
}

static void
gclue_mozilla_finalize (GObject *object)
{
        GClueMozilla *mozilla = GCLUE_MOZILLA (object);

        g_clear_weak_pointer (&mozilla->priv->wifi);

        G_OBJECT_CLASS (gclue_mozilla_parent_class)->finalize (object);
}

static void
gclue_mozilla_init (GClueMozilla *mozilla)
{
        mozilla->priv = gclue_mozilla_get_instance_private (mozilla);
        mozilla->priv->wifi = NULL;
        mozilla->priv->tower_valid = FALSE;
        mozilla->priv->bss_submitted = FALSE;
}

static void
gclue_mozilla_class_init (GClueMozillaClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gclue_mozilla_finalize;
}

GClueMozilla *
gclue_mozilla_get_singleton (void)
{
        static GClueMozilla *mozilla = NULL;

        if (!mozilla) {
                mozilla = g_object_new (GCLUE_TYPE_MOZILLA, NULL);
                g_object_add_weak_pointer (G_OBJECT (mozilla), (gpointer) &mozilla);
        } else
                g_object_ref (mozilla);

        return mozilla;
}

void
gclue_mozilla_set_wifi (GClueMozilla *mozilla,
                        GClueWifi *wifi)
{
        g_return_if_fail (GCLUE_IS_MOZILLA (mozilla));

        if (mozilla->priv->wifi == wifi)
                return;

        g_clear_weak_pointer (&mozilla->priv->wifi);

        if (!wifi) {
                return;
        }

        mozilla->priv->wifi = wifi;
        g_object_add_weak_pointer (G_OBJECT (mozilla->priv->wifi),
                                   (gpointer) &mozilla->priv->wifi);
}

gboolean
gclue_mozilla_test_set_wifi (GClueMozilla *mozilla,
                             GClueWifi *old, GClueWifi *new)
{
        if (mozilla->priv->wifi != old)
                return FALSE;

        gclue_mozilla_set_wifi (mozilla, new);
        return TRUE;
}

void
gclue_mozilla_set_bss_dirty (GClueMozilla *mozilla)
{
        g_return_if_fail (GCLUE_IS_MOZILLA (mozilla));

        mozilla->priv->bss_submitted = FALSE;
}

static gboolean gclue_mozilla_tower_identical (const GClue3GTower *t1,
                                               const GClue3GTower *t2)
{
        return g_strcmp0 (t1->opc, t2->opc) == 0 && t1->lac == t2->lac &&
                t1->cell_id == t2->cell_id && t1->tec == t2->tec;
}

void
gclue_mozilla_set_tower (GClueMozilla *mozilla,
                         const GClue3GTower *tower)
{
        g_return_if_fail (GCLUE_IS_MOZILLA (mozilla));

        if (!tower ||
            !(tower->tec > GCLUE_TOWER_TEC_UNKNOWN
              && tower->tec <= GCLUE_TOWER_TEC_MAX_VALID)) {
                mozilla->priv->tower_valid = FALSE;
                return;
        }

        if (mozilla->priv->tower_valid &&
            mozilla->priv->tower_submitted) {
                mozilla->priv->tower_submitted =
                        gclue_mozilla_tower_identical (&mozilla->priv->tower,
                                                       tower);
        } else
                mozilla->priv->tower_submitted = FALSE;

        mozilla->priv->tower = *tower;
        mozilla->priv->tower_valid = TRUE;
}

gboolean
gclue_mozilla_has_tower (GClueMozilla *mozilla)
{
        g_return_val_if_fail (GCLUE_IS_MOZILLA (mozilla), FALSE);

        return mozilla->priv->tower_valid;
}

GClue3GTower *
gclue_mozilla_get_tower (GClueMozilla *mozilla)
{
        g_return_val_if_fail (GCLUE_IS_MOZILLA (mozilla), NULL);

        if (!mozilla->priv->tower_valid)
                return NULL;

        return &mozilla->priv->tower;
}
