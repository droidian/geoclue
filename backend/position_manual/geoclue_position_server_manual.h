/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2006 Garmin
 * 
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation; 
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GEOCLUE_POSITION_SERVER_H__
#define __GEOCLUE_POSITION_SERVER_H__

#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <glib.h>
#define GEOCLUE_POSITION_DBUS_SERVICE     "org.foinse_project.geoclue.position.manual"
#define GEOCLUE_POSITION_DBUS_PATH        "/org/foinse_project/geoclue/position/manual"
#define GEOCLUE_POSITION_DBUS_INTERFACE   "org.foinse_project.geoclue.position"

G_BEGIN_DECLS

//Let's create a geoclue_position object that has one method of geoclue_position
typedef struct GeocluePosition GeocluePosition;
typedef struct GeocluePositionClass GeocluePositionClass;

GType geoclue_position_get_type (void);
struct GeocluePosition
{
    GObject parent;

};

struct GeocluePositionClass
{
  GObjectClass parent;
  DBusGConnection *connection;

          /* Signals */
    void (*current_position_changed) (GeocluePosition*, gdouble, gdouble );
};

#define TYPE_GEOCLUE_POSITION              (geoclue_position_get_type ())
#define GEOCLUE_POSITION(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOCLUE_POSITION, GeocluePosition))
#define GEOCLUE_POSITION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOCLUE_POSITION, GeocluePositionClass))
#define IS_GEOCLUE_POSITION(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOCLUE_POSITION))
#define IS_GEOCLUE_POSITION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOCLUE_POSITION))
#define GEOCLUE_POSITION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOCLUE_POSITION, GeocluePositionClass))

gboolean geoclue_position_version (GeocluePosition *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);
gboolean geoclue_position_service_provider(GeocluePosition *obj, char** name, GError **error);   

gboolean geoclue_position_current_position(GeocluePosition *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error );
gboolean geoclue_position_current_position_error(GeocluePosition *obj, gdouble* OUT_latitude_error, gdouble* OUT_longitude_error, GError **error );
gboolean geoclue_position_current_altitude(GeocluePosition *obj, gdouble* OUT_altitude, GError **error );
gboolean geoclue_position_current_velocity(GeocluePosition *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error );
gboolean geoclue_position_current_time(GeocluePosition *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclue_position_satellites_in_view(GeocluePosition *obj, GArray** OUT_prn_numbers, GError **error );
gboolean geoclue_position_satellites_data(GeocluePosition *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error );
gboolean geoclue_position_sun_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclue_position_sun_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclue_position_moon_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclue_position_moon_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );

gboolean geoclue_position_civic_location (GeocluePosition* obj,
                                          char** OUT_country,
                                          char** OUT_region,
                                          char** OUT_locality,
                                          char** OUT_area,
                                          char** OUT_postalcode,
                                          char** OUT_street,
                                          char** OUT_building,
                                          char** OUT_floor,
                                          char** OUT_room,
                                          char** OUT_text,
                                          GError** error);

gboolean geoclue_position_service_available(GeocluePosition *obj, gboolean* OUT_available, char** OUT_reason, GError** error);
gboolean geoclue_position_shutdown(GeocluePosition *obj, GError** error);



G_END_DECLS




#endif

