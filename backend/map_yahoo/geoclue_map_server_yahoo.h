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
#ifndef __GEOMAPSERVER_SERVER_H__
#define __GEOMAPSERVER_SERVER_H__

#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#define GEOMAPSERVER_DBUS_SERVICE     "org.foinse_project.geomap"
#define GEOMAPSERVER_DBUS_PATH        "/org/foinse_project/geomap"
#define GEOMAPSERVER_DBUS_INTERFACE   "org.foinse_project.geomap"





G_BEGIN_DECLS

//Let's create a geomapserver object that has one method of geomapserver
typedef struct Geomapserver Geomapserver;
typedef struct GeomapserverClass GeomapserverClass;

GType geomapserver_get_type (void);
struct Geomapserver
{
    GObject parent;
   
    char* buffer;
    GArray* OUT_map_buffer;
    gint width;
    gint height;
    
    
    gboolean pending_request;
     
};

struct GeomapserverClass
{
  GObjectClass parent;
  DBusGConnection *connection;
  

  
          /* Signals */
    void (*get_map_finished) (Geomapserver*, gint, GArray* , gchar* );
};

#define TYPE_GEOMAPSERVER              (geomapserver_get_type ())
#define GEOMAPSERVER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOMAPSERVER, Geomapserver))
#define GEOMAPSERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOMAPSERVER, GeomapserverClass))
#define IS_GEOMAPSERVER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOMAPSERVER))
#define IS_GEOMAPSERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOMAPSERVER))
#define GEOMAPSERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOMAPSERVER, GeomapserverClass))

gboolean geomapserver_version (Geomapserver *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);
gboolean geomapserver_service_provider(Geomapserver *obj, char** name, GError **error);   
gboolean geomapserver_max_zoom(Geomapserver *obj, int* max_zoom, GError **error);
gboolean geomapserver_min_zoom(Geomapserver *obj, int* min_zoom, GError **error);
gboolean geomapserver_max_height(Geomapserver *obj, int* max_height, GError **error);
gboolean geomapserver_min_height(Geomapserver *obj, int* min_height, GError **error);
gboolean geomapserver_max_width(Geomapserver *obj, int* max_width, GError **error);
gboolean geomapserver_min_width(Geomapserver *obj, int* min_width, GError **error);
gboolean geomapserver_get_map (Geomapserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, int* return_code, GError **error);
gboolean geomapserver_latlong_to_offset(Geomapserver *obj, const gdouble IN_latitude, const gdouble IN_longitude,  const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, int* OUT_x_offset, int* OUT_y_offset, GError **error);
gboolean geomapserver_offset_to_latlong(Geomapserver *obj, const int IN_x_offset,const int IN_y_offset, const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, gdouble* OUT_latitude, gdouble* OUT_longitude,  GError **error );
gboolean geomapserver_find_zoom_level (Geomapserver *obj, const gdouble IN_latitude_top_left, const gdouble IN_longitude_top_left, const gdouble IN_latitude_bottom_right, const gdouble IN_longitude_bottom_right, const gint IN_width, const gint IN_height,  gint* OUT_zoom, GError** error);




G_END_DECLS




#endif

