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

#include <glib.h>
#include "gclue-location-source.h"

/**
 * SECTION:gclue-location-source
 * @short_description: GeoIP client
 * @include: gclue-glib/gclue-location-source.h
 *
 * The interface all geolocation sources must implement.
 **/

static gboolean
start_source (GClueLocationSource *source);
static gboolean
stop_source (GClueLocationSource *source);

G_DEFINE_ABSTRACT_TYPE (GClueLocationSource, gclue_location_source, G_TYPE_OBJECT)

struct _GClueLocationSourcePrivate
{
        GeocodeLocation *location;

        guint active_counter;

        GClueAccuracyLevel avail_accuracy_level;
};

enum
{
        PROP_0,
        PROP_LOCATION,
        PROP_ACTIVE,
        PROP_AVAILABLE_ACCURACY_LEVEL,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_location_source_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (object);

        switch (prop_id) {
        case PROP_LOCATION:
                g_value_set_object (value, source->priv->location);
                break;

        case PROP_ACTIVE:
                g_value_set_boolean (value,
                                     gclue_location_source_get_active (source));
                break;

        case PROP_AVAILABLE_ACCURACY_LEVEL:
                g_value_set_enum (value, source->priv->avail_accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_location_source_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (object);

        switch (prop_id) {
        case PROP_LOCATION:
        {
                GeocodeLocation *location = g_value_get_object (value);

                gclue_location_source_set_location (source, location);
                break;
        }

        case PROP_AVAILABLE_ACCURACY_LEVEL:
                source->priv->avail_accuracy_level = g_value_get_enum (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_location_source_finalize (GObject *object)
{
        GClueLocationSourcePrivate *priv = GCLUE_LOCATION_SOURCE (object)->priv;

        gclue_location_source_stop (GCLUE_LOCATION_SOURCE (object));
        g_clear_object (&priv->location);

        G_OBJECT_CLASS (gclue_location_source_parent_class)->finalize (object);
}

static void
gclue_location_source_class_init (GClueLocationSourceClass *klass)
{
        GObjectClass *object_class;

        klass->start = start_source;
        klass->stop = stop_source;

        object_class = G_OBJECT_CLASS (klass);
        object_class->get_property = gclue_location_source_get_property;
        object_class->set_property = gclue_location_source_set_property;
        object_class->finalize = gclue_location_source_finalize;
        g_type_class_add_private (object_class, sizeof (GClueLocationSourcePrivate));

        gParamSpecs[PROP_LOCATION] = g_param_spec_object ("location",
                                                          "Location",
                                                          "Location",
                                                          GEOCODE_TYPE_LOCATION,
                                                          G_PARAM_READWRITE);
        g_object_class_install_property (object_class,
                                         PROP_LOCATION,
                                         gParamSpecs[PROP_LOCATION]);

        gParamSpecs[PROP_ACTIVE] = g_param_spec_boolean ("active",
                                                         "Active",
                                                         "Active",
                                                         FALSE,
                                                         G_PARAM_READABLE);
        g_object_class_install_property (object_class,
                                         PROP_ACTIVE,
                                         gParamSpecs[PROP_ACTIVE]);

        gParamSpecs[PROP_AVAILABLE_ACCURACY_LEVEL] =
                g_param_spec_enum ("available-accuracy-level",
                                   "AvailableAccuracyLevel",
                                   "Available accuracy level",
                                   GCLUE_TYPE_ACCURACY_LEVEL,
                                   0,
                                   G_PARAM_READWRITE);
        g_object_class_install_property (object_class,
                                         PROP_AVAILABLE_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_AVAILABLE_ACCURACY_LEVEL]);
}

static void
gclue_location_source_init (GClueLocationSource *source)
{
        source->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (source,
                                             GCLUE_TYPE_LOCATION_SOURCE,
                                             GClueLocationSourcePrivate);
}

static gboolean
start_source (GClueLocationSource *source)
{
        source->priv->active_counter++;
        if (source->priv->active_counter > 1) {
                g_debug ("%s already active, not starting.",
                         G_OBJECT_TYPE_NAME (source));
                return FALSE;
        }

        g_object_notify (G_OBJECT (source), "active");
        g_debug ("%s now active", G_OBJECT_TYPE_NAME (source));
        return TRUE;
}

static gboolean
stop_source (GClueLocationSource *source)
{
        if (source->priv->active_counter == 0) {
                g_debug ("%s already inactive, not stopping.",
                         G_OBJECT_TYPE_NAME (source));
                return FALSE;
        }

        source->priv->active_counter--;
        if (source->priv->active_counter > 0) {
                g_debug ("%s still in use, not stopping.",
                         G_OBJECT_TYPE_NAME (source));
                return FALSE;
        }

        g_object_notify (G_OBJECT (source), "active");
        g_debug ("%s now inactive", G_OBJECT_TYPE_NAME (source));

        return TRUE;
}

/**
 * gclue_location_source_start:
 * @source: a #GClueLocationSource
 *
 * Start searching for location and keep an eye on location changes.
 **/
void
gclue_location_source_start (GClueLocationSource *source)
{
        g_return_if_fail (GCLUE_IS_LOCATION_SOURCE (source));

        GCLUE_LOCATION_SOURCE_GET_CLASS (source)->start (source);
}

/**
 * gclue_location_source_stop:
 * @source: a #GClueLocationSource
 *
 * Stop searching for location and no need to keep an eye on location changes
 * anymore.
 **/
void
gclue_location_source_stop (GClueLocationSource *source)
{
        g_return_if_fail (GCLUE_IS_LOCATION_SOURCE (source));

        GCLUE_LOCATION_SOURCE_GET_CLASS (source)->stop (source);
}

/**
 * gclue_location_source_get_location:
 * @source: a #GClueLocationSource
 *
 * Returns: (transfer none): The location, or NULL if unknown.
 **/
GeocodeLocation *
gclue_location_source_get_location (GClueLocationSource *source)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), NULL);

        return source->priv->location;
}

/**
 * gclue_location_source_set_location:
 * @source: a #GClueLocationSource
 *
 * Set the current location to @location. Its meant to be only used by
 * subclasses.
 **/
void
gclue_location_source_set_location (GClueLocationSource *source,
                                    GeocodeLocation     *location)
{
        GClueLocationSourcePrivate *priv = source->priv;

        if (priv->location == NULL)
                priv->location = g_object_new (GEOCODE_TYPE_LOCATION, NULL);

        g_object_set (priv->location,
                      "latitude", geocode_location_get_latitude (location),
                      "longitude", geocode_location_get_longitude (location),
                      "accuracy", geocode_location_get_accuracy (location),
                      "description", geocode_location_get_description (location),
                      NULL);

        g_object_notify (G_OBJECT (source), "location");
}

/**
 * gclue_location_source_get_active:
 * @source: a #GClueLocationSource
 *
 * Returns: TRUE if source is active, FALSE otherwise.
 **/
gboolean
gclue_location_source_get_active (GClueLocationSource *source)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), FALSE);

        return (source->priv->active_counter > 0);
}

/**
 * gclue_location_source_get_available_accuracy_level:
 * @source: a #GClueLocationSource
 *
 * Returns: The currently available accuracy level.
 **/
GClueAccuracyLevel
gclue_location_source_get_available_accuracy_level (GClueLocationSource *source)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), 0);

        return source->priv->avail_accuracy_level;
}
