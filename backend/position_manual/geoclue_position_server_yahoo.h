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

#ifndef __GEOCLUESERVER_SERVER_H__
#define __GEOCLUESERVER_SERVER_H__

#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <glib.h>
#define GEOCLUESERVER_DBUS_SERVICE     "org.foinse_project.geoclue"
#define GEOCLUESERVER_DBUS_PATH        "/org/foinse_project/geoclue"
#define GEOCLUESERVER_DBUS_INTERFACE   "org.foinse_project.geoclue"

G_BEGIN_DECLS

//Let's create a geoclueserver object that has one method of geoclueserver
typedef struct Geoclueserver Geoclueserver;
typedef struct GeoclueserverClass GeoclueserverClass;

GType geoclueserver_get_type (void);
struct Geoclueserver
{
    GObject parent;
     
};

struct GeoclueserverClass
{
  GObjectClass parent;
  DBusGConnection *connection;

          /* Signals */
    void (*current_position_changed) (Geoclueserver*, gdouble, gdouble );
};

#define TYPE_GEOCLUESERVER              (geoclueserver_get_type ())
#define GEOCLUESERVER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOCLUESERVER, Geoclueserver))
#define GEOCLUESERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOCLUESERVER, GeoclueserverClass))
#define IS_GEOCLUESERVER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOCLUESERVER))
#define IS_GEOCLUESERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOCLUESERVER))
#define GEOCLUESERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOCLUESERVER, GeoclueserverClass))

gboolean geoclueserver_version (Geoclueserver *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);
gboolean geoclueserver_service_provider(Geoclueserver *obj, char** name, GError **error);   

gboolean geoclueserver_current_position(Geoclueserver *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error );
gboolean geoclueserver_current_position_error(Geoclueserver *obj, gdouble* OUT_latitude_error, gdouble* OUT_longitude_error, GError **error );
gboolean geoclueserver_current_altitude(Geoclueserver *obj, gdouble* OUT_altitude, GError **error );
gboolean geoclueserver_current_velocity(Geoclueserver *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error );
gboolean geoclueserver_current_time(Geoclueserver *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_satellites_in_view(Geoclueserver *obj, GArray** OUT_prn_numbers, GError **error );
gboolean geoclueserver_satellites_data(Geoclueserver *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error );
gboolean geoclueserver_sun_rise(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_sun_set(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_moon_rise(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_moon_set(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error );
gboolean geoclueserver_geocode(Geoclueserver *obj, const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error );
gboolean geoclueserver_geocode_free_text(Geoclueserver *obj, const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error );



G_END_DECLS




#endif

