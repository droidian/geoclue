/* vim: set et ts=8 sw=8: */
/* gclue-location.h
 *
 * Copyright 2012 Bastien Nocera
 * Copyright 2015 Ankit (Verma)
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
 *    Authors: Bastien Nocera <hadess@hadess.net>
 *             Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *             Ankit (Verma) <ankitstarski@gmail.com>
 */

#ifndef GCLUE_LOCATION_H
#define GCLUE_LOCATION_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GCLUE_TYPE_LOCATION            (gclue_location_get_type ())
#define GCLUE_LOCATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATION, GClueLocation))
#define GCLUE_IS_LOCATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_LOCATION))
#define GCLUE_LOCATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_LOCATION, GClueLocationClass))
#define GCLUE_IS_LOCATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_LOCATION))
#define GCLUE_LOCATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_LOCATION, GClueLocationClass))

#define INVALID_COORDINATE -G_MAXDOUBLE

typedef struct _GClueLocation        GClueLocation;
typedef struct _GClueLocationClass   GClueLocationClass;
typedef struct _GClueLocationPrivate GClueLocationPrivate;

struct _GClueLocation
{
        /* Parent instance structure */
        GObject parent_instance;

        GClueLocationPrivate *priv;
};

struct _GClueLocationClass
{
        /* Parent class structure */
        GObjectClass parent_class;
};

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GClueLocation, g_object_unref)

GType gclue_location_get_type (void);

/**
 * GCLUE_LOCATION_ALTITUDE_UNKNOWN:
 *
 * Constant representing unknown altitude.
 */
#define GCLUE_LOCATION_ALTITUDE_UNKNOWN -G_MAXDOUBLE

/**
 * GCLUE_LOCATION_ACCURACY_UNKNOWN:
 *
 * Constant representing unknown accuracy.
 */
#define GCLUE_LOCATION_ACCURACY_UNKNOWN -1

/**
 * GCLUE_LOCATION_ACCURACY_STREET:
 *
 * Constant representing street-level accuracy.
 */
#define GCLUE_LOCATION_ACCURACY_STREET 1000 /* 1 km */

/**
 * GCLUE_LOCATION_ACCURACY_CITY:
 *
 * Constant representing city-level accuracy.
 */
#define GCLUE_LOCATION_ACCURACY_CITY 15000 /* 15 km */

/**
 * GCLUE_LOCATION_ACCURACY_REGION:
 *
 * Constant representing region-level accuracy.
 */
#define GCLUE_LOCATION_ACCURACY_REGION 50000 /* 50 km */

/**
 * GCLUE_LOCATION_ACCURACY_COUNTRY:
 *
 * Constant representing country-level accuracy.
 */
#define GCLUE_LOCATION_ACCURACY_COUNTRY 300000 /* 300 km */

/**
 * GCLUE_LOCATION_ACCURACY_CONTINENT:
 *
 * Constant representing continent-level accuracy.
 */
#define GCLUE_LOCATION_ACCURACY_CONTINENT 3000000 /* 3000 km */

/**
 * GCLUE_LOCATION_HEADING_UNKNOWN:
 *
 * Constant representing unknown heading.
 */
#define GCLUE_LOCATION_HEADING_UNKNOWN -1.0

/**
 * GCLUE_LOCATION_SPEED_UNKNOWN:
 *
 * Constant representing unknown speed.
 */
#define GCLUE_LOCATION_SPEED_UNKNOWN -1.0

GClueLocation *gclue_location_new (gdouble latitude,
                                   gdouble longitude,
                                   gdouble accuracy);

GClueLocation *gclue_location_new_full
                                  (gdouble     latitude,
                                   gdouble     longitude,
                                   gdouble     accuracy,
                                   gdouble     speed,
                                   gdouble     heading,
                                   gdouble     altitude,
                                   guint64     timestamp,
                                   const char *description);

GClueLocation *gclue_location_create_from_nmeas
                                  (const char     *nmeas[],
                                   GClueLocation  *prev_location,
                                   GError        **error);

GClueLocation *gclue_location_duplicate
                                  (GClueLocation *location);

void gclue_location_set_description
                                  (GClueLocation *loc,
                                   const char      *description);
const char *gclue_location_get_description
                                  (GClueLocation *loc);

gdouble gclue_location_get_latitude
                                  (GClueLocation *loc);
gdouble gclue_location_get_longitude
                                  (GClueLocation *loc);
gdouble gclue_location_get_altitude
                                  (GClueLocation *loc);
gdouble gclue_location_get_accuracy
                                  (GClueLocation *loc);
guint64 gclue_location_get_timestamp
                                  (GClueLocation *loc);
void gclue_location_set_speed     (GClueLocation *loc,
                                   gdouble        speed);

void gclue_location_set_speed_from_prev_location
                                  (GClueLocation *location,
                                   GClueLocation *prev_location);

gdouble gclue_location_get_speed  (GClueLocation *loc);

void gclue_location_set_heading   (GClueLocation *loc,
                                   gdouble        heading);

void gclue_location_set_heading_from_prev_location
                                  (GClueLocation *location,
                                   GClueLocation *prev_location);


gdouble gclue_location_get_heading
                                  (GClueLocation *loc);
double gclue_location_get_distance_from
                                  (GClueLocation *loca,
                                   GClueLocation *locb);

#endif /* GCLUE_LOCATION_H */
