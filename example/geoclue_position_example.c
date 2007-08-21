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

#include <stdio.h>
#include <geoclue/position.h>
#include <glib.h>
#include <math.h>
#include <time.h>
/**
 *
 * Call the test program with no arguments (default backend will be used),
 * or use the name of the backend as argument:
 *     ./geoclue-position-example gpsd
 *
**/


int main (int argc, char** argv)
{
   
    g_type_init ();
    g_thread_init (NULL);
    
    if (argv[1] != NULL) {
        g_debug ("Testing specified backend: %s", argv[1]);
		position_provider* provider;
        provider = g_malloc(sizeof(position_provider));	
		provider->service = g_strdup_printf ("org.freedesktop.geoclue.position.%s", argv[1]);
		provider->path = g_strdup_printf ("/org/freedesktop/geoclue/position/%s", argv[1]);
		provider->connection = NULL;
		provider->proxy = NULL;
		provider->ref = 1;

        if (geoclue_position_init (provider) != GEOCLUE_POSITION_SUCCESS){
        	g_printerr ("Initialization failed");
        	return 1;
        }    
    } else {
        g_debug ("Testing default backend");
        
        if (geoclue_position_init (NULL) != GEOCLUE_POSITION_SUCCESS){
        	g_printerr ("Initialization failed");
        	return 1;
        }
    }

    gint status;
    char* reason;
    g_debug ("Querying geoclue_position_service_status");
    if (geoclue_position_service_status (NULL, &status, &reason) != GEOCLUE_POSITION_SUCCESS) {
        g_debug ("current status query failed");
    } else {
        g_debug ("current status query ok");
        printf ("The backend status is %d\n %d No service available\n %d acquiring altitude \n %d acquiring longitude \n %d acquiring latitude \n %d has alititude available \n %d  has longitude available  \n %d  has latitude available  \n for the reason %s\n", 
        		status,
        		(status) == GEOCLUE_POSITION_NO_SERVICE_AVAILABLE,
        		(status & GEOCLUE_POSITION_ACQUIRING_ALTITUDE) == GEOCLUE_POSITION_ACQUIRING_ALTITUDE,
        		(status & GEOCLUE_POSITION_ACQUIRING_LONGITUDE) == GEOCLUE_POSITION_ACQUIRING_LONGITUDE,
        		(status & GEOCLUE_POSITION_ACQUIRING_LATITUDE) == GEOCLUE_POSITION_ACQUIRING_LATITUDE,
        		(status & GEOCLUE_POSITION_ALTITUDE_AVAILABLE) == GEOCLUE_POSITION_ALTITUDE_AVAILABLE,
        		(status & GEOCLUE_POSITION_LONGITUDE_AVAILABLE) == GEOCLUE_POSITION_LONGITUDE_AVAILABLE,
        		(status & GEOCLUE_POSITION_LATITUDE_AVAILABLE) == GEOCLUE_POSITION_LATITUDE_AVAILABLE,
        		reason);
    }
    
    
    gdouble lat, lon, altitude;
    time_t current_time, timestamp;
    struct tm ts;
    char buf[100];   
    g_debug ("Querying current position");
    if (geoclue_position_current_position (NULL, &timestamp, &lat, &lon, &altitude) != GEOCLUE_POSITION_SUCCESS) {
        g_debug ("current position query failed");
    } else {
        g_debug ("current position query ok");
        
        printf ("You are at %lf %lf with an altitude of %lf\n", lat, lon, altitude);
        printf ("This reading was taken at ");
        ts = *localtime(&timestamp);
        strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
        printf("%s\n", buf);
    }
    
    gchar* country = NULL;
    gchar* locality = NULL;
    gchar* postalcode = NULL;
    gchar* street = NULL;
    gchar* desc = NULL;
  
    g_debug ("Querying civic location");
    if (geoclue_position_civic_location (NULL, &country, NULL, &locality, NULL, &postalcode, &street, NULL, NULL,NULL, &desc, NULL) != GEOCLUE_POSITION_SUCCESS) {
        g_debug ("civic location query failed");
    } else {
        g_debug ("civic location query ok");
        if (country) printf ("Country: %s\n", country);
        if (locality) printf ("Locality: %s\n", locality);
        if (postalcode) printf ("Postalcode: %s\n", postalcode);
        if (street) printf ("Street: %s\n", street);
        if (desc) printf ("Description: %s\n", desc);
    }

    if (geoclue_position_close (NULL) != GEOCLUE_POSITION_SUCCESS){
        g_debug ("position_close failed");
    } else {
        g_debug ("position_close ok");
    }
    
    return 0;   
}
