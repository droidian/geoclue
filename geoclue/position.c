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

#include <position.h>
#include <position_client_glue.h>
#include <stdio.h>
#include <position_signal_marshal.h>
#include <geoclue_master_client_glue.h>

//#define GEOCLUE_POSITION_DBUS_SERVICE     "org.foinse_project.geoclue.position"
//#define GEOCLUE_POSITION_DBUS_PATH        "/org/foinse_project/geoclue/position"
#define GEOCLUE_POSITION_DBUS_INTERFACE   "org.foinse_project.geoclue.position"   
 
#define GEOCLUE_MASTER_DBUS_SERVICE     "org.foinse_project.geoclue.master"
#define GEOCLUE_MASTER_DBUS_PATH        "/org/foinse_project/geoclue/master"
#define GEOCLUE_MASTER_DBUS_INTERFACE   "org.foinse_project.geoclue.master" 
 
        
static  DBusGConnection*        geoclue_position_connection =   NULL;
static  DBusGProxy*             geoclue_position_proxy      =   NULL;
static  GEOCLUE_POSITION_CALLBACK         callbackfunction  =   NULL;
static  void*                   userdatastore     =   NULL; 


   
void geoclue_position_current_position_changed(void* userdata, gdouble lat, gdouble lon)
{

        g_print("current position changed \n");
 
        if(callbackfunction != NULL)
            callbackfunction( lat, lon , userdatastore );           
   
}





GEOCLUE_POSITION_RETURNCODE geoclue_position_version(int* major, int* minor, int* micro)
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_version ( geoclue_position_proxy, major, minor, micro, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position version: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}


       
GEOCLUE_POSITION_RETURNCODE geoclue_position_service_provider(char** name)
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_service_provider ( geoclue_position_proxy, name, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position service provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}
   
   
   
   
GEOCLUE_POSITION_RETURNCODE geoclue_position_init()
{
    GError* error = NULL;
    geoclue_position_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (geoclue_position_connection == NULL)
    {
        g_printerr ("Geomap failed to open connection to bus: %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_POSITION_DBUS_ERROR;
    }
    
    
    
     
    DBusGProxy* master = dbus_g_proxy_new_for_name (geoclue_position_connection,
                                                    GEOCLUE_MASTER_DBUS_SERVICE,
                                                    GEOCLUE_MASTER_DBUS_PATH,
                                                    GEOCLUE_MASTER_DBUS_INTERFACE);   
    
   
    char* service;
    char* path;
    org_foinse_project_geoclue_master_get_best_position_provider (master, &service, &path, &error);
    if( error != NULL )
    {
        g_printerr ("Error getting best position provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }   
    
    
    
    
    geoclue_position_proxy = dbus_g_proxy_new_for_name (geoclue_position_connection,
                                                    service,
                                                    path,
                                                    GEOCLUE_POSITION_DBUS_INTERFACE);
    
    free(service);
    free(path);                                
    dbus_g_object_register_marshaller ( _geoclue_position_VOID__DOUBLE_DOUBLE,
                                        G_TYPE_NONE,
                                        G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (geoclue_position_proxy,
                             "current_position_changed",
                             G_TYPE_DOUBLE, G_TYPE_DOUBLE,  G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (geoclue_position_proxy,
                                 "current_position_changed",
                                 (GCallback) geoclue_position_current_position_changed,
                                 (gpointer)NULL,
                                 (GClosureNotify) NULL);

    callbackfunction    = NULL;
    userdatastore       = NULL;  
    return GEOCLUE_POSITION_SUCCESS;        
}

 
GEOCLUE_POSITION_RETURNCODE geoclue_position_close()
{
    g_object_unref (geoclue_position_proxy);
    geoclue_position_proxy = NULL;
    geoclue_position_connection = NULL;
    callbackfunction    = NULL;
    userdatastore       = NULL;
    return GEOCLUE_POSITION_SUCCESS;   
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_current_position ( gdouble* OUT_latitude, gdouble* OUT_longitude )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_current_position ( geoclue_position_proxy, OUT_latitude,OUT_longitude , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_position: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_current_position_error ( gdouble* OUT_latitude_error, gdouble* OUT_longitude_error )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_current_position_error ( geoclue_position_proxy, OUT_latitude_error, OUT_longitude_error, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_position_error: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_current_altitude ( gdouble* OUT_altitude )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_current_altitude ( geoclue_position_proxy, OUT_altitude, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_altitude: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_current_velocity ( gdouble* OUT_north_velocity, gdouble* OUT_east_velocity )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_current_velocity ( geoclue_position_proxy, OUT_north_velocity, OUT_east_velocity, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_velocity: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_current_time ( gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_current_time ( geoclue_position_proxy, OUT_year, OUT_month , OUT_day , OUT_hours , OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_time: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_satellites_in_view ( GArray** OUT_prn_numbers )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_satellites_in_view ( geoclue_position_proxy, OUT_prn_numbers, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position satellites_in_view: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_satellites_data ( const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_satellites_data  ( geoclue_position_proxy, IN_prn_number , OUT_elevation, OUT_azimuth , OUT_signal_noise_ratio , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position satellites_data : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_sun_rise ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_sun_rise ( geoclue_position_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position sun_rise : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_sun_set ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_sun_set( geoclue_position_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position sun_set : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_moon_rise ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_moon_rise ( geoclue_position_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position moon_riser: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

GEOCLUE_POSITION_RETURNCODE geoclue_position_moon_set ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds )
{
    if(geoclue_position_connection == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
    if(geoclue_position_proxy == NULL)
        return GEOCLUE_POSITION_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_position_moon_set( geoclue_position_proxy, IN_latitude, IN_longitude , IN_year , IN_month  , IN_day , OUT_hours, OUT_minutes , OUT_seconds , &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position moon_set: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}






