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
#ifndef __GEOCLUE_GEOCODE_SERVER_H__
#define __GEOCLUE_GEOCODE_SERVER_H__

#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#define GEOCLUE_GEOCODE_DBUS_SERVICE     "org.foinse_project.geoclue.geocode.yahoo"
#define GEOCLUE_GEOCODE_DBUS_PATH        "/org/foinse_project/geoclue/geocode/yahoo"
#define GEOCLUE_GEOCODE_DBUS_INTERFACE   "org.foinse_project.geoclue.geocode"





G_BEGIN_DECLS

//Let's create a geoclue_map object that has one method of geoclue_map
typedef struct GeoclueGeocode GeoclueGeocode;
typedef struct GeoclueGeocodeClass GeoclueGeocodeClass;

GType geoclue_map_get_type (void);
struct GeoclueGeocode
{
    GObject parent;

     
};

struct GeoclueGeocodeClass
{
  GObjectClass parent;
  DBusGConnection *connection;
  
};

#define TYPE_GEOCLUE_GEOCODE              (geoclue_geocode_get_type ())
#define GEOCLUE_GEOCODE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOCLUE_GEOCODE, GeoclueGeocode))
#define GEOCLUE_GEOCODE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOCLUE_GEOCODE, GeoclueGeocodeClass))
#define IS_GEOCLUE_GEOCODE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOCLUE_GEOCODE))
#define IS_GEOCLUE_GEOCODE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOCLUE_GEOCODE))
#define GEOCLUE_GEOCODE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOCLUE_GEOCODE, GeoclueGeocodeClass))




gboolean geoclue_geocode_version (GeoclueGeocode *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);
gboolean geoclue_geocode_service_provider(GeoclueGeocode *obj, char** name, GError **error);   
gboolean geoclue_geocode_to_lat_lon (GeoclueGeocode *obj, const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error );
gboolean geoclue_geocode_free_text_to_lat_lon (GeoclueGeocode *obj, const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error );
gboolean geoclue_geocode_lat_lon_to_address(GeoclueGeocode *obj, gdouble IN_latitude, gdouble IN_longitude, char ** OUT_street, char ** OUT_city, char ** OUT_state, char ** OUT_zip, gint* OUT_return_code, GError **error );




G_END_DECLS




#endif

