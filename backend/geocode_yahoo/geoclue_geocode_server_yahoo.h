/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2006-2007 by Garmin Ltd. or its subsidiaries
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


#include <config.h>
#include <dbus/dbus-glib.h>
#include "../common/geoclue_web_service.h"

#define GEOCLUE_GEOCODE_DBUS_SERVICE     "org.freedesktop.geoclue.geocode.yahoo"
#define GEOCLUE_GEOCODE_DBUS_PATH        "/org/freedesktop/geoclue/geocode/yahoo"
#define GEOCLUE_GEOCODE_DBUS_INTERFACE   "org.freedesktop.geoclue.geocode"


G_BEGIN_DECLS

typedef struct GeoclueGeocode GeoclueGeocode;
typedef struct GeoclueGeocodeClass GeoclueGeocodeClass;

GType geoclue_map_get_type (void);
struct GeoclueGeocode
{
    GObject parent;
    GMainLoop*  loop;     
    GeoclueWebService *web_service;
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

gboolean geoclue_geocode_service_available(GeoclueGeocode *obj, gboolean* OUT_available, char** OUT_reason, GError** error);
gboolean geoclue_geocode_shutdown(GeoclueGeocode *obj, GError** error);

G_END_DECLS


#endif
