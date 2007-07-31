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

/**
 *
 * Call the test program with no arguments (default backend will be used),
 * or use the name of the backend as argument:
 *     ./geoclue-position-example gpsd
 *
**/


int main (int argc, char** argv)
{
    gdouble lat, lon;
    gboolean init_ok = FALSE;
    
    g_type_init ();
    g_thread_init (NULL);
    
    if (argv[1] != NULL) {
        g_debug ("Testing specified backend: %s", argv[1]);
        
        gchar* service = g_strdup_printf ("org.foinse_project.geoclue.position.%s", argv[1]);
        gchar* path = g_strdup_printf ("/org/foinse_project/geoclue/position/%s", argv[1]);
        if (geoclue_position_init_specific (service, path) == GEOCLUE_POSITION_SUCCESS){
            init_ok = TRUE;
        }    
    } else {
        g_debug ("Testing default backend");
        
        if (geoclue_position_init () == GEOCLUE_POSITION_SUCCESS){
            init_ok = TRUE;
        }
    }
    
    if (!init_ok) {
        g_printerr ("Initialization failed");
        return 1;
    }

    g_debug ("Querying current position");
    if (geoclue_position_current_position (&lat, &lon) != GEOCLUE_POSITION_SUCCESS) {
        g_debug ("current position query failed");
    } else {
        g_debug ("current position query ok");
        printf ("You are at %f %f\n\n", lat, lon);
    }
    
    gchar* locality = NULL;
    gchar* country = NULL;

    g_debug ("Querying civic location");
    if (geoclue_position_civic_location (&country, NULL, &locality, NULL,NULL, NULL, NULL, NULL,NULL, NULL) != GEOCLUE_POSITION_SUCCESS) {
        g_debug ("civic location query failed");
    } else {
        g_debug ("civic location query ok");

        if (locality && country) {
            printf("You are in %s, %s\n\n", locality, country);
        }else if (country){
            /*TODO: This is never reached. Investigate why pointers are non-null 
                    when there's no data? */
            printf("You are in %s\n\n", country);
        }
    }


    if (geoclue_position_close () != GEOCLUE_POSITION_SUCCESS){
        g_debug ("position_close failed");
    } else {
        g_debug ("position_close ok");
    }
    
    return 0;   
}
