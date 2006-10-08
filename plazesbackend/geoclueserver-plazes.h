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

#ifndef ___GEOCLUESERVER_PLAZES_H__
#define ___GEOCLUESERVER_PLAZES_H__


#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <glib.h>
#define GEOCLUESERVER_PLAZES_DBUS_SERVICE     "org.foinse_project.geoclue"
#define GEOCLUESERVER_PLAZES_DBUS_PATH        "/org/foinse_project/geoclue"
#define GEOCLUESERVER_PLAZES_DBUS_INTERFACE   "org.foinse_project.geoclue"


G_BEGIN_DECLS

#define GEOCLUESERVER_PLAZES_TYPE                 (geoclueserver_plazes_plazes_get_type ())
#define GEOCLUESERVER_PLAZES(object)              (G_TYPE_CHECK_INSTANCE_CAST ((object), GEOCLUESERVER_PLAZES_TYPE, GeoclueserverPlazes))
#define GEOCLUESERVER_PLAZES_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUESERVER_PLAZES_TYPE, GeoclueserverPlazesClass))
#define IS_GEOCLUESERVER_PLAZES(object)           (G_TYPE_CHECK_INSTANCE_TYPE ((object), GEOCLUESERVER_PLAZES_TYPE))
#define IS_GEOCLUESERVER_PLAZES_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUESERVER_PLAZES_TYPE))
#define GEOCLUESERVER_PLAZES_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCLUESERVER_PLAZES_TYPE, GeoclueserverPlazesClass))

//Let's create a geoclueserver object that has one method of geoclueserver
typedef struct _GeoclueserverPlazes GeoclueserverPlazes;
typedef struct _GeoclueserverPlazesClass GeoclueserverPlazesClass;

GType geoclueserverplazes_get_type (void);
struct GeoclueserverPlazes
{
    GObject parent;
     
};

struct GeoclueserverPlazesClass
{
  GObjectClass parent;
  DBusGConnection *connection;

          /* Signals */
    void (*current_position_changed) (GeoclueserverPlazes*, gdouble, gdouble );
};



gboolean geoclueserver_plazes_version (GeoclueserverPlazes *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);
gboolean geoclueserver_plazes_service_provider(GeoclueserverPlazes *obj, char** name, GError **error);   

gboolean geoclueserver_plazes_current_position(GeoclueserverPlazes *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error );
gboolean geoclueserver_plazes_current_position_error(GeoclueserverPlazes *obj, gdouble* OUT_latitude_error, gdouble* OUT_longitude_error, GError **error );
gboolean geoclueserver_plazes_current_altitude(GeoclueserverPlazes *obj, gdouble* OUT_altitude, GError **error );
gboolean geoclueserver_plazes_current_velocity(GeoclueserverPlazes *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error );
gboolean geoclueserver_plazes_current_time(GeoclueserverPlazes *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_plazes_satellites_in_view(GeoclueserverPlazes *obj, GArray** OUT_prn_numbers, GError **error );
gboolean geoclueserver_plazes_satellites_data(GeoclueserverPlazes *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error );
gboolean geoclueserver_plazes_sun_rise(GeoclueserverPlazes *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_plazes_sun_set(GeoclueserverPlazes *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_plazes_moon_rise(GeoclueserverPlazes *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_plazes_moon_set(GeoclueserverPlazes *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_plazes_geocode(GeoclueserverPlazes *obj, const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error );
gboolean geoclueserver_plazes_geocode_free_text(GeoclueserverPlazes *obj, const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error );


G_END_DECLS

#endif
