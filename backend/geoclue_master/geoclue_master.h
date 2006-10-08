/* Geomap - A DBus api and wrapper for getting geography pictures
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
#ifndef __GEOCLUE_SERVER_H__
#define __GEOCLUE_SERVER_H__

#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#define GEOCLUE_DBUS_SERVICE     "org.foinse_project.geoclue"
#define GEOCLUE_DBUS_PATH        "/org/foinse_project/geoclue"
#define GEOCLUE_DBUS_INTERFACE   "org.foinse_project.geoclue"





G_BEGIN_DECLS

typedef struct Geoclue Geoclue;
typedef struct GeoclueClass GeoclueClass;

GType geoclue_get_type (void);
struct Geoclue
{
    GObject parent;     
};

struct GeoclueClass
{
  GObjectClass parent;
  DBusGConnection *connection;
};

#define TYPE_GEOCLUE              (geoclue_get_type ())
#define GEOCLUE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOCLUE, Geoclue))
#define GEOCLUE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOCLUE, GeoclueClass))
#define IS_GEOCLUE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOCLUE))
#define IS_GEOCLUE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOCLUE))
#define GEOCLUE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOCLUE, GeoclueClass))

gboolean geoclue_map_version (Geoclue *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);
gboolean geoclue_map_service_provider(Geoclue *obj, char** name, GError **error);   
gboolean geoclue_map_max_zoom(Geoclue *obj, int* max_zoom, GError **error);
gboolean geoclue_map_min_zoom(Geoclue *obj, int* min_zoom, GError **error);
gboolean geoclue_map_max_height(Geoclue *obj, int* max_height, GError **error);
gboolean geoclue_map_min_height(Geoclue *obj, int* min_height, GError **error);
gboolean geoclue_map_max_width(Geoclue *obj, int* max_width, GError **error);
gboolean geoclue_map_min_width(Geoclue *obj, int* min_width, GError **error);
gboolean geoclue_map_get_map (Geoclue *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, int* return_code, GError **error);
gboolean geoclue_map_latlong_to_offset(Geoclue *obj, const gdouble IN_latitude, const gdouble IN_longitude,  const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, int* OUT_x_offset, int* OUT_y_offset, GError **error);
gboolean geoclue_map_offset_to_latlong(Geoclue *obj, const int IN_x_offset,const int IN_y_offset, const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, gdouble* OUT_latitude, gdouble* OUT_longitude,  GError **error );
gboolean geoclue_map_find_zoom_level (Geoclue *obj, const gdouble IN_latitude_top_left, const gdouble IN_longitude_top_left, const gdouble IN_latitude_bottom_right, const gdouble IN_longitude_bottom_right, const gint IN_width, const gint IN_height,  gint* OUT_zoom, GError** error);




G_END_DECLS




#endif

