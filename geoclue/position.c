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

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

#include <position.h>
#include <position_client_glue.h>
#include <stdio.h>
#include <stdlib.h>
#include <position_signal_marshal.h>
#include <string.h>
#include <config.h>

#define GEOCLUE_POSITION_DBUS_INTERFACE   "org.freedesktop.geoclue.position"   
#define GEOCLUE_MASTER_DBUS_SERVICE     "org.freedesktop.geoclue.master"
#define GEOCLUE_MASTER_DBUS_PATH        "/org/freedesktop/geoclue/master"
       
static  position_provider*        default_position_provider = NULL;

position_returncode geoclue_position_init(	position_provider* provider )
{
	
	if(!provider)
	{
		if(!default_position_provider)
		{
			default_position_provider = g_malloc(sizeof(position_provider));	
			default_position_provider->service = strdup(GEOCLUE_MASTER_DBUS_SERVICE);
			default_position_provider->path = strdup(GEOCLUE_MASTER_DBUS_PATH);
			default_position_provider->connection = NULL;
			default_position_provider->proxy = NULL;
			default_position_provider->ref = 1;
			provider = default_position_provider;
		}
		else
		{
			default_position_provider->ref++;
			return GEOCLUE_POSITION_SUCCESS;
		}
	}
	else
	{
		//for flexibility
		default_position_provider = provider;
	}

	if(!provider->service)
		return GEOCLUE_POSITION_INVALID_INPUT; 
	if(!provider->path)
		return GEOCLUE_POSITION_INVALID_INPUT;
	if(!provider->connection)
	{
	    GError* error = NULL;
	    provider->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	    if (!provider->connection)
	    {
	        g_printerr ("Failed to open connection to bus: %s\n", error->message);
	        g_error_free (error);
	        return GEOCLUE_POSITION_DBUS_ERROR;
	    }
		
	}
	if(!provider->proxy)
	{
		provider->proxy = dbus_g_proxy_new_for_name (	provider->connection,
														provider->service,
														provider->path,
	                                                    GEOCLUE_POSITION_DBUS_INTERFACE);
	}    
                          
    dbus_g_object_register_marshaller ( _geoclue_position_VOID__INT_DOUBLE_DOUBLE_DOUBLE,
                                        G_TYPE_NONE,
                                        G_TYPE_INT,
                                        G_TYPE_DOUBLE, 
                                        G_TYPE_DOUBLE, 
                                        G_TYPE_DOUBLE,
                                        G_TYPE_INVALID);
    dbus_g_proxy_add_signal (	provider->proxy,
		                        "current_position_changed",
		                        G_TYPE_INT,
		                        G_TYPE_DOUBLE, 
		                        G_TYPE_DOUBLE, 
		                        G_TYPE_DOUBLE,
		                        G_TYPE_INVALID);
    
    dbus_g_object_register_marshaller ( _geoclue_position_VOID__INT_STRING,
                                        G_TYPE_NONE,
        		                        G_TYPE_INT,
        		                        G_TYPE_STRING,
                                        G_TYPE_INVALID);
    dbus_g_proxy_add_signal (	provider->proxy,
		                        "service_status_changed",
		                        G_TYPE_INT,
		                        G_TYPE_STRING,
		                        G_TYPE_INVALID);
    dbus_g_object_register_marshaller ( _geoclue_position_VOID__STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING,
                                        G_TYPE_NONE,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
        		                        G_TYPE_STRING,
                                        G_TYPE_INVALID);
    dbus_g_proxy_add_signal (	provider->proxy,
		                        "civic_location_changed",
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_STRING,
		                        G_TYPE_INVALID);

    return GEOCLUE_POSITION_SUCCESS;        
}

position_returncode geoclue_position_close(position_provider* provider)
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
	provider->ref--;
	if(provider->ref == 0)
	{
		g_object_unref (provider->proxy);
	    if(provider->path)
	    	g_free(provider->path);
	    if(provider->service)
	    	g_free(provider->service);   
	    g_free(provider);
	}
    return GEOCLUE_POSITION_SUCCESS;   
}

position_returncode geoclue_position_version(	position_provider* provider,
												int* major,
												int* minor,
												int* micro	)
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    GError* error = NULL;
    org_freedesktop_geoclue_position_version ( provider->proxy, major, minor, micro, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position version: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}
       
position_returncode geoclue_position_service_name(	position_provider* provider,
													char** name)
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}                            
    GError* error = NULL;
    org_freedesktop_geoclue_position_service_name ( provider->proxy, name, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position service provider: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

position_returncode geoclue_position_get_all_providers(char*** OUT_service, char*** OUT_path, char*** OUT_desc)
{
/** TODO copy geoclue master method to here */
	*OUT_service = NULL;
	*OUT_path = NULL;
	*OUT_desc = NULL;
    return GEOCLUE_POSITION_METHOD_NOT_IMPLEMENTED;        
}

position_returncode geoclue_position_current_position (	position_provider* provider,
														gint* OUT_timestamp,
														gdouble* OUT_latitude,
														gdouble* OUT_longitude,
														gdouble* OUT_altitude )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}                                 
    GError* error = NULL;
    org_freedesktop_geoclue_position_current_position ( provider->proxy, OUT_timestamp, OUT_latitude, OUT_longitude, OUT_altitude, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_position: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

position_returncode geoclue_position_add_position_callback(	position_provider* provider,
															position_callback callback,
															void* userdata )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    dbus_g_proxy_connect_signal (	provider->proxy,
	                                "current_position_changed",
	                                (GCallback) callback,
	                                (gpointer)userdata,
	                                (GClosureNotify) NULL);
    return GEOCLUE_POSITION_SUCCESS;
}

position_returncode geoclue_position_remove_position_callback(	position_provider* provider,
															position_callback callback,
															void* userdata )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    dbus_g_proxy_disconnect_signal (	provider->proxy,
		                                "current_position_changed",
		                                (GCallback) callback,
		                                (gpointer)userdata);
    return GEOCLUE_POSITION_SUCCESS;
}

position_returncode geoclue_position_current_position_error (	position_provider* provider,
																gdouble* OUT_latitude_error, 
																gdouble* OUT_longitude_error, 
																gdouble* OUT_altitude_error )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}                               
    GError* error = NULL;
    org_freedesktop_geoclue_position_current_position_error ( provider->proxy, OUT_latitude_error, OUT_longitude_error, OUT_altitude_error, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_position_error: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

position_returncode geoclue_position_service_status	 (	position_provider* provider,
														gint* OUT_status, 
														char** OUT_string )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}                               
    GError* error = NULL;
    org_freedesktop_geoclue_position_service_status ( provider->proxy, OUT_status, OUT_string, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position service_status: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}

position_returncode geoclue_position_add_status_callback(	position_provider* provider,
															status_callback callback,
															void* userdata )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    dbus_g_proxy_connect_signal (	provider->proxy,
	                                "service_status_changed",
	                                (GCallback) callback,
	                                (gpointer)userdata,
	                                (GClosureNotify) NULL);
    return GEOCLUE_POSITION_SUCCESS;
}

position_returncode geoclue_position_remove_status_callback(	position_provider* provider,
															status_callback callback,
															void* userdata )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    dbus_g_proxy_disconnect_signal (	provider->proxy,
		                                "service_status_changed",
		                                (GCallback) callback,
		                                (gpointer)userdata);
    return GEOCLUE_POSITION_SUCCESS;
}

position_returncode geoclue_position_current_velocity (	position_provider* provider,
														gint* OUT_timestamp,
														gdouble* OUT_north_velocity, 
														gdouble* OUT_east_velocity,
														gdouble* OUT_altitude_velocity)
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}                              
    GError* error = NULL;
    org_freedesktop_geoclue_position_current_velocity ( provider->proxy, OUT_timestamp, OUT_north_velocity, OUT_east_velocity, OUT_altitude_velocity, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position current_velocity: %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}




position_returncode geoclue_position_satellites_data (	position_provider* provider,
														gint* OUT_timestamp,
														GArray** OUT_prn_number,
														GArray** OUT_elevation, 
														GArray** OUT_azimuth, 
														GArray** OUT_signal_noise_ratio, 
														GArray** OUT_differential, 
														GArray** OUT_ephemeris )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}                                 
    GError* error = NULL;
    org_freedesktop_geoclue_position_satellites_data  ( provider->proxy, OUT_timestamp, OUT_prn_number , OUT_elevation, OUT_azimuth , OUT_signal_noise_ratio , OUT_differential,  OUT_ephemeris, &error );
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position satellites_data : %s\n", error->message);
        g_error_free (error);  
        return GEOCLUE_POSITION_DBUS_ERROR;        
    }
    return GEOCLUE_POSITION_SUCCESS;              
}


position_returncode geoclue_position_civic_location (	position_provider* provider, 
														char** OUT_country, 
                                                        char** OUT_region,
                                                        char** OUT_locality,
                                                        char** OUT_area,
                                                        char** OUT_postalcode,
                                                        char** OUT_street,
                                                        char** OUT_building,
                                                        char** OUT_floor,
                                                        char** OUT_description,
                                                        char** OUT_room,
                                                        char** OUT_text)
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}   
    GError* error = NULL;
    org_freedesktop_geoclue_position_civic_location (provider->proxy,
                                                        OUT_country,
                                                        OUT_region,
                                                        OUT_locality,
                                                        OUT_area,
                                                        OUT_postalcode,
                                                        OUT_street,
                                                        OUT_building,
                                                        OUT_floor,
                                                        OUT_room,
                                                        OUT_description,
                                                        OUT_text,
                                                        &error);
    if( error != NULL )
    {
        g_printerr ("Error getting geoclue_position civic_location : %s\n", error->message);
        g_error_free (error);
        return GEOCLUE_POSITION_DBUS_ERROR;
    }
    return GEOCLUE_POSITION_SUCCESS;
}

position_returncode geoclue_position_add_civic_callback (	position_provider* provider,
  															civic_callback callback,
   															void* userdata )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    dbus_g_proxy_connect_signal (	provider->proxy,
	                                "civic_location_changed",
	                                (GCallback) callback,
	                                (gpointer)userdata,
	                                (GClosureNotify) NULL);
    return GEOCLUE_POSITION_SUCCESS;
}

position_returncode geoclue_position_remove_civic_callback (	position_provider* provider,
  															civic_callback callback,
   															void* userdata )
{
	if(!provider)
	{
		if(!default_position_provider)
		{
	        return GEOCLUE_POSITION_NOT_INITIALIZED;			
		}
		else
		{
			provider = default_position_provider;
		}
	}
    dbus_g_proxy_disconnect_signal (	provider->proxy,
	                                "civic_location_changed",
	                                (GCallback) callback,
	                                (gpointer)userdata);
    return GEOCLUE_POSITION_SUCCESS;
}



