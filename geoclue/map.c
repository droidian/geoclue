/* GEOCLUE_MAP - A DBus api and wrapper for getting geography pictures
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

#include <map.h>
#include <map_client_glue.h>
#include <stdio.h>
#include <map_signal_marshal.h>


#define GEOCLUE_MAP_DBUS_SERVICE     "org.foinse_project.geoclue.geoclue_map"
#define GEOCLUE_MAP_DBUS_PATH        "/org/foinse_project/geoclue/geoclue_map"
#define GEOCLUE_MAP_DBUS_INTERFACE   "org.foinse_project.geoclue.geoclue_map"   
        
static  DBusGConnection*        geoclue_map_connection =   NULL;
static  DBusGProxy*             geoclue_map_proxy      =   NULL;
static  GEOCLUE_MAP_CALLBACK         callbackfunction  =   NULL;
static  void*                   userdatastore     =   NULL; 


   
void geoclue_map_get_geoclue_map_finished(void* userdata, GEOCLUE_MAP_RETURNCODE returncode, GArray* geoclue_map_buffer, gchar* buffer_mime_type)
{   
   // printf("\n\n\nGet Map Finished returned \n\n\n\n");
   if(returncode == GEOCLUE_MAP_SUCCESS)
   {
        if(callbackfunction != NULL)
            callbackfunction( returncode, geoclue_map_buffer, buffer_mime_type , userdatastore );           
   }
   else
   {
        g_printerr("GEOCLUE_MAPping get geoclue_map error return code %d\n", returncode);
   }
}



GEOCLUE_MAP_RETURNCODE geoclue_map_version(int* major, int* minor, int* micro)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_map_version ( geoclue_map_proxy, major, minor, micro, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting version: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_MAP_DBUS_ERROR;        
    }
    return GEOCLUE_MAP_SUCCESS;              
}

GEOCLUE_MAP_RETURNCODE geoclue_map_service_provider(char** name)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_foinse_project_geoclue_map_service_provider( geoclue_map_proxy, name, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting service provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_MAP_DBUS_ERROR;        
    }
    return GEOCLUE_MAP_SUCCESS;              
}
   
   
   
GEOCLUE_MAP_RETURNCODE geoclue_map_init()
{
    GError* error = NULL;
    geoclue_map_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (geoclue_map_connection == NULL)
    {
        g_printerr ("GEOCLUE_MAP failed to open connection to bus: %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_MAP_DBUS_ERROR;
    }
    geoclue_map_proxy = dbus_g_proxy_new_for_name (geoclue_map_connection,
                                                    GEOCLUE_MAP_DBUS_SERVICE,
                                                    GEOCLUE_MAP_DBUS_PATH,
                                                    GEOCLUE_MAP_DBUS_INTERFACE);
                                    
    dbus_g_object_register_marshaller ( _geoclue_map_VOID__INT_BOXED_STRING,
                                        G_TYPE_NONE,
                                        G_TYPE_INT, DBUS_TYPE_G_UCHAR_ARRAY, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (geoclue_map_proxy,
                             "get_geoclue_map_finished",
                             G_TYPE_INT, DBUS_TYPE_G_UCHAR_ARRAY, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (geoclue_map_proxy,
                                 "get_geoclue_map_finished",
                                 (GCallback) geoclue_map_get_geoclue_map_finished,
                                 (gpointer)NULL,
                                 (GClosureNotify) NULL);

    callbackfunction    = NULL;
    userdatastore       = NULL;  
    return GEOCLUE_MAP_SUCCESS;        
}

 
GEOCLUE_MAP_RETURNCODE geoclue_map_close()
{
    g_object_unref (geoclue_map_proxy);
    geoclue_map_proxy = NULL;
    geoclue_map_connection = NULL;
    callbackfunction    = NULL;
    userdatastore       = NULL;
    return GEOCLUE_MAP_SUCCESS;   
}



GEOCLUE_MAP_RETURNCODE geoclue_map_get_map (const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, GEOCLUE_MAP_CALLBACK func, void* userdata)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    //g_print("GEOCLUE_MAPping!\n");
    GError* error = NULL;
    int return_code;
    if(!org_foinse_project_geoclue_map_get_map (geoclue_map_proxy, IN_latitude, IN_longitude, IN_width, IN_height, IN_zoom, &return_code, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        }               
    }
    callbackfunction    = func;
    userdatastore       = userdata;
    //g_print("GEOCLUE_MAPping OUT!\n");    
    
    return  GEOCLUE_MAP_SUCCESS;
}

GEOCLUE_MAP_RETURNCODE geoclue_map_max_zoom(int* max_zoom)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_max_zoom(geoclue_map_proxy, max_zoom, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}
    
GEOCLUE_MAP_RETURNCODE geoclue_map_min_zoom(int* min_zoom)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_min_zoom(geoclue_map_proxy, min_zoom, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}


GEOCLUE_MAP_RETURNCODE geoclue_map_max_height(int* max_height)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_max_height(geoclue_map_proxy, max_height, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}

GEOCLUE_MAP_RETURNCODE geoclue_map_min_height(int* min_height)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_min_height(geoclue_map_proxy, min_height, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}

GEOCLUE_MAP_RETURNCODE geoclue_map_max_width(int* max_width)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_max_width(geoclue_map_proxy, max_width, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}

GEOCLUE_MAP_RETURNCODE geoclue_map_min_width(int* min_width)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_min_width(geoclue_map_proxy, min_width, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}
    
    

GEOCLUE_MAP_RETURNCODE geoclue_map_latlong_to_offset(const gdouble IN_latitude, const gdouble IN_longitude,  const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, int* OUT_x_offset, int* OUT_y_offset)
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_latlong_to_offset(geoclue_map_proxy, IN_latitude, IN_longitude, IN_zoom, IN_center_latitude,   IN_center_longitude, OUT_x_offset, OUT_y_offset, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;
}

GEOCLUE_MAP_RETURNCODE geoclue_map_offset_to_latlong(const int IN_x_offset,const int IN_y_offset, const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, gdouble* OUT_latitude, gdouble* OUT_longitude )
{
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    printf("offset to long\n");
    
    if(!org_foinse_project_geoclue_map_offset_to_latlong(geoclue_map_proxy, IN_x_offset, IN_y_offset, IN_zoom, IN_center_latitude,   IN_center_longitude, OUT_latitude, OUT_longitude, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;    
    
}


GEOCLUE_MAP_RETURNCODE geoclue_map_find_zoom_level (const gdouble IN_latitude_top_left, const gdouble IN_longitude_top_left, const gdouble IN_latitude_bottom_right, const gdouble IN_longitude_bottom_right, const gint IN_width, const gint IN_height, gint* OUT_zoom)
{
    
    if(geoclue_map_connection == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;  
    if(geoclue_map_proxy == NULL)
        return GEOCLUE_MAP_NOT_INITIALIZED;     
    GError* error = NULL;
    if(!org_foinse_project_geoclue_map_find_zoom_level (geoclue_map_proxy, IN_latitude_top_left, IN_longitude_top_left, IN_latitude_bottom_right, IN_width, IN_height, IN_longitude_bottom_right, OUT_zoom, &error))
    {
        if(error != NULL)
        {
            g_printerr(" Error in geoclue_map async dbus call %s\n", error->message);
            g_error_free (error);  
        } 
        return  GEOCLUE_MAP_DBUS_ERROR;              
    }
    return  GEOCLUE_MAP_SUCCESS;    
}




