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

#include <find.h>
#include <find_client_glue.h>
#include <stdio.h>
#include <stdlib.h>
#include <find_signal_marshal.h>
#include <geoclue_master_client_glue.h>

#define GEOCLUE_FIND_DBUS_INTERFACE   "org.freedesktop.geoclue.find"   
 
#define GEOCLUE_MASTER_DBUS_SERVICE     "org.freedesktop.geoclue.master"
#define GEOCLUE_MASTER_DBUS_PATH        "/org/freedesktop/geoclue/master"
#define GEOCLUE_MASTER_DBUS_INTERFACE   "org.freedesktop.geoclue.master" 
 
        
static  DBusGConnection*        geoclue_find_connection =   NULL;
static  DBusGProxy*             geoclue_find_proxy      =   NULL;
static  GEOCLUE_FIND_CALLBACK   callbackfunction        =   NULL;
static  void*                   userdatastore           =   NULL; 
static  int                     current_search_id       =   0;

   
//void geoclue_find_results_found(void* userdata, gdouble lat, gdouble lon)
void geoclue_find_results_found(void* userdata, gint search_id, char** name, char** description, GArray* latitude, GArray* longitude , char** address, char** city , char** state, char** zip)
{
printf("Results Found\n");
        if(callbackfunction != NULL)
            callbackfunction(  name, description, latitude, longitude , address, city , state, zip, userdatastore);
        
   
}





GEOCLUE_FIND_RETURNCODE geoclue_find_version(int* major, int* minor, int* micro)
{
    if(geoclue_find_connection == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
    if(geoclue_find_proxy == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_find_version ( geoclue_find_proxy, major, minor, micro, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_find version: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }
    return GEOCLUE_FIND_SUCCESS;              
}


       
GEOCLUE_FIND_RETURNCODE geoclue_find_service_provider(char** name)
{
    if(geoclue_find_connection == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
    if(geoclue_find_proxy == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_find_service_provider ( geoclue_find_proxy, name, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_find service provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }
    return GEOCLUE_FIND_SUCCESS;              
}
   
   
GEOCLUE_FIND_RETURNCODE geoclue_find_init_specific(char* service, char* path)
{
    GError* error = NULL;
    geoclue_find_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (geoclue_find_connection == NULL)
    {
        g_printerr ("Failed to open connection to bus: %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_FIND_DBUS_ERROR;
    }
    
    
    geoclue_find_proxy = dbus_g_proxy_new_for_name (geoclue_find_connection,
                                                    service,
                                                    path,
                                                    GEOCLUE_FIND_DBUS_INTERFACE);
                                
    dbus_g_object_register_marshaller ( _geoclue_find_VOID__INT_BOXED_BOXED_BOXED_BOXED_BOXED_BOXED_BOXED_BOXED,
                                        G_TYPE_NONE,
                                        G_TYPE_INT, G_TYPE_STRV, G_TYPE_STRV, DBUS_TYPE_G_UINT64_ARRAY, DBUS_TYPE_G_UINT64_ARRAY,G_TYPE_STRV, G_TYPE_STRV,G_TYPE_STRV,G_TYPE_STRV, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (geoclue_find_proxy,
                             "results_found",
                             G_TYPE_INT, G_TYPE_STRV, G_TYPE_STRV, DBUS_TYPE_G_UINT64_ARRAY  , DBUS_TYPE_G_UINT64_ARRAY, G_TYPE_STRV,G_TYPE_STRV,G_TYPE_STRV,G_TYPE_STRV, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (geoclue_find_proxy,
                                 "results_found",
                                 (GCallback) geoclue_find_results_found,
                                 (gpointer)NULL,
                                 (GClosureNotify) NULL);

    callbackfunction    = NULL;
    userdatastore       = NULL;  
    current_search_id   = 0;
    return GEOCLUE_FIND_SUCCESS;        
}

  
   
GEOCLUE_FIND_RETURNCODE geoclue_find_init()
{
    GError* error = NULL;
    geoclue_find_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (geoclue_find_connection == NULL)
    {
        g_printerr ("Failed to open connection to bus: %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_FIND_DBUS_ERROR;
    }
    
    
     printf(" master %s and %s\n", GEOCLUE_MASTER_DBUS_SERVICE, GEOCLUE_MASTER_DBUS_PATH);
     
    DBusGProxy* master = dbus_g_proxy_new_for_name (geoclue_find_connection,
                                                    GEOCLUE_MASTER_DBUS_SERVICE,
                                                    GEOCLUE_MASTER_DBUS_PATH,
                                                    GEOCLUE_MASTER_DBUS_INTERFACE);   
    
   
    char* service;
    char* path;
    char* desc;
    org_freedesktop_geoclue_master_get_default_find_provider (master, &service, &path, &desc, &error);
    if( error != NULL )
    {
        g_printerr ("Error getting default find provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }   
    
    
    printf(" Hooking up to %s and %s\n", service, path);
    
    geoclue_find_proxy = dbus_g_proxy_new_for_name (geoclue_find_connection,
                                                    service,
                                                    path,
                                                    GEOCLUE_FIND_DBUS_INTERFACE);
    
    free(service);
    free(path); 
    free(desc);                               
                                
                                
    dbus_g_object_register_marshaller ( _geoclue_find_VOID__INT_BOXED_BOXED_BOXED_BOXED_BOXED_BOXED_BOXED_BOXED,
                                        G_TYPE_NONE,
                                        G_TYPE_INT, G_TYPE_STRV, G_TYPE_STRV, DBUS_TYPE_G_UINT64_ARRAY, DBUS_TYPE_G_UINT64_ARRAY,G_TYPE_STRV, G_TYPE_STRV,G_TYPE_STRV,G_TYPE_STRV, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (geoclue_find_proxy,
                             "results_found",
                             G_TYPE_INT, G_TYPE_STRV, G_TYPE_STRV, DBUS_TYPE_G_UINT64_ARRAY  , DBUS_TYPE_G_UINT64_ARRAY, G_TYPE_STRV,G_TYPE_STRV,G_TYPE_STRV,G_TYPE_STRV, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (geoclue_find_proxy,
                                 "results_found",
                                 (GCallback) geoclue_find_results_found,
                                 (gpointer)NULL,
                                 (GClosureNotify) NULL);

    printf("signal hooked up\n");

    callbackfunction    = NULL;
    userdatastore       = NULL;  
    return GEOCLUE_FIND_SUCCESS;        
}

 
GEOCLUE_FIND_RETURNCODE geoclue_find_close()
{
    g_object_unref (geoclue_find_proxy);
    geoclue_find_proxy = NULL;
    geoclue_find_connection = NULL;
    callbackfunction    = NULL;
    userdatastore       = NULL;
    return GEOCLUE_FIND_SUCCESS;   
}




GEOCLUE_FIND_RETURNCODE geoclue_find_get_all_providers(char*** OUT_service, char*** OUT_path, char*** OUT_desc)
{
    GError* error = NULL;
    if (geoclue_find_connection == NULL)
    {
        geoclue_find_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    }
    
    DBusGProxy* master = dbus_g_proxy_new_for_name (geoclue_find_connection,
                                                    GEOCLUE_MASTER_DBUS_SERVICE,
                                                    GEOCLUE_MASTER_DBUS_PATH,
                                                    GEOCLUE_MASTER_DBUS_INTERFACE);   

    org_freedesktop_geoclue_master_get_all_find_providers (master, OUT_service, OUT_path, OUT_desc, &error);
    if( error != NULL )
    {
        g_printerr ("Error getting all find provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }   
    
    return GEOCLUE_FIND_SUCCESS;        
}




GEOCLUE_FIND_RETURNCODE geoclue_find_top_level_categories (char *** OUT_names)
{
    if(geoclue_find_connection == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
    if(geoclue_find_proxy == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_find_top_level_categories  ( geoclue_find_proxy, OUT_names, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_find top level categories : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }
    return GEOCLUE_FIND_SUCCESS;              
}

GEOCLUE_FIND_RETURNCODE geoclue_find_subcategories (const char * IN_category_name, char *** OUT_subcategory_names)
{
    if(geoclue_find_connection == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
    if(geoclue_find_proxy == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_find_subcategories  ( geoclue_find_proxy, IN_category_name, OUT_subcategory_names, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_find subcategories : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }
    return GEOCLUE_FIND_SUCCESS;              
}

GEOCLUE_FIND_RETURNCODE geoclue_find_find_near (const gdouble IN_latitude, const gdouble IN_longitude, const char * IN_category_name, const char *IN_name, GEOCLUE_FIND_CALLBACK callback, void* userdata)
{
    if(geoclue_find_connection == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
    if(geoclue_find_proxy == NULL)
        return GEOCLUE_FIND_NOT_INITIALIZED;  
    
    callbackfunction    = callback;
    userdatastore       = userdata;  
                                   
    GError* error = NULL;
    org_freedesktop_geoclue_find_find_near  ( geoclue_find_proxy, IN_latitude, IN_longitude, IN_category_name, IN_name ,&current_search_id, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_find find near : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_FIND_DBUS_ERROR;        
    }
    return GEOCLUE_FIND_SUCCESS;              
}






