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
#include <string.h>
#include "gclue-3g.h"
#include "gclue-3g-tower.h"
#include "gclue-config.h"
#include "gclue-modem-manager.h"
#include "gclue-location.h"
#include "gclue-mozilla.h"
#include "gclue-wifi.h"

/**
 * SECTION:gclue-3g
 * @short_description: 3GPP-based geolocation
 *
 * Contains functions to get the geolocation based on 3GPP cell towers.
 **/

/* Should be slightly less than MAX_LOCATION_AGE in gclue-locator.c, so we don't
 * get replaced by a less accurate WiFi location while still connected to a tower.
 * Technically, this can only happen on the NEIGHBORHOOD accuracy level (since at
 * this level WiFi does scrambling), but it won't hurt on higher ones, too.
 * In seconds.
 */
#define LOCATION_3GPP_TIMEOUT (25 * 60)

static unsigned int gclue_3g_running;

struct _GClue3GPrivate {
        GClueMozilla *mozilla;
        GClueModem *modem;

        GCancellable *cancellable;

        gulong threeg_notify_id;
        guint location_3gpp_timeout_id;
};

G_DEFINE_TYPE_WITH_CODE (GClue3G,
                         gclue_3g,
                         GCLUE_TYPE_WEB_SOURCE,
                         G_ADD_PRIVATE (GClue3G))

static GClueLocationSourceStartResult
gclue_3g_start (GClueLocationSource *source);
static GClueLocationSourceStopResult
gclue_3g_stop (GClueLocationSource *source);
static SoupMessage *
gclue_3g_create_query (GClueWebSource *web,
                       const char **query_data_description,
                       GError        **error);
static SoupMessage *
gclue_3g_create_submit_query (GClueWebSource  *web,
                              GClueLocation   *location,
                              GError         **error);
static GClueAccuracyLevel
gclue_3g_get_available_accuracy_level (GClueWebSource *web,
                                       gboolean available);

static void
on_3g_enabled (GObject      *source_object,
               GAsyncResult *result,
               gpointer      user_data)
{
        g_autoptr(GError) error = NULL;

        if (!gclue_modem_enable_3g_finish (GCLUE_MODEM (source_object),
                                           result,
                                           &error)) {
                if (error && !g_error_matches (error, G_IO_ERROR,
                                               G_IO_ERROR_CANCELLED)) {
                        g_warning ("Failed to enable 3GPP: %s", error->message);
                }
        }
}

static void
on_is_3g_available_notify (GObject    *gobject,
                           GParamSpec *pspec,
                           gpointer    user_data)
{
        GClue3G *source = GCLUE_3G (user_data);
        GClue3GPrivate *priv = source->priv;
        gboolean available_3g;

        available_3g = gclue_modem_get_is_3g_available (priv->modem);
        g_debug ("3G available notify %d", (int)available_3g);

        gclue_web_source_refresh (GCLUE_WEB_SOURCE (source));

        if (gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (source)) &&
            available_3g)
                gclue_modem_enable_3g (priv->modem,
                                       priv->cancellable,
                                       on_3g_enabled,
                                       source);
}

static void cancel_location_3gpp_timeout (GClue3G *g3g)
{
        GClue3GPrivate *priv = g3g->priv;

        if (!priv->location_3gpp_timeout_id)
                return;

        g_source_remove (priv->location_3gpp_timeout_id);
        priv->location_3gpp_timeout_id = 0;
}

static GClueLocation *
gclue_3g_parse_response (GClueWebSource *source,
                         const char     *content,
                         GError        **error)
{
        const char *location_description =
                gclue_web_source_get_query_data_description (source);

        return gclue_mozilla_parse_response (content, location_description, error);
}

static gboolean
gclue_3g_parse_submit_response (GClueWebSource *source,
                                const char     *content,
                                gint            status_code,
                                GError        **error)
{
        return gclue_mozilla_parse_submit_response (content, status_code, error);
}

static void
gclue_3g_finalize (GObject *g3g)
{
        GClue3G *source = (GClue3G *) g3g;
        GClue3GPrivate *priv = source->priv;

        G_OBJECT_CLASS (gclue_3g_parent_class)->finalize (g3g);

        g_cancellable_cancel (priv->cancellable);

        g_signal_handler_disconnect (priv->modem,
                                     priv->threeg_notify_id);
        priv->threeg_notify_id = 0;

        cancel_location_3gpp_timeout (source);

        g_clear_object (&priv->modem);
        g_clear_object (&priv->mozilla);
        g_clear_object (&priv->cancellable);
}

static void
gclue_3g_class_init (GClue3GClass *klass)
{
        GClueWebSourceClass *web_class = GCLUE_WEB_SOURCE_CLASS (klass);
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *g3g_class = G_OBJECT_CLASS (klass);

        g3g_class->finalize = gclue_3g_finalize;

        source_class->start = gclue_3g_start;
        source_class->stop = gclue_3g_stop;
        web_class->create_query = gclue_3g_create_query;
        web_class->parse_response = gclue_3g_parse_response;
        web_class->create_submit_query = gclue_3g_create_submit_query;
        web_class->parse_submit_response = gclue_3g_parse_submit_response;
        web_class->get_available_accuracy_level =
                gclue_3g_get_available_accuracy_level;
}

static void
gclue_3g_init (GClue3G *source)
{
        GClue3GPrivate *priv;
        GClueWebSource *web_source = GCLUE_WEB_SOURCE (source);
        GClueConfig *config = gclue_config_get_singleton ();

        source->priv = gclue_3g_get_instance_private (source);
        priv = source->priv;

        priv->cancellable = g_cancellable_new ();

        priv->mozilla = gclue_mozilla_get_singleton ();
        gclue_web_source_set_locate_url (web_source,
                                         gclue_config_get_wifi_url (config));
        gclue_web_source_set_submit_url (web_source,
                                         gclue_config_get_wifi_submit_url (config));

        priv->modem = gclue_modem_manager_get_singleton ();
        priv->threeg_notify_id =
                        g_signal_connect (priv->modem,
                                          "notify::is-3g-available",
                                          G_CALLBACK (on_is_3g_available_notify),
                                          source);
        priv->location_3gpp_timeout_id = 0;
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
 * Get the #GClue3G singleton, for the specified max accuracy level @level.
 *
 * Returns: (transfer full): a new ref to #GClue3G. Use g_object_unref()
 * when done.
 **/
GClue3G *
gclue_3g_get_singleton (GClueAccuracyLevel level)
{
        static GClue3G *source[] = { NULL, NULL };
        int i;

        g_return_val_if_fail (level >= GCLUE_ACCURACY_LEVEL_CITY, NULL);

        i = gclue_wifi_should_skip_bsss (level) ? 0 : 1;
        if (source[i] == NULL) {
                source[i] = g_object_new (GCLUE_TYPE_3G,
                                          "accuracy-level", level,
                                          "compute-movement", FALSE,
                                          NULL);
                g_object_weak_ref (G_OBJECT (source[i]),
                                   on_3g_destroyed,
                                   &source[i]);
        } else
                g_object_ref (source[i]);

        return source[i];
}

static gboolean
g3g_should_skip_bsss (GClue3G *g3g)
{
        GClueAccuracyLevel level;

        g_object_get (G_OBJECT (g3g), "accuracy-level", &level, NULL);
        return gclue_wifi_should_skip_bsss (level);
}

static SoupMessage *
gclue_3g_create_query (GClueWebSource *web,
                       const char **query_data_description,
                       GError        **error)
{
        GClue3G *g3g = GCLUE_3G (web);
        GClue3GPrivate *priv = g3g->priv;
        gboolean skip_bss;
        const char *url;

        if (!gclue_mozilla_has_tower (priv->mozilla)) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_NOT_INITIALIZED,
                                     "3GPP cell tower info unavailable");
                return NULL; /* Not initialized yet */
        }

        skip_bss = g3g_should_skip_bsss (g3g);
        if (skip_bss) {
                g_debug ("Will skip BSSs in query due to our accuracy level");
        }

        url = gclue_web_source_get_locate_url (web);
        return gclue_mozilla_create_query (priv->mozilla, url, FALSE, skip_bss,
                                           query_data_description, error);
}

static SoupMessage *
gclue_3g_create_submit_query (GClueWebSource  *web,
                              GClueLocation   *location,
                              GError         **error)
{
        GClue3GPrivate *priv = GCLUE_3G (web)->priv;
        const char *submit_url;
        GClueConfig *config = gclue_config_get_singleton ();

        if (!gclue_config_get_wifi_submit_data (config))
                return NULL;

        if (!gclue_mozilla_has_tower (priv->mozilla)) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_NOT_INITIALIZED,
                                     "3GPP cell tower info unavailable");
                return NULL; /* Not initialized yet */
        }

        submit_url = gclue_web_source_get_submit_url (web);
        return gclue_mozilla_create_submit_query (priv->mozilla,
                                                  submit_url,
                                                  location,
                                                  error);
}

static GClueAccuracyLevel
gclue_3g_get_available_accuracy_level (GClueWebSource *web,
                                       gboolean        network_available)
{
        if (gclue_modem_get_is_3g_available (GCLUE_3G (web)->priv->modem) &&
            network_available)
                return GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD;
        else
                return GCLUE_ACCURACY_LEVEL_NONE;
}

gboolean gclue_3g_should_skip_tower (GClueAccuracyLevel level)
{
        return level < GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD;
}

static gboolean
on_location_3gpp_timeout (gpointer user_data)
{
        GClue3G *g3g = GCLUE_3G (user_data);
        GClue3GPrivate *priv = g3g->priv;

        if (!gclue_mozilla_has_tower (priv->mozilla)) {
                g_debug ("3GPP location timeout, but no tower");
                priv->location_3gpp_timeout_id = 0;
                return G_SOURCE_REMOVE;
        }

        g_debug ("3GPP location timeout, re-sending existing location");
        gclue_web_source_refresh (GCLUE_WEB_SOURCE (g3g));

        return G_SOURCE_CONTINUE;
}

static void set_location_3gpp_timeout (GClue3G *g3g)
{
        GClue3GPrivate *priv = g3g->priv;

        g_debug ("Scheduling new 3GPP location timeout");

        cancel_location_3gpp_timeout (g3g);
        priv->location_3gpp_timeout_id = g_timeout_add_seconds (LOCATION_3GPP_TIMEOUT,
                                                                on_location_3gpp_timeout,
                                                                g3g);
}

static void
on_fix_3g (GClueModem   *modem,
           const gchar  *opc,
           gulong        lac,
           gulong        cell_id,
           GClueTowerTec tec,
           gpointer    user_data)
{
        GClue3G *g3g = GCLUE_3G (user_data);
        GClue3GPrivate *priv = g3g->priv;

        g_debug ("3GPP %s fix available",
                 tec == GCLUE_TOWER_TEC_NO_FIX ? "no" : "new");

        if (tec != GCLUE_TOWER_TEC_NO_FIX) {
                GClue3GTower tower;

                g_strlcpy (tower.opc, opc,
                           GCLUE_3G_TOWER_OPERATOR_CODE_STR_LEN + 1);
                tower.lac = lac;
                tower.cell_id = cell_id;
                tower.tec = tec;
                set_location_3gpp_timeout (g3g);
                gclue_mozilla_set_tower (priv->mozilla, &tower);
        } else {
                cancel_location_3gpp_timeout (g3g);
                gclue_mozilla_set_tower (priv->mozilla, NULL);
        }

        gclue_web_source_refresh (GCLUE_WEB_SOURCE (user_data));
}

static GClueLocationSourceStartResult
gclue_3g_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClue3GPrivate *priv;
        GClueLocationSourceStartResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source),
                              GCLUE_LOCATION_SOURCE_START_RESULT_FAILED);
        priv = GCLUE_3G (source)->priv;

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_3g_parent_class);
        base_result = base_class->start (source);
        if (base_result != GCLUE_LOCATION_SOURCE_START_RESULT_OK)
                return base_result;

        if (gclue_3g_running == 0) {
                g_debug ("First 3GPP source starting up");
        }
        gclue_3g_running++;

        g_signal_connect (priv->modem,
                          "fix-3g",
                          G_CALLBACK (on_fix_3g),
                          source);

        /* Emits fix-3g signal even if the location hasn't actually changed to prime us */
        if (gclue_modem_get_is_3g_available (priv->modem))
                gclue_modem_enable_3g (priv->modem,
                                       priv->cancellable,
                                       on_3g_enabled,
                                       source);
        return base_result;
}

static GClueLocationSourceStopResult
gclue_3g_stop (GClueLocationSource *source)
{
        GClue3G *g3g = GCLUE_3G (source);
        GClue3GPrivate *priv = g3g->priv;
        GClueLocationSourceClass *base_class;
        g_autoptr(GError) error = NULL;
        GClueLocationSourceStopResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source),
                              GCLUE_LOCATION_SOURCE_STOP_RESULT_FAILED);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_3g_parent_class);
        base_result = base_class->stop (source);
        if (base_result != GCLUE_LOCATION_SOURCE_STOP_RESULT_OK)
                return base_result;

        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->modem),
                                              G_CALLBACK (on_fix_3g),
                                              source);

        cancel_location_3gpp_timeout (g3g);

        g_assert (gclue_3g_running > 0);
        gclue_3g_running--;
        if (gclue_3g_running > 0) {
                return base_result;
        }

        g_debug ("Last 3GPP source stopping, disabling location gathering and invalidating existing tower");

        if (gclue_modem_get_is_3g_available (priv->modem))
                if (!gclue_modem_disable_3g (priv->modem,
                                             priv->cancellable,
                                             &error)) {
                        g_warning ("Failed to disable 3GPP: %s",
                                   error->message);
                }

        gclue_mozilla_set_tower (priv->mozilla, NULL);

        return base_result;
}
