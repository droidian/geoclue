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
#ifndef __ORG_FREEDESKTOP_GEOCLUE_GEOCODE_GEOCLUE_GEOCODE_H__
#define __ORG_FREEDESKTOP_GEOCLUE_GEOCODE_GEOCLUE_GEOCODE_H__

#include <dbus/dbus-glib.h>


G_BEGIN_DECLS


typedef enum _geoclue_geocode_returncode
{
    GEOCLUE_GEOCODE_SUCCESS                  = 0,
    GEOCLUE_GEOCODE_NOT_INITIALIZED          = -1, 
    GEOCLUE_GEOCODE_DBUS_ERROR               = -2,
    GEOCLUE_GEOCODE_SERVICE_NOT_AVAILABLE    = -3, 
   
} GEOCLUE_GEOCODE_RETURNCODE;



    typedef void (*GEOCLUE_GEOCODE_CALLBACK)(gdouble lat, gdouble lon, void* userdata);

 

    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_version(int* major, int* minor, int* micro);
    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_init();
    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_close();       
    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_service_provider(char** name);       
    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_to_lat_lon ( const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code );
    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_free_text_to_lat_lon ( const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code );
    GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_lat_lon_to_address( gdouble IN_latitude, gdouble IN_longitude, char ** OUT_street, char ** OUT_city, char ** OUT_state, char ** OUT_zip, gint* OUT_return_code );



G_END_DECLS


#endif // __ORG_FREEDESKTOP_GEOCLUE_GEOCODE_GEOCLUE_GEOCODE_H__























