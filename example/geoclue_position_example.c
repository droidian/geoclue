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

#include <stdio.h>
#include <geoclue/position.h>
#include <glib.h>
#include <math.h>

int main (int argc, char** argv)
{
    g_type_init();
    printf("Asking for location\n");
    geoclue_position_init();
    //geoclue_position_init_specific("org.foinse_project.geoclue.position.gpsd","/org/foinse_project/geoclue/position/gpsd");
    gdouble lat, lon;
    geoclue_position_current_position(&lat, &lon);
    printf("You are at %f %f\n", lat, lon);
    
    geoclue_position_current_altitude ( &lon );
    
    double temp, temp2;
    geoclue_position_current_velocity( &temp, &temp2 ) ;
    printf("2d vel %f %f \n", temp, temp2);
    temp = sqrt(temp * temp + temp2 * temp2);
    printf("1d vel %f \n", temp);
    geoclue_position_current_altitude( &temp ) ;   
    printf("Altitude %f \n", temp);
     
     
    
 
    return 0;   
}
