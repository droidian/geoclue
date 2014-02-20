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
#include "geocode-glib/geocode-location.h"

/**
 * SECTION:gclue-modem-gps
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-modem-gps.h
 *
 * Contains functions to get the geolocation from a GPS modem.
 **/

struct _GClueModemGPSPrivate {
        MMLocationGpsRaw *gps_raw;

        GCancellable *cancellable;
};

static MMModemLocationSource
gclue_modem_gps_get_req_modem_location_caps (GClueModemSource *source,
                                             const char      **caps_name);
static void
gclue_modem_gps_modem_location_changed (GClueModemSource *source,
                                        MMModemLocation  *modem_location);

G_DEFINE_TYPE (GClueModemGPS, gclue_modem_gps, GCLUE_TYPE_MODEM_SOURCE)

static void
gclue_modem_gps_finalize (GObject *ggps)
{
        GClueModemGPSPrivate *priv = GCLUE_MODEM_GPS (ggps)->priv;

        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
        g_clear_object (&priv->gps_raw);

        G_OBJECT_CLASS (gclue_modem_gps_parent_class)->finalize (ggps);
}

static void
gclue_modem_gps_class_init (GClueModemGPSClass *klass)
{
        GClueModemSourceClass *source_class = GCLUE_MODEM_SOURCE_CLASS (klass);
        GObjectClass *ggps_class = G_OBJECT_CLASS (klass);

        source_class->get_req_modem_location_caps =
                gclue_modem_gps_get_req_modem_location_caps;
        source_class->modem_location_changed =
                gclue_modem_gps_modem_location_changed;
        ggps_class->finalize = gclue_modem_gps_finalize;

        g_type_class_add_private (klass, sizeof (GClueModemGPSPrivate));
}

static void
gclue_modem_gps_init (GClueModemGPS *source)
{
        source->priv = G_TYPE_INSTANCE_GET_PRIVATE ((source), GCLUE_TYPE_MODEM_GPS, GClueModemGPSPrivate);

        source->priv->cancellable = g_cancellable_new ();
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
get_accuracy_from_nmea (GClueModemGPS     *source,
                        MMLocationGpsNmea *location_nmea)
{
        const char *gga;
        gdouble hdop; /* Horizontal Dilution Of Precision */
        char **parts;

        gga = mm_location_gps_nmea_get_trace (location_nmea, "$GPGGA");
        if (gga == NULL) {
                g_warning ("Failed to find GGA sentence:\n");
                return 300;
        }

        parts = g_strsplit (gga, ",", -1);
        if (g_strv_length (parts) < 14 ) {
                g_warning ("Failed to parse NMEA GGA sentence:\n%s", gga);
                g_strfreev (parts);
                return 300;
        }

        hdop = g_ascii_strtod (parts[8], NULL);
        g_strfreev (parts);

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

static void
on_get_gps_nmea_ready (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
        GClueModemGPS *source = GCLUE_MODEM_GPS (user_data);
        GClueModemGPSPrivate *priv = source->priv;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        MMLocationGpsNmea *location_nmea;
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy;
        GError *error = NULL;

        location_nmea = mm_modem_location_get_gps_nmea_finish (modem_location,
                                                               res,
                                                               &error);
        if (error != NULL) {
                g_warning ("Failed to get location from NMEA information: %s",
                           error->message);
                g_error_free (error);
                return;
        }

        if (location_nmea == NULL) {
                g_debug ("No NMEA");
                return;
        }

        latitude = mm_location_gps_raw_get_latitude (priv->gps_raw);
        longitude = mm_location_gps_raw_get_longitude (priv->gps_raw);
        g_clear_object (&priv->gps_raw);

        accuracy = get_accuracy_from_nmea (source, location_nmea);
        g_object_unref (location_nmea);

        location = geocode_location_new (latitude, longitude, accuracy);
        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (source),
                                            location);
}

static void
on_get_gps_raw_ready (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
        GClueModemGPS *source = GCLUE_MODEM_GPS (user_data);
        GClueModemGPSPrivate *priv = source->priv;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        GError *error = NULL;

        priv->gps_raw = mm_modem_location_get_gps_raw_finish (modem_location,
                                                              res,
                                                              &error);
        if (error != NULL) {
                g_warning ("Failed to get location from 3GPP: %s",
                           error->message);
                g_error_free (error);
                return;
        }

        if (priv->gps_raw == NULL) {
                g_debug ("No GPS");
                return;
        }

        mm_modem_location_get_gps_nmea (modem_location,
                                        source->priv->cancellable,
                                        on_get_gps_nmea_ready,
                                        source);
}

static void
gclue_modem_gps_modem_location_changed (GClueModemSource *source,
                                        MMModemLocation  *modem_location)
{
        mm_modem_location_get_gps_raw (modem_location,
                                       GCLUE_MODEM_GPS (source)->priv->cancellable,
                                       on_get_gps_raw_ready,
                                       source);
}

static MMModemLocationSource
gclue_modem_gps_get_req_modem_location_caps (GClueModemSource *source,
                                             const char      **caps_name)
{
        if (caps_name != NULL)
                *caps_name = "GPS";

        return MM_MODEM_LOCATION_SOURCE_GPS_RAW |
               MM_MODEM_LOCATION_SOURCE_GPS_NMEA;
}
