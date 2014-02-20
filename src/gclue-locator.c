/* vim: set et ts=8 sw=8: */
/* gclue-locator.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
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

#include "config.h"

#include <glib/gi18n.h>

#include "gclue-locator.h"
#include "gclue-ipclient.h"
#include "public-api/gclue-enum-types.h"

#if GCLUE_USE_WIFI_SOURCE
#include "gclue-wifi.h"
#endif

#if GCLUE_USE_3G_SOURCE
#include "gclue-3g.h"
#endif

#if GCLUE_USE_MODEM_GPS_SOURCE
#include "gclue-modem-gps.h"
#endif

/* This class is like a master location source that hides all individual
 * location sources from rest of the code
 */

G_DEFINE_TYPE (GClueLocator, gclue_locator, GCLUE_TYPE_LOCATION_SOURCE)

struct _GClueLocatorPrivate
{
        GList *sources;

        GClueAccuracyLevel accuracy_level;
};

enum
{
        PROP_0,
        PROP_ACCURACY_LEVEL,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
on_location_changed (GObject    *gobject,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        GClueLocationSource *locator = GCLUE_LOCATION_SOURCE (user_data);
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (gobject);
        GeocodeLocation *location, *cur_location;

        cur_location = gclue_location_source_get_location (locator);
        location = gclue_location_source_get_location (source);

        g_debug ("New location available");

        if (cur_location != NULL &&
            geocode_location_get_distance_from (location, cur_location) <
            geocode_location_get_accuracy (location) &&
            geocode_location_get_accuracy (location) >
            geocode_location_get_accuracy (cur_location)) {
                /* We only take the new location if either the previous one
                 * lies outside its accuracy circle or its more or as
                 * accurate as previous one.
                 */
                g_debug ("Ignoring less accurate new location");
                return;
        }

        gclue_location_source_set_location (locator, location);
}

static void
gclue_locator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                g_value_set_enum (value, locator->priv->accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                locator->priv->accuracy_level = g_value_get_enum (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_finalize (GObject *gsource)
{
        GClueLocator *locator = GCLUE_LOCATOR (gsource);
        GClueLocatorPrivate *priv = locator->priv;

        g_list_free_full (priv->sources, g_object_unref);
        priv->sources = NULL;

        G_OBJECT_CLASS (gclue_locator_parent_class)->finalize (gsource);
}

static void
gclue_locator_constructed (GObject *object)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);
        GClueLocationSource *submit_source = NULL;
        GList *node;

        if (locator->priv->accuracy_level >= GCLUE_IPCLIENT_ACCURACY_LEVEL) {
                GClueIpclient *ipclient = gclue_ipclient_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        ipclient);
        }
#if GCLUE_USE_3G_SOURCE
        if (locator->priv->accuracy_level >= GCLUE_3G_ACCURACY_LEVEL) {
                GClue3G *source = gclue_3g_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        source);
        }
#endif
#if GCLUE_USE_WIFI_SOURCE
        if (locator->priv->accuracy_level >= GCLUE_WIFI_ACCURACY_LEVEL) {
                GClueWifi *wifi = gclue_wifi_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        wifi);
        }
#endif
#if GCLUE_USE_MODEM_GPS_SOURCE
        if (locator->priv->accuracy_level >= GCLUE_MODEM_GPS_ACCURACY_LEVEL) {
                GClueModemGPS *gps = gclue_modem_gps_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        gps);
                submit_source = GCLUE_LOCATION_SOURCE (gps);
        }
#endif

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                GClueLocationSource *src = GCLUE_LOCATION_SOURCE (node->data);
                GeocodeLocation *location;

                location = gclue_location_source_get_location (src);
                if (location != NULL)
                        gclue_location_source_set_location
                                (GCLUE_LOCATION_SOURCE (locator), location);

                g_signal_connect (G_OBJECT (src),
                                  "notify::location",
                                  G_CALLBACK (on_location_changed),
                                  locator);

                if (submit_source != NULL && GCLUE_IS_WEB_SOURCE (src))
                        gclue_web_source_set_submit_source
                                (GCLUE_WEB_SOURCE (src),
                                 submit_source);
        }
}

static void
gclue_locator_class_init (GClueLocatorClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->get_property = gclue_locator_get_property;
        object_class->set_property = gclue_locator_set_property;
        object_class->finalize = gclue_locator_finalize;
        object_class->constructed = gclue_locator_constructed;
        g_type_class_add_private (object_class, sizeof (GClueLocatorPrivate));

        gParamSpecs[PROP_ACCURACY_LEVEL] = g_param_spec_enum ("accuracy-level",
                                                              "AccuracyLevel",
                                                              "Accuracy level",
                                                              GCLUE_TYPE_ACCURACY_LEVEL,
                                                              GCLUE_ACCURACY_LEVEL_CITY,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_ACCURACY_LEVEL]);
}

static void
gclue_locator_init (GClueLocator *locator)
{
        locator->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (locator,
                                            GCLUE_TYPE_LOCATOR,
                                            GClueLocatorPrivate);
}

GClueLocator *
gclue_locator_new (GClueAccuracyLevel level)
{
        return g_object_new (GCLUE_TYPE_LOCATOR,
                             "accuracy-level", level,
                             NULL);
}

GClueAccuracyLevel
gclue_locator_get_accuracy_level (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator),
                              GCLUE_ACCURACY_LEVEL_COUNTRY);

        return locator->priv->accuracy_level;
}
