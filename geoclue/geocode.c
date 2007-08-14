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

#define DBUS_API_SUBJECT_TO_CHANGE 

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

#include <geocode.h>
#include <geocode_client_glue.h>
#include <stdio.h>
#include <geoclue_master_client_glue.h>
#include <config.h>

#define GEOCLUE_GEOCODE_DBUS_SERVICE     "org.freedesktop.geoclue.geocode.yahoo"
#define GEOCLUE_GEOCODE_DBUS_PATH        "/org/freedesktop/geoclue/geocode/yahoo"
#define GEOCLUE_GEOCODE_DBUS_INTERFACE   "org.freedesktop.geoclue.geocode"   
 
        
static  DBusGConnection*        geoclue_geocode_connection =   NULL;
static  DBusGProxy*             geoclue_geocode_proxy      =   NULL;



GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_version(int* major, int* minor, int* micro)
{
    if(geoclue_geocode_connection == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
    if(geoclue_geocode_proxy == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_geocode_version ( geoclue_geocode_proxy, major, minor, micro, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_geocode version: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_GEOCODE_DBUS_ERROR;        
    }
    return GEOCLUE_GEOCODE_SUCCESS;              
}


       
GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_service_provider(char** name)
{
    if(geoclue_geocode_connection == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
    if(geoclue_geocode_proxy == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_geocode_service_provider ( geoclue_geocode_proxy, name, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_geocode service provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_GEOCODE_DBUS_ERROR;        
    }
    return GEOCLUE_GEOCODE_SUCCESS;              
}
   
   
   
   
GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_init()
{
    GError* error = NULL;
    geoclue_geocode_connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
    if (geoclue_geocode_connection == NULL)
    {
        g_printerr ("Failed to open connection to bus: %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_GEOCODE_DBUS_ERROR;
    }
    
   
    
    
    
    geoclue_geocode_proxy = dbus_g_proxy_new_for_name (geoclue_geocode_connection,
                                                    GEOCLUE_GEOCODE_DBUS_SERVICE,
                                                    GEOCLUE_GEOCODE_DBUS_PATH,
                                                    GEOCLUE_GEOCODE_DBUS_INTERFACE);
    
    return GEOCLUE_GEOCODE_SUCCESS;        
}

 
GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_close()
{
    g_object_unref (geoclue_geocode_proxy);
    geoclue_geocode_proxy = NULL;
    geoclue_geocode_connection = NULL;
    return GEOCLUE_GEOCODE_SUCCESS;   
}

GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_to_lat_lon ( const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code )
{
    if(geoclue_geocode_connection == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
    if(geoclue_geocode_proxy == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_geocode_to_lat_lon ( geoclue_geocode_proxy, IN_street , IN_city , IN_state , IN_zip , OUT_latitude , OUT_longitude , OUT_return_code , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_geocode geocode: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_GEOCODE_DBUS_ERROR;        
    }
    return GEOCLUE_GEOCODE_SUCCESS;              
}

GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_free_text_to_lat_lon ( const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code )
{
    if(geoclue_geocode_connection == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
    if(geoclue_geocode_proxy == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_geocode_free_text_to_lat_lon ( geoclue_geocode_proxy, IN_free_text , OUT_latitude , OUT_longitude , OUT_return_code ,  &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_geocode geocode_free_text: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_GEOCODE_DBUS_ERROR;        
    }
    return GEOCLUE_GEOCODE_SUCCESS;              
}

GEOCLUE_GEOCODE_RETURNCODE geoclue_geocode_lat_lon_to_address ( gdouble IN_latitude, gdouble IN_longitude, char ** OUT_street, char ** OUT_city, char ** OUT_state, char ** OUT_zip, gint* OUT_return_code )
{
    if(geoclue_geocode_connection == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
    if(geoclue_geocode_proxy == NULL)
        return GEOCLUE_GEOCODE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_geocode_lat_lon_to_address  ( geoclue_geocode_proxy,  IN_latitude, IN_longitude, OUT_street, OUT_city, OUT_state, OUT_zip, OUT_return_code ,  &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_geocode geocode lat lon to addres: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_GEOCODE_DBUS_ERROR;        
    }
    return GEOCLUE_GEOCODE_SUCCESS;              
}





