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

G_DEFINE_ABSTRACT_TYPE (GClueLocationSource, gclue_location_source, G_TYPE_OBJECT)

struct _GClueLocationSourcePrivate
{
        GeocodeLocation *location;
};

enum
{
        PROP_0,
        PROP_LOCATION,
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

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_location_source_class_init (GClueLocationSourceClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->get_property = gclue_location_source_get_property;
        object_class->set_property = gclue_location_source_set_property;
        g_type_class_add_private (object_class, sizeof (GClueLocationSourcePrivate));

        gParamSpecs[PROP_LOCATION] = g_param_spec_object ("location",
                                                          "Location",
                                                          "Location",
                                                          GEOCODE_TYPE_LOCATION,
                                                          G_PARAM_READWRITE);
        g_object_class_install_property (object_class,
                                         PROP_LOCATION,
                                         gParamSpecs[PROP_LOCATION]);
}

static void
gclue_location_source_init (GClueLocationSource *source)
{
        source->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (source,
                                             GCLUE_TYPE_LOCATION_SOURCE,
                                             GClueLocationSourcePrivate);
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
