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
#ifndef __ORG_FREEDESKTOP_GEOCLUE_POSITION_GEOCLUE_POSITION_H__
#define __ORG_FREEDESKTOP_GEOCLUE_POSITION_GEOCLUE_POSITION_H__

#include <dbus/dbus-glib.h>


/** \page Position
 *
 * \section intro_sec Introduction
 * 
 * Have you ever wondered what you could do with an application if you used your users location to customize his experience?
 * This is what the position API is designed for, to make it easy for you to add this to the user experience.
 * You could ...
 *  - Set the timezone automatically based on position.
 *  - Add a position sender to your Instant Messaging Client to share position with friends.
 *  - Tag all your imported photos with the position where they are taken.
 * 
 * \section usage_sec Usage
 * 
 * The Geoclue Position API is meant to be simple yet useful.  This will be describing the C API, but is applicable to the DBUS specification.
 *
 * This is the example from geoclue_position_example.c
 * 
 * position.h
**/


G_BEGIN_DECLS

/** position_returncode is the return code from all method indicating the success of the call made. 
 */
typedef enum position_returncode
{
    GEOCLUE_POSITION_SUCCESS                  =  0,
    GEOCLUE_POSITION_NOT_INITIALIZED          = -1, 
    GEOCLUE_POSITION_DBUS_ERROR               = -2,
    GEOCLUE_POSITION_SERVICE_NOT_AVAILABLE    = -3, 
    GEOCLUE_POSITION_METHOD_NOT_IMPLEMENTED   = -4,
    GEOCLUE_POSITION_NO_SATELLITE_FIX         = -5,
    GEOCLUE_POSITION_SATELLITE_NOT_IN_VIEW    = -6,
    GEOCLUE_POSITION_INVALID_INPUT            = -7, 
   
} position_returncode;

/** position_status is a set of flags that is bitpacked into a integer for geoclue_position_service_status command 
 */
typedef enum position_status
{
    GEOCLUE_POSITION_NO_SERVICE_AVAILABLE    = 0,  
    GEOCLUE_POSITION_ACQUIRING_ALTITUDE      = 1,  
    GEOCLUE_POSITION_ACQUIRING_LONGITUDE     = 2,  
    GEOCLUE_POSITION_ACQUIRING_LATITUDE      = 4,  
    GEOCLUE_POSITION_ALTITUDE_AVAILABLE      = 8,  
    GEOCLUE_POSITION_LONGITUDE_AVAILABLE     = 16,  
    GEOCLUE_POSITION_LATITUDE_AVAILABLE      = 32     
  
} position_status;

/*! This is an type used for advanced purposes (opening multiple backends manually), generally programs should pass a NULL pointer to functions to recieve default functionality
 * For advanced use allocate the struct and set service and path properly, set connection and proxy to NULL and ref to 1
 * */
typedef struct position_provider
{
	char* service;
	char* path;
	DBusGConnection* connection;
	DBusGProxy*      proxy;
	int ref;	
}position_provider;



typedef void (*position_callback)(	void* ignore,
									gint timestamp,
									gdouble lat,
									gdouble lon,
									gdouble altitude,
									void* userdata	);

typedef void (*civic_callback)(	   void* ignore,
								   char* country, 
                                   char* region,
                                   char* locality,
                                   char* area,
                                   char* postalcode,
                                   char* street,
                                   char* building,
                                   char* floor,
                                   char* room,
                                   char* description,
                                   char* text,
                                   void* userdata	);

typedef void (*status_callback)(	void* ignore,
									gint status,
									char* user_message,
									void* userdata	);

/*!
* \brief This will read in static information files of backends install on the system 
* \param service DBUS service path array
* \param path DBUS path to object array
* \param desc Text Description array
* \return see position_returncode
*/
position_returncode geoclue_position_get_all_providers(	char*** OUT_service,
														char*** OUT_path,
														char*** OUT_desc	);

 /*!
 * \brief This function will initialize a connection to a geoclue position backend.
 * \param provider this will accept NULL and will use the default geoclue master position provider, if not NULL, it will allocation as specified
 * \return see position_returncoder
 */
position_returncode geoclue_position_init(	position_provider* provider );
   
 /*!
 * \brief This Function will close Dbus connections and clean up memory.
 * \param provider this will accept NULL and will use the default geoclue master position provider 
 * \return see position_returncode
 */
position_returncode geoclue_position_close(position_provider* provider);  

/*!
 * \brief A function to find the version of protocol the backend is speaking
 * \param provider this will accept NULL and will use the default geoclue master position provider 
 * \param major Major Version Number
 * \param minor MInor Version Number
 * \param micro Micro Version Number
 * \return see position_returncode
 */
position_returncode geoclue_position_version(	position_provider* provider,
												int* major,
												int* minor,
												int* micro	);
 /*!
 * \brief Text description of current provider
 * \param provider this will accept NULL and will use the default geoclue master position provider 
 * \param name name of provider
 * \return see position_returncode
 */     
position_returncode geoclue_position_service_name(	position_provider* provider,
													char** name);       

 /*!
 * \brief This will query the current position from the selected backend.  
 *  Please make sure and query service status also to find which parameters are valid.
 * \param provider this will accept NULL and will use the default geoclue master position provider 
 * \param OUT_timestamp  Unix time (seconds since the epoch)
 * \param OUT_latitude In degrees
 * \param OUT_longitude In degrees
 * \param OUT_longitude In Meters above sea level
 * \return see position_returncode
 */  
position_returncode geoclue_position_current_position (	position_provider* provider,
														gint* OUT_timestamp,
														gdouble* OUT_latitude,
														gdouble* OUT_longitude,
														gdouble* OUT_altitude );

/*!
 * \brief Register a callback for Asyncronous notification of position
 * \param provider this will accept NULL and will use the default geoclue master position provider 
 * \param callback Function to call
 * \param userdata User specified data
 * \return see position_returncode
 */  
position_returncode geoclue_position_add_position_callback(	position_provider* provider,
															position_callback callback,
															void* userdata );
/*!
* \brief Remove a callback for Asyncronous notification of position, you must supply the same function pointer and userdata to remove properly
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param callback Function to remove
* \param userdata User specified data, must be the same as provided on setup
* \return see position_returncode
*/  
position_returncode geoclue_position_remove_position_callback(	position_provider* provider,
																position_callback callback,
																void* userdata );   

 /*!
 * \brief This returns the accuracy of the current backend.   Please check status to see if these are valid.   If status does not state that one dimension is unavailable, then the return value here is undefined.
 * \param provider this will accept NULL and will use the default geoclue master position provider 
 * \param OUT_latitude_error Error in Meters
 * \param OUT_longitude_error Error in Meters
 * \param OUT_longitude_error Error in Meters
 * \return see position_returncode
 */  
position_returncode geoclue_position_current_position_error (	position_provider* provider,
																gdouble* OUT_latitude_error, 
																gdouble* OUT_longitude_error, 
																gdouble* OUT_altitude_error );
/*!
* \brief This returns the current status (or state) of the backend.  This will tell you what dimension of position are valid
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param OUT_status This is a position_status set of bits
* \param OUT_string This message is a user readable string, generally used to explain why the backend cannot locate the current position  ("Weak GPS Signal, no internet connection available for lookup")
* \return see position_returncode
*/     
position_returncode geoclue_position_service_status	 (	position_provider* provider,
														gint* OUT_status, 
														char** OUT_string );    
/*!
* \brief Register a callback for asyncronous notification of status
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param callback Function to call
* \param userdata User specified data
* \return see position_returncode
*/  
position_returncode geoclue_position_add_status_callback(	position_provider* provider,
															status_callback callback,
															void* userdata );
/*!
* \brief Remove a callback for asyncronous notification of status, you must supply the same function pointer and userdata to remove properly
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param callback Function to remove
* \param userdata User specified data, must be the same as provided on setup
* \return see position_returncode
*/  
position_returncode geoclue_position_remove_status_callback(	position_provider* provider,
																status_callback callback,
																void* userdata );   



/*!
* \brief This will query the current velocity from the selected backend.  
*  Please make sure and query service status also to find which parameters are valid.
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param OUT_timestamp  Unix time (seconds since the epoch)
* \param OUT_north_velocity In degrees
* \param OUT_east_velocity In degrees
* \param OUT_altitude_velocity In Meters above sea level
* \return see position_returncode
*/    
position_returncode geoclue_position_current_velocity (	position_provider* provider,
														gint* OUT_timestamp,
														gdouble* OUT_north_velocity, 
														gdouble* OUT_east_velocity,
														gdouble* OUT_altitude_velocity);


/*!
* \brief If you current backend is a GPS device this will tell you the PRN numbers of the satellites in view
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param OUT_prn_numbers  A GArray of ints that represent prn numbers
* \return see position_returncode
*/      
position_returncode geoclue_position_satellites_in_view (	position_provider* provider,
															GArray** OUT_prn_numbers );

/*!
* \brief This will return satellite information for the requested PRN number.  This function return GArrays of it's outputs.   Each individual satellites data can be found by indexing through the GArray at the same index.   I.E. prn[0] elevation[0] will correspond to one satellite.
* \param provider this will accept NULL and will use the default geoclue master position provider  
* \param IN_prn_number PRN number of satellite (int)
* \param OUT_elevation satellite data (double)
* \param OUT_azimuth satellite data (double)
* \param OUT_signal_noise_ratio satellite data (double)
* \param OUT_differential satellite data (boolean)
* \param OUT_ephemeris satellite data (boolean)
* \return see position_returncode
*/      
position_returncode geoclue_position_satellites_data (	position_provider* provider,
														gint* OUT_timestamp,
														GArray** OUT_prn_number,
														GArray** OUT_elevation, 
														GArray** OUT_azimuth, 
														GArray** OUT_signal_noise_ratio, 
														GArray** OUT_differential, 
														GArray** OUT_ephemeris );
/*!
* \brief This will attempt to return string information about the current location.   Strings may be NULL!!!
* NOTE: civic location part of the API is not stable !
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param OUT_country
* \param OUT_region
* \param OUT_locality
* \param OUT_area
* \param OUT_postalcode
* \param OUT_street
* \param OUT_building
* \param OUT_floor
* \param OUT_description
* \param OUT_room
* \param OUT_text A compiled full string description of the current location
* \return see position_returncode
*/ 
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
                                                        char** OUT_text);

/*!
* \brief Register a callback for Asyncronous notification of civic location changed
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param callback Function to call
* \param userdata User specified data
* \return see position_returncode
*/     
position_returncode geoclue_position_add_civic_callback (	position_provider* provider,
  															civic_callback callback,
   															void* userdata );
    
    /*!
* \brief Remove a callback for Asyncronous notification of civic location changed, you must supply the same function pointer and userdata to remove properly
* \param provider this will accept NULL and will use the default geoclue master position provider 
* \param callback Function to remove
* \param userdata User specified data, must be the same as provided on setup
* \return see position_returncode
*/     
position_returncode geoclue_position_remove_civic_callback (	position_provider* provider,
 															civic_callback callback,
 															void* userdata );


G_END_DECLS


#endif // __ORG_FREEDESKTOP_GEOCLUE_POSITION_GEOCLUE_POSITION_H__























