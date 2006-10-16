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
#include <geoclue/geocode.h>
#include <glib.h>

int main (int argc, char** argv)
{
    g_type_init();
    geoclue_geocode_init();

    char* street = "1104 Mass" ;
    char* city = "Lawrence";
    char* state = "ks";
    char* zip = "";

    gdouble lat, lon;
    gint ret_code;
    geoclue_geocode_to_lat_lon ( street , city , state , zip,&lat, &lon,&ret_code );
    printf("for address %s %s %s %s is at %f %f\n", street , city , state , zip,lat, lon);
    char* freetext = "sunnyville, ca" ;
    geoclue_geocode_free_text_to_lat_lon ( freetext, &lat, &lon,&ret_code  );
    printf("for address %s is at %f %f\n", freetext, lat, lon);   
    geoclue_geocode_lat_lon_to_address(lat, lon, &street , &city , &state , &zip, &ret_code  );
    printf(" %f %f is address %s %s %s %s\n",lat, lon, street , city , state , zip); 

    
 
    return 0;   
}
