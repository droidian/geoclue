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
#include <string.h>
#include "gclue-modem-gps.h"
#include "gclue-modem-manager.h"
#include "gclue-location.h"

/**
 * SECTION:gclue-modem-gps
 * @short_description: GPS modem-based geolocation source
 * @include: gclue-glib/gclue-modem-gps.h
 *
 * Contains functions to get the geolocation from a GPS modem.
 **/

struct _GClueModemGPSPrivate {
        GClueModem *modem;

        GCancellable *cancellable;

        gulong gps_notify_id;
};


G_DEFINE_TYPE_WITH_CODE (GClueModemGPS,
                         gclue_modem_gps,
                         GCLUE_TYPE_LOCATION_SOURCE,
                         G_ADD_PRIVATE (GClueModemGPS))

static GClueLocationSourceStartResult
gclue_modem_gps_start (GClueLocationSource *source);
static GClueLocationSourceStopResult
gclue_modem_gps_stop (GClueLocationSource *source);

static void
refresh_accuracy_level (GClueModemGPS *source)
{
        GClueAccuracyLevel new, existing;

        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (source));

        if (gclue_modem_get_is_gps_available (source->priv->modem))
                new = GCLUE_ACCURACY_LEVEL_EXACT;
        else
                new = GCLUE_ACCURACY_LEVEL_NONE;

        if (new != existing) {
                g_debug ("Available accuracy level from %s: %u",
                         G_OBJECT_TYPE_NAME (source), new);
                g_object_set (G_OBJECT (source),
                              "available-accuracy-level", new,
                              NULL);
        }
}

static void
on_gps_enabled (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
        GClueModemGPS *source = GCLUE_MODEM_GPS (user_data);
        GError *error = NULL;

        if (!gclue_modem_enable_gps_finish (source->priv->modem,
                                            result,
                                            &error)) {
                g_warning ("Failed to enable GPS: %s", error->message);
                g_error_free (error);
        }
}

static void
on_is_gps_available_notify (GObject    *gobject,
                            GParamSpec *pspec,
                            gpointer    user_data)
{
        GClueModemGPS *source = GCLUE_MODEM_GPS (user_data);
        GClueModemGPSPrivate *priv = source->priv;

        refresh_accuracy_level (source);

        if (gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (source)) &&
            gclue_modem_get_is_gps_available (priv->modem))
                gclue_modem_enable_gps (priv->modem,
                                       priv->cancellable,
                                       on_gps_enabled,
                                       source);
}

static void
on_time_threshold_changed (GObject    *gobject,
                           GParamSpec *pspec,
                           gpointer    user_data)
{
        GClueModemGPS *source = GCLUE_MODEM_GPS (user_data);
        guint threshold;

        threshold = gclue_min_uint_get_value (GCLUE_MIN_UINT (gobject));
        gclue_modem_set_time_threshold (source->priv->modem, threshold);
}

static void
gclue_modem_gps_finalize (GObject *ggps)
{
        GClueModemGPSPrivate *priv = GCLUE_MODEM_GPS (ggps)->priv;

        G_OBJECT_CLASS (gclue_modem_gps_parent_class)->finalize (ggps);

        g_signal_handler_disconnect (priv->modem,
                                     priv->gps_notify_id);
        priv->gps_notify_id = 0;

        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
        g_clear_object (&priv->modem);
}

static void
gclue_modem_gps_class_init (GClueModemGPSClass *klass)
{
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *ggps_class = G_OBJECT_CLASS (klass);

        ggps_class->finalize = gclue_modem_gps_finalize;

        source_class->start = gclue_modem_gps_start;
        source_class->stop = gclue_modem_gps_stop;
}

static void
gclue_modem_gps_init (GClueModemGPS *source)
{
        GClueModemGPSPrivate *priv;
        GClueMinUINT *threshold;

        source->priv = gclue_modem_gps_get_instance_private (source);
        priv = source->priv;

        priv->cancellable = g_cancellable_new ();

        priv->modem = gclue_modem_manager_get_singleton ();
        priv->gps_notify_id =
                        g_signal_connect (priv->modem,
                                          "notify::is-gps-available",
                                          G_CALLBACK (on_is_gps_available_notify),
                                          source);
        threshold = gclue_location_source_get_time_threshold
                        (GCLUE_LOCATION_SOURCE (source));
        g_signal_connect (threshold,
                          "notify::value",
                          G_CALLBACK (on_time_threshold_changed),
                          source);
}

static void
on_modem_gps_destroyed (gpointer data,
                 GObject *where_the_object_was)
{
        GClueModemGPS **source = (GClueModemGPS **) data;

        *source = NULL;
}

/**
 * gclue_modem_gps_get_singleton:
 *
 * Get the #GClueModemGPS singleton.
 *
 * Returns: (transfer full): a new ref to #GClueModemGPS. Use g_object_unref()
 * when done.
 **/
GClueModemGPS *
gclue_modem_gps_get_singleton (void)
{
        static GClueModemGPS *source = NULL;

        if (source == NULL) {
                source = g_object_new (GCLUE_TYPE_MODEM_GPS, NULL);
                g_object_weak_ref (G_OBJECT (source),
                                   on_modem_gps_destroyed,
                                   &source);
        } else
                g_object_ref (source);

        return source;
}

static void
on_fix_gps (GClueModem *modem,
            const char *nmeas[],
            gpointer    user_data)
{
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (user_data);
        GClueLocation *prev_location;
        g_autoptr(GClueLocation) location = NULL;
        GError *error = NULL;

        prev_location = gclue_location_source_get_location (source);
        location = gclue_location_create_from_nmeas (nmeas,
                                                     prev_location,
                                                     &error);

        if (error != NULL) {
            g_warning ("Error: %s", error->message);
            g_clear_error (&error);

            return;
        }

        gclue_location_source_set_location (source,
                                            location);
}

static GClueLocationSourceStartResult
gclue_modem_gps_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueModemGPSPrivate *priv;
        GClueLocationSourceStartResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source),
                              GCLUE_LOCATION_SOURCE_START_RESULT_FAILED);
        priv = GCLUE_MODEM_GPS (source)->priv;

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_modem_gps_parent_class);
        base_result = base_class->start (source);
        if (base_result != GCLUE_LOCATION_SOURCE_START_RESULT_OK)
                return base_result;

        g_signal_connect (priv->modem,
                          "fix-gps",
                          G_CALLBACK (on_fix_gps),
                          source);

        if (gclue_modem_get_is_gps_available (priv->modem))
                gclue_modem_enable_gps (priv->modem,
                                        priv->cancellable,
                                        on_gps_enabled,
                                        source);

        return base_result;
}

static GClueLocationSourceStopResult
gclue_modem_gps_stop (GClueLocationSource *source)
{
        GClueModemGPSPrivate *priv = GCLUE_MODEM_GPS (source)->priv;
        GClueLocationSourceClass *base_class;
        GError *error = NULL;
        GClueLocationSourceStopResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), FALSE);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_modem_gps_parent_class);
        base_result = base_class->stop (source);
        if (base_result == GCLUE_LOCATION_SOURCE_STOP_RESULT_STILL_USED)
                return base_result;

        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->modem),
                                              G_CALLBACK (on_fix_gps),
                                              source);

        if (gclue_modem_get_is_gps_available (priv->modem))
                if (!gclue_modem_disable_gps (priv->modem,
                                              priv->cancellable,
                                              &error)) {
                        g_warning ("Failed to disable GPS: %s",
                                   error->message);
                        g_error_free (error);
                }

        return base_result;
}
