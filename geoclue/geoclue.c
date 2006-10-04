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

#define DBUS_API_SUBJECT_TO_CHANGE 

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

#include <geoclue.h>
#include <geoclue_client_glue.h>
#include <stdio.h>
#include <geoclue_signal_marshal.h>


#define GEOCLUE_DBUS_SERVICE     "org.foinse_project.geoclue"
#define GEOCLUE_DBUS_PATH        "/org/foinse_project/geoclue"
#define GEOCLUE_DBUS_INTERFACE   "org.foinse_project.geoclue"   
        
static  DBusGConnection*        geoclue_connection =   NULL;
static  DBusGProxy*             geoclue_proxy      =   NULL;
static  GEOCLUE_CALLBACK         callbackfunction  =   NULL;
static  void*                   userdatastore     =   NULL; 


   
void geoclue_current_position_changed(void* userdata, gdouble lat, gdouble lon)
{

        g_print("current position changed \n");
 
        if(callbackfunction != NULL)
            callbackfunction( lat, lon , userdatastore );           
   
}





GEOCLUE_RETURNCODE geoclue_version(int* major, int* minor, int* micro)
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_version ( geoclue_proxy, major, minor, micro, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue version: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}


       
GEOCLUE_RETURNCODE geoclue_service_provider(char** name)
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_service_provider ( geoclue_proxy, name, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue service provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}
   
   
   
   
GEOCLUE_RETURNCODE geoclue_init()
{
    GError* error = NULL;
    geoclue_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (geoclue_connection == NULL)
    {
        g_printerr ("Geomap failed to open connection to bus: %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_DBUS_ERROR;
    }
    geoclue_proxy = dbus_g_proxy_new_for_name (geoclue_connection,
                                                    GEOCLUE_DBUS_SERVICE,
                                                    GEOCLUE_DBUS_PATH,
                                                    GEOCLUE_DBUS_INTERFACE);
                                    
    dbus_g_object_register_marshaller ( _geoclue_VOID__DOUBLE_DOUBLE,
                                        G_TYPE_NONE,
                                        G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (geoclue_proxy,
                             "current_position_changed",
                             G_TYPE_DOUBLE, G_TYPE_DOUBLE,  G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (geoclue_proxy,
                                 "current_position_changed",
                                 (GCallback) geoclue_current_position_changed,
                                 (gpointer)NULL,
                                 (GClosureNotify) NULL);

    callbackfunction    = NULL;
    userdatastore       = NULL;  
    return GEOCLUE_SUCCESS;        
}

 
GEOCLUE_RETURNCODE geoclue_close()
{
    g_object_unref (geoclue_proxy);
    geoclue_proxy = NULL;
    geoclue_connection = NULL;
    callbackfunction    = NULL;
    userdatastore       = NULL;
    return GEOCLUE_SUCCESS;   
}

GEOCLUE_RETURNCODE geoclue_current_position ( gdouble* OUT_latitude, gdouble* OUT_longitude )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_current_position ( geoclue_proxy, OUT_latitude,OUT_longitude , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue current_position: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_current_position_error ( gdouble* OUT_latitude_error, gdouble* OUT_longitude_error )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_current_position_error ( geoclue_proxy, OUT_latitude_error, OUT_longitude_error, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue current_position_error: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_current_altitude ( gdouble* OUT_altitude )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_current_altitude ( geoclue_proxy, OUT_altitude, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue current_altitude: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_current_velocity ( gdouble* OUT_north_velocity, gdouble* OUT_east_velocity )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_current_velocity ( geoclue_proxy, OUT_north_velocity, OUT_east_velocity, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue current_velocity: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_current_time ( gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_current_time ( geoclue_proxy, OUT_year, OUT_month , OUT_day , OUT_hours , OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue current_time: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_satellites_in_view ( GArray** OUT_prn_numbers )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_satellites_in_view ( geoclue_proxy, OUT_prn_numbers, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue satellites_in_view: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_satellites_data ( const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_satellites_data  ( geoclue_proxy, IN_prn_number , OUT_elevation, OUT_azimuth , OUT_signal_noise_ratio , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue satellites_data : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_sun_rise ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_sun_rise ( geoclue_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue sun_rise : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_sun_set ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_sun_set( geoclue_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue sun_set : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_moon_rise ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_moon_rise ( geoclue_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue moon_riser: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_moon_set ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_moon_set( geoclue_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue moon_set: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_geocode ( const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_geocode ( geoclue_proxy, IN_street , IN_city , IN_state , IN_zip , OUT_latitude , OUT_longitude , OUT_return_code , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue geocode: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}

GEOCLUE_RETURNCODE geoclue_geocode_free_text ( const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code )
{
    if(geoclue_connection == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
    if(geoclue_proxy == NULL)
        return GEOCLUE_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_geocode_free_text ( geoclue_proxy, IN_free_text , OUT_latitude , OUT_longitude , OUT_return_code ,  &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue geocode_free_text: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_DBUS_ERROR;        
    }
    return GEOCLUE_SUCCESS;              
}







