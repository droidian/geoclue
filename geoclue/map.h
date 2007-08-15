/* geoclue_map - A DBus api and wrapper for getting geography pictures
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
#ifndef __ORG_FREEDESKTOP_GEOCLUE_GEOCLUE_MAP_H__
#define __ORG_FREEDESKTOP_GEOCLUE_GEOCLUE_MAP_H__

#include <dbus/dbus-glib.h>

G_BEGIN_DECLS


typedef enum _geoclue_map_returncode
{
    GEOCLUE_MAP_SUCCESS                  = 0,
    GEOCLUE_MAP_NOT_INITIALIZED          = -1, 
    GEOCLUE_MAP_DBUS_ERROR               = -2,
    GEOCLUE_MAP_SERVICE_NOT_AVAILABLE    = -3,
    GEOCLUE_MAP_HEIGHT_TOO_BIG           = -4,
    GEOCLUE_MAP_HEIGHT_TOO_SMALL         = -5,
    GEOCLUE_MAP_WIDTH_TOO_BIG            = -6,
    GEOCLUE_MAP_WIDTH_TOO_SMALL          = -7,
    GEOCLUE_MAP_ZOOM_TOO_BIG             = -8,
    GEOCLUE_MAP_ZOOM_TOO_SMALL           = -9,
    GEOCLUE_MAP_INVALID_LATITUDE         = -10,
    GEOCLUE_MAP_INVALID_LONGITUDE        = -11   
   
} GEOCLUE_MAP_RETURNCODE;



    typedef void (*GEOCLUE_MAP_CALLBACK)(GEOCLUE_MAP_RETURNCODE returncode, GArray* geoclue_map_buffer, gchar* buffer_mime_type, void* userdata);

 
    /*!
     * \brief geoclue_map version 
     * \param major placeholder for version return 
     * \param minor 
     * \param micro
     * \return    GEOCLUE_MAP_NO_ERROR                 = 0,
     *            GEOCLUE_MAP_DBUS_ERROR               = -1,
     *            GEOCLUE_MAP_SERVICE_NOT_AVAILABLE    = -2 
     */
    GEOCLUE_MAP_RETURNCODE geoclue_map_version(int* major, int* minor, int* micro);


    /*!
     * \brief geoclue_map initialization 
     * \return    GEOCLUE_MAP_NO_ERROR                 = 0,
     *            GEOCLUE_MAP_DBUS_ERROR               = -1,
     *            GEOCLUE_MAP_SERVICE_NOT_AVAILABLE    = -2 
     */
    GEOCLUE_MAP_RETURNCODE geoclue_map_init();
    GEOCLUE_MAP_RETURNCODE geoclue_map_init_specific(char* service, char* path);
    GEOCLUE_MAP_RETURNCODE geoclue_map_get_all_providers(char*** OUT_service, char*** OUT_path, char*** OUT_desc);
 

    /*!
     * \brief geoclue_map cleanup 
     * \return    GEOCLUE_MAP_NO_ERROR                 = 0,
     *            GEOCLUE_MAP_DBUS_ERROR               = -1,
     *            GEOCLUE_MAP_SERVICE_NOT_AVAILABLE    = -2 
     */
    GEOCLUE_MAP_RETURNCODE geoclue_map_close();
    
    
    GEOCLUE_MAP_RETURNCODE geoclue_map_service_provider(char** name);
    
    GEOCLUE_MAP_RETURNCODE geoclue_map_max_zoom(int* max_zoom);
    GEOCLUE_MAP_RETURNCODE geoclue_map_min_zoom(int* min_zoom);
    GEOCLUE_MAP_RETURNCODE geoclue_map_max_height(int* max_height);
    GEOCLUE_MAP_RETURNCODE geoclue_map_min_height(int* min_height);
    GEOCLUE_MAP_RETURNCODE geoclue_map_max_width(int* max_width);
    GEOCLUE_MAP_RETURNCODE geoclue_map_min_width(int* min_width);
    GEOCLUE_MAP_RETURNCODE geoclue_map_get_map (const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, GEOCLUE_MAP_CALLBACK func, void* userdatain);
    GEOCLUE_MAP_RETURNCODE geoclue_map_latlong_to_offset(const gdouble IN_latitude, const gdouble IN_longitude,  const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, int* OUT_x_offset, int* OUT_y_offset);
    GEOCLUE_MAP_RETURNCODE geoclue_map_offset_to_latlong(const int IN_x_offset,const int IN_y_offset, const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, gdouble* OUT_latitude, gdouble* OUT_longitude );

    GEOCLUE_MAP_RETURNCODE geoclue_map_find_zoom_level (const gdouble IN_latitude_top_left, const gdouble IN_longitude_top_left, const gdouble IN_latitude_bottom_right, const gdouble IN_longitude_bottom_right, const gint IN_width, const gint IN_height, gint* OUT_zoom);
    





G_END_DECLS


#endif // __ORG_FREEDESKTOP_GEOCLUE_MAP_GEOCLUE_MAP_H__























