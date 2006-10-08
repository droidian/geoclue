/* Geomap - A DBus api and wrapper for getting geography pictures
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
#include <geoclue_map_master.h>
#include <geoclue_position_master.h>





int main( int   argc,
          char *argv[] )
{
    
    g_type_init ();

    
    GMainLoop*  loop = g_main_loop_new(NULL,TRUE);    

    GeoclueMap* map_obj = NULL; 
    map_obj = GEOCLUE_MAP(g_type_create_instance (geoclue_map_get_type()));
    GeocluePosition* position_obj = NULL; 
    position_obj = GEOCLUE_POSITION(g_type_create_instance (geoclue_position_get_type()));
        

    g_main_loop_run(loop);
    
    g_object_unref(map_obj); 
    g_object_unref(position_obj);   
    g_main_loop_unref(loop);
    
    
    return 0;
}








