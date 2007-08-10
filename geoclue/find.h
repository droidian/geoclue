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
#ifndef __ORG_FREEDESKTOP_GEOCLUE_FIND_GEOCLUE_FIND_H__
#define __ORG_FREEDESKTOP_GEOCLUE_FIND_GEOCLUE_FIND_H__

#include <dbus/dbus-glib.h>


G_BEGIN_DECLS


typedef enum _geoclue_find_returncode
{
    GEOCLUE_FIND_SUCCESS                  =  0,
    GEOCLUE_FIND_NOT_INITIALIZED          = -1, 
    GEOCLUE_FIND_DBUS_ERROR               = -2,
    GEOCLUE_FIND_SERVICE_NOT_AVAILABLE    = -3, 
    GEOCLUE_FIND_METHOD_NOT_IMPLEMENTED   = -4,
    GEOCLUE_FIND_NO_SATELLITE_FIX         = -5,
    GEOCLUE_FIND_SATELLITE_NOT_IN_VIEW    = -6
   
} GEOCLUE_FIND_RETURNCODE;

typedef enum _geoclue_find_fix
{
    GEOCLUE_FIND_NO_FIX                         = -1,  
    GEOCLUE_FIND_TWO_DIMENSION                  = 1,  
    GEOCLUE_FIND_THREE_DIMENSION                = 2,  
    GEOCLUE_FIND_TWO_DIMENSION_DIFFERENTIAL     = 3,  
    GEOCLUE_FIND_THREE_DIMENSION_DIFFERENTIAL   = 4    
  
} GEOCLUE_FIND_FIX;


    typedef void (*GEOCLUE_FIND_CALLBACK)(char** name, char** description, GArray* latitude, GArray* longitude , char** address, char** city , char** state, char** zip, void* userdata );

 
    /*!
     * \brief geoclue version 
     * \param major placeholder for version return 
     * \param minor 
     * \param micro
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_FIND_RETURNCODE geoclue_find_version(int* major, int* minor, int* micro);
    
     /*!
     * \brief geoclue init, must be called first.  Uses default provider 
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_FIND_RETURNCODE geoclue_find_init();
    
     /*!
     * \brief geoclue init, must be called first.  
     * \param service DBUS service path 
     * \param path DBUS path to object
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_FIND_RETURNCODE geoclue_find_init_specific(char* service, char* path);

     /*!
     * \brief query master for all providers of find 
     * \param service DBUS service path array
     * \param path DBUS path to object array
     * \param desc Text Description array
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_FIND_RETURNCODE geoclue_find_get_all_providers(char*** OUT_service, char*** OUT_path, char*** OUT_desc);
    
    
     /*!
     * \brief Clean up and free memory 
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_FIND_RETURNCODE geoclue_find_close();  
    
     /*!
     * \brief Text description of current provider
     * \param name name of provider
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */     
    GEOCLUE_FIND_RETURNCODE geoclue_find_service_provider(char** name);       



GEOCLUE_FIND_RETURNCODE geoclue_find_top_level_categories (char *** OUT_names);
GEOCLUE_FIND_RETURNCODE geoclue_find_subcategories (const char * IN_category_name, char *** OUT_subcategory_names);
GEOCLUE_FIND_RETURNCODE geoclue_find_find_near (const gdouble IN_latitude, const gdouble IN_longitude, const char * IN_category_name, const char *IN_name, GEOCLUE_FIND_CALLBACK callback, void* userdata);


   


G_END_DECLS


#endif // __ORG_FREEDESKTOP_GEOCLUE_FIND_GEOCLUE_FIND_H__























