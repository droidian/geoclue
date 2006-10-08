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

int main (int argc, char** argv)
{
    g_type_init();
    geoclue_position_init();
    gdouble lat, lon;
    geoclue_position_current_position(&lat, &lon);
    printf("You are at %f %f\n", lat, lon);
    
 
    return 0;   
}
