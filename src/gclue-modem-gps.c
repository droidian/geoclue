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
#include <string.h>
#include "gclue-modem-gps.h"
#include "gclue-modem-manager.h"
#include "geocode-glib/geocode-location.h"

/**
 * SECTION:gclue-modem-gps
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-modem-gps.h
 *
 * Contains functions to get the geolocation from a GPS modem.
 **/

struct _GClueModemGPSPrivate {
        GClueModem *modem;

        GCancellable *cancellable;

        gulong gps_notify_id;
};


G_DEFINE_TYPE (GClueModemGPS, gclue_modem_gps, GCLUE_TYPE_LOCATION_SOURCE)

static gboolean
gclue_modem_gps_start (GClueLocationSource *source);
static gboolean
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

        g_type_class_add_private (klass, sizeof (GClueModemGPSPrivate));
}

static void
gclue_modem_gps_init (GClueModemGPS *source)
{
        GClueModemGPSPrivate *priv;

        source->priv = G_TYPE_INSTANCE_GET_PRIVATE ((source), GCLUE_TYPE_MODEM_GPS, GClueModemGPSPrivate);
        priv = source->priv;

        priv->cancellable = g_cancellable_new ();

        priv->modem = gclue_modem_manager_get_singleton ();
        priv->gps_notify_id =
                        g_signal_connect (priv->modem,
                                          "notify::is-gps-available",
                                          G_CALLBACK (on_is_gps_available_notify),
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

static gdouble
get_accuracy_from_hdop (gdouble hdop)
{
        /* FIXME: These are really just rough estimates based on:
         *        http://en.wikipedia.org/wiki/Dilution_of_precision_%28GPS%29#Meaning_of_DOP_Values
         */
        if (hdop <= 1)
                return 0;
        else if (hdop <= 2)
                return 1;
        else if (hdop <= 5)
                return 3;
        else if (hdop <= 10)
                return 50;
        else if (hdop <= 20)
                return 100;
        else
                return 300;
}

#define INVALID_COORDINATE -G_MAXDOUBLE

static gdouble
parse_coordinate_string (const char *coordinate,
                         const char *direction)
{
        gdouble minutes, degrees, out;
        gchar *degrees_str;

        if (coordinate[0] == '\0' ||
            direction[0] == '\0' ||
            direction[0] == '\0')
                return INVALID_COORDINATE;

        if (direction[0] != 'N' &&
            direction[0] != 'S' &&
            direction[0] != 'E' &&
            direction[0] != 'W') {
                g_warning ("Unknown direction '%s' for coordinates, ignoring..",
                           direction);
                return INVALID_COORDINATE;
        }

        degrees_str = g_strndup (coordinate, 2);
        degrees = g_ascii_strtod (degrees_str, NULL);
        g_free (degrees_str);

        minutes = g_ascii_strtod (coordinate + 2, NULL);

        /* Include the minutes as part of the degrees */
        out = degrees + (minutes / 60.0);

        if (direction[0] == 'S' || direction[0] == 'W')
                out = 0 - out;

        return out;
}

static gdouble
parse_altitude_string (const char *altitude,
                       const char *unit)
{
        if (altitude[0] == '\0' || unit[0] == '\0')
                return GEOCODE_LOCATION_ALTITUDE_UNKNOWN;

        if (unit[0] != 'M') {
                g_warning ("Unknown unit '%s' for altitude, ignoring..",
                           unit);

                return GEOCODE_LOCATION_ALTITUDE_UNKNOWN;
        }

        return g_ascii_strtod (altitude, NULL);
}

static void
on_fix_gps (GClueModem *modem,
            const char *gga,
            gpointer    user_data)
{
        GClueModemGPS *source = GCLUE_MODEM_GPS (user_data);
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy, altitude;
        gdouble hdop; /* Horizontal Dilution Of Precision */
        char **parts;

        parts = g_strsplit (gga, ",", -1);
        if (g_strv_length (parts) < 14 ) {
                g_warning ("Failed to parse NMEA GGA sentence:\n%s", gga);

                goto out;
        }

        /* For sentax of GGA senentences:
         * http://www.gpsinformation.org/dale/nmea.htm#GGA
         */
        latitude = parse_coordinate_string (parts[2], parts[3]);
        longitude = parse_coordinate_string (parts[4], parts[5]);
        if (latitude == INVALID_COORDINATE || longitude == INVALID_COORDINATE)
                goto out;

        altitude = parse_altitude_string (parts[9], parts[10]);
        if (altitude == GEOCODE_LOCATION_ALTITUDE_UNKNOWN)
                goto out;

        hdop = g_ascii_strtod (parts[8], NULL);
        accuracy = get_accuracy_from_hdop (hdop);

        location = geocode_location_new (latitude, longitude, accuracy);
        if (altitude != GEOCODE_LOCATION_ALTITUDE_UNKNOWN)
                g_object_set (location, "altitude", altitude, NULL);
        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (source),
                                            location);
out:
        g_strfreev (parts);
}

static gboolean
gclue_modem_gps_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueModemGPSPrivate *priv;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), FALSE);
        priv = GCLUE_MODEM_GPS (source)->priv;

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_modem_gps_parent_class);
        if (!base_class->start (source))
                return FALSE;

        g_signal_connect (priv->modem,
                          "fix-gps",
                          G_CALLBACK (on_fix_gps),
                          source);

        if (gclue_modem_get_is_gps_available (priv->modem))
                gclue_modem_enable_gps (priv->modem,
                                        priv->cancellable,
                                        on_gps_enabled,
                                        source);

        return TRUE;
}

static gboolean
gclue_modem_gps_stop (GClueLocationSource *source)
{
        GClueModemGPSPrivate *priv = GCLUE_MODEM_GPS (source)->priv;
        GClueLocationSourceClass *base_class;
        GError *error = NULL;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), FALSE);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_modem_gps_parent_class);
        if (!base_class->stop (source))
                return FALSE;

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

        return TRUE;
}
