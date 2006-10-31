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
#ifndef __ORG_FOINSE_PROJECT_GEOCLUE_POSITION_GEOCLUE_POSITION_H__
#define __ORG_FOINSE_PROJECT_GEOCLUE_POSITION_GEOCLUE_POSITION_H__

#include <dbus/dbus-glib.h>


G_BEGIN_DECLS


typedef enum _geoclue_position_returncode
{
    GEOCLUE_POSITION_SUCCESS                  = 0,
    GEOCLUE_POSITION_NOT_INITIALIZED          = -1, 
    GEOCLUE_POSITION_DBUS_ERROR               = -2,
    GEOCLUE_POSITION_SERVICE_NOT_AVAILABLE    = -3, 
   
} GEOCLUE_POSITION_RETURNCODE;



    typedef void (*GEOCLUE_POSITION_CALLBACK)(gdouble lat, gdouble lon, void* userdata);

 
    /*!
     * \brief texttospeech version 
     * \param major placeholder for version return 
     * \param minor 
     * \param micro
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_POSITION_RETURNCODE geoclue_position_version(int* major, int* minor, int* micro);
    
     /*!
     * \brief texttospeech init, must be called first.  Uses default provider 
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_POSITION_RETURNCODE geoclue_position_init();
    
     /*!
     * \brief texttospeech init, must be called first.  
     * \param service DBUS service path 
     * \param path DBUS path to object
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_POSITION_RETURNCODE geoclue_position_init_specific(char* service, char* path);

     /*!
     * \brief query master for all providers of position 
     * \param service DBUS service path array
     * \param path DBUS path to object array
     * \param desc Text Description array
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_POSITION_RETURNCODE geoclue_position_get_all_providers(char*** OUT_service, char*** OUT_path, char*** OUT_desc);
    
    
     /*!
     * \brief Clean up and free memory 
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */
    GEOCLUE_POSITION_RETURNCODE geoclue_position_close();  
    
     /*!
     * \brief Text description of current provider
     * \param name name of provider
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */     
    GEOCLUE_POSITION_RETURNCODE geoclue_position_service_provider(char** name);       

     /*!
     * \brief Current Position
     * \param OUT_latitude In degrees
     * \param OUT_longitude In degrees
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */  
    GEOCLUE_POSITION_RETURNCODE geoclue_position_current_position ( gdouble* OUT_latitude, gdouble* OUT_longitude );
    
     /*!
     * \brief Asyncronous notification of location changed callback
     * \param callback Function to call
     * \param userdata User specified data
     * \return TRUE Version returned sucessfully
     *         FALSE Error
     */  
    GEOCLUE_POSITION_RETURNCODE geoclue_position_set_position_callback(GEOCLUE_POSITION_CALLBACK callback, void* userdata );    
    
    
     /*!
     * \brief Not implemented Yet
     */
    GEOCLUE_POSITION_RETURNCODE geoclue_position_current_position_error ( gdouble* OUT_latitude_error, gdouble* OUT_longitude_error );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_current_altitude ( gdouble* OUT_altitude );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_current_velocity ( gdouble* OUT_north_velocity, gdouble* OUT_east_velocity );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_current_time ( gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_satellites_in_view ( GArray** OUT_prn_numbers );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_satellites_data ( const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_sun_rise ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_sun_set ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_moon_rise ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds );
    GEOCLUE_POSITION_RETURNCODE geoclue_position_moon_set ( const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds );


G_END_DECLS


#endif // __ORG_FOINSE_PROJECT_GEOCLUE_POSITION_GEOCLUE_POSITION_H__























