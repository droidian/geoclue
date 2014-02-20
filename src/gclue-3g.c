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
#include <string.h>
#include <libxml/tree.h>
#include "gclue-3g.h"
#include "geocode-glib/geocode-location.h"

#define URL "http://www.opencellid.org/cell/get?mcc=%u&mnc=%u" \
            "&lac=%lu&cellid=%lu&key=4f2d9207-1339-4718-a412-9683f1461de6"

/**
 * SECTION:gclue-3g
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-3g.h
 *
 * Contains functions to get the geolocation based on 3G cell towers. It
 * uses <ulink url="http://www.opencellid.org">OpenCellID</ulink> to translate
 * cell tower info to a location.
 **/

struct _GClue3GPrivate {
        MMLocation3gpp *location_3gpp;

        SoupSession *soup_session;
        SoupMessage *query;
        GCancellable *cancellable;

        gulong network_changed_id;
};

static MMModemLocationSource
gclue_3g_get_req_modem_location_caps (GClueModemSource *source,
                                      const char      **caps_name);
static void
gclue_3g_modem_location_changed (GClueModemSource *source,
                                 MMModemLocation  *modem_location);

G_DEFINE_TYPE (GClue3G, gclue_3g, GCLUE_TYPE_MODEM_SOURCE)

static void
cancel_pending_query (GClue3G *source)
{
        GClue3GPrivate *priv = source->priv;

        if (priv->query != NULL && priv->network_changed_id == 0) {
                g_debug ("Cancelling query");
                soup_session_cancel_message (priv->soup_session,
                                             priv->query,
                                             SOUP_STATUS_CANCELLED);
                priv->query = NULL;
        }

        if (priv->network_changed_id != 0) {
                g_signal_handler_disconnect (g_network_monitor_get_default (),
                                             priv->network_changed_id);
                priv->network_changed_id = 0;
        }
}

static void
gclue_3g_finalize (GObject *g3g)
{
        GClue3G *source = (GClue3G *) g3g;
        GClue3GPrivate *priv = source->priv;

        cancel_pending_query (source);

        g_clear_object (&priv->soup_session);
        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
        g_clear_object (&priv->location_3gpp);

        G_OBJECT_CLASS (gclue_3g_parent_class)->finalize (g3g);
}

static void
gclue_3g_class_init (GClue3GClass *klass)
{
        GClueModemSourceClass *source_class = GCLUE_MODEM_SOURCE_CLASS (klass);
        GObjectClass *g3g_class = G_OBJECT_CLASS (klass);

        source_class->get_req_modem_location_caps =
                gclue_3g_get_req_modem_location_caps;
        source_class->modem_location_changed = gclue_3g_modem_location_changed;
        g3g_class->finalize = gclue_3g_finalize;

        g_type_class_add_private (klass, sizeof (GClue3GPrivate));
}

static void
gclue_3g_init (GClue3G *source)
{
        source->priv = G_TYPE_INSTANCE_GET_PRIVATE ((source), GCLUE_TYPE_3G, GClue3GPrivate);

        source->priv->cancellable = g_cancellable_new ();

        source->priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);
}

static void
on_3g_destroyed (gpointer data,
                 GObject *where_the_object_was)
{
        GClue3G **source = (GClue3G **) data;

        *source = NULL;
}

/**
 * gclue_3g_new:
 *
 * Get the #GClue3G singleton.
 *
 * Returns: (transfer full): a new ref to #GClue3G. Use g_object_unref()
 * when done.
 **/
GClue3G *
gclue_3g_get_singleton (void)
{
        static GClue3G *source = NULL;

        if (source == NULL) {
                source = g_object_new (GCLUE_TYPE_3G, NULL);
                g_object_weak_ref (G_OBJECT (source),
                                   on_3g_destroyed,
                                   &source);
        } else
                g_object_ref (source);

        return source;
}

static SoupMessage *
create_query (GClue3G *source)
{
        GClue3GPrivate *priv = source->priv;
        SoupMessage *ret = NULL;
        char *uri;
        guint mcc, mnc;
        gulong lac, cell_id;

        mcc = mm_location_3gpp_get_mobile_country_code (priv->location_3gpp);
        mnc = mm_location_3gpp_get_mobile_network_code (priv->location_3gpp);
        lac = mm_location_3gpp_get_location_area_code (priv->location_3gpp);
        cell_id = mm_location_3gpp_get_cell_id (priv->location_3gpp);
        uri = g_strdup_printf (URL,
                               mcc,
                               mnc,
                               lac,
                               cell_id);
        ret = soup_message_new ("GET", uri);
        g_debug ("Will send request '%s'", uri);

        g_free (uri);

        return ret;
}

static GeocodeLocation *
parse_response (GClue3G    *source,
                const char *xml)
{
        xmlDocPtr doc;
        xmlNodePtr node;
        xmlChar *prop;
        GeocodeLocation *location;
        gdouble latitude, longitude;

        doc = xmlParseDoc ((const xmlChar *) xml);
        if (doc == NULL) {
                g_warning ("Failed to parse following response:\n%s",  xml);
                return NULL;
        }
        node = xmlDocGetRootElement (doc);
        if (node == NULL || strcmp ((const char *) node->name, "rsp") != 0) {
                g_warning ("Expected 'rsp' root node in response:\n%s",  xml);
                return NULL;
        }

        /* Find 'cell' node */
        for (node = node->children; node != NULL; node = node->next) {
                if (strcmp ((const char *) node->name, "cell") == 0)
                        break;
        }
        if (node == NULL) {
                g_warning ("Failed to find node 'cell' in response:\n%s",  xml);
                return NULL;
        }

        /* Get latitude and longitude from 'cell' node */
        prop = xmlGetProp (node, (xmlChar *) "lat");
        if (prop == NULL) {
                g_warning ("No 'lat' property node on 'cell' in response:\n%s",
                           xml);
                return NULL;
        }
        latitude = g_ascii_strtod ((const char *) prop, NULL);
        xmlFree (prop);

        prop = xmlGetProp (node, (xmlChar *) "lon");
        if (prop == NULL) {
                g_warning ("No 'lon' property node on 'cell' in response:\n%s",
                           xml);
                return NULL;
        }
        longitude = g_ascii_strtod ((const char *) prop, NULL);
        xmlFree (prop);

        /* FIXME: Is 3km a good average coverage area for a cell tower */
        location = geocode_location_new (latitude, longitude, 3000);

        xmlFreeDoc (doc);

        return location;
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GClue3G *source;
        char *contents;
        GeocodeLocation *location;
        SoupURI *uri;

        if (query->status_code == SOUP_STATUS_CANCELLED)
                return;

        source = GCLUE_3G (user_data);
        source->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_warning ("Failed to query location: %s", query->reason_phrase);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
        uri = soup_message_get_uri (query);
        g_debug ("Got following response from '%s':\n%s",
                 soup_uri_to_string (uri, FALSE),
                 contents);
        location = parse_response (source, contents);
        g_free (contents);
        if (location == NULL)
                return;

        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (source),
                                            location);
}

static void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClue3GPrivate *priv = GCLUE_3G (user_data)->priv;

        if (!available)
                return;

        g_return_if_fail (priv->query != NULL);
        g_return_if_fail (priv->network_changed_id != 0);

        g_signal_handler_disconnect (g_network_monitor_get_default (),
                                     priv->network_changed_id);
        priv->network_changed_id = 0;

        soup_session_queue_message (priv->soup_session,
                                    priv->query,
                                    query_callback,
                                    user_data);
}

static gboolean
is_location_3gpp_same (GClue3G        *source,
                       MMLocation3gpp *location_3gpp)
{
        GClue3GPrivate *priv = source->priv;
        guint mcc, mnc, new_mcc, new_mnc;
        gulong lac, cell_id, new_lac, new_cell_id;

        if (priv->location_3gpp == NULL)
                return FALSE;

        mcc = mm_location_3gpp_get_mobile_country_code (priv->location_3gpp);
        mnc = mm_location_3gpp_get_mobile_network_code (priv->location_3gpp);
        lac = mm_location_3gpp_get_location_area_code (priv->location_3gpp);
        cell_id = mm_location_3gpp_get_cell_id (priv->location_3gpp);

        new_mcc = mm_location_3gpp_get_mobile_country_code (location_3gpp);
        new_mnc = mm_location_3gpp_get_mobile_network_code (location_3gpp);
        new_lac = mm_location_3gpp_get_location_area_code (location_3gpp);
        new_cell_id = mm_location_3gpp_get_cell_id (location_3gpp);

        return (mcc == new_mcc &&
                mnc == new_mnc &&
                lac == new_lac &&
                cell_id == new_cell_id);
}

static void
on_get_3gpp_ready (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
        GClue3G *source = GCLUE_3G (user_data);
        GClue3GPrivate *priv = source->priv;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        MMLocation3gpp *location_3gpp;
        GNetworkMonitor *monitor;
        GError *error = NULL;

        location_3gpp = mm_modem_location_get_3gpp_finish (modem_location,
                                                           res,
                                                           &error);
        if (error != NULL) {
                g_warning ("Failed to get location from 3GPP: %s",
                           error->message);
                g_error_free (error);
                return;
        }

        if (location_3gpp == NULL) {
                g_debug ("No 3GPP");
                return;
        }

        if (is_location_3gpp_same (source, location_3gpp)) {
                g_debug ("New 3GPP location is same as last one");
                return;
        }
        g_clear_object (&priv->location_3gpp);
        priv->location_3gpp = location_3gpp;

        cancel_pending_query (source);

        priv->query = create_query (source);

        monitor = g_network_monitor_get_default ();
        if (!g_network_monitor_get_network_available (monitor)) {
                g_debug ("No network, will send request later");
                priv->network_changed_id =
                        g_signal_connect (monitor,
                                          "network-changed",
                                          G_CALLBACK (on_network_changed),
                                          user_data);
                return;
        }

        soup_session_queue_message (priv->soup_session,
                                    priv->query,
                                    query_callback,
                                    user_data);
}

static void
gclue_3g_modem_location_changed (GClueModemSource *source,
                                 MMModemLocation  *modem_location)
{
        mm_modem_location_get_3gpp (modem_location,
                                    GCLUE_3G (source)->priv->cancellable,
                                    on_get_3gpp_ready,
                                    source);
}

static MMModemLocationSource
gclue_3g_get_req_modem_location_caps (GClueModemSource *source,
                                      const char      **caps_name)

{
        if (caps_name != NULL)
                *caps_name = "3GPP";

        return MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI;
}
