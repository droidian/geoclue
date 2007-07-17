
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
#include "map_gtk.h"

static  GEOCLUE_MAP_GTK_CALLBACK         callbackfunction  =   NULL;

void geoclue_map_gtk_callback(GEOCLUE_MAP_RETURNCODE returncode, GArray* map_buffer, gchar* buffer_mime_type, void* userdata)
{ 
    GError* error = NULL;
    GdkPixbuf* buf ;

    GdkPixbufLoader* pixbufloader =  gdk_pixbuf_loader_new_with_mime_type
                                    (buffer_mime_type,
                                     &error);
                                     
    gdk_pixbuf_loader_write         (pixbufloader,
                                    map_buffer->data,
                                    map_buffer->len,
                                     &error);
                                                                    
    buf  = gdk_pixbuf_loader_get_pixbuf(pixbufloader); 

    if(callbackfunction != NULL)
        callbackfunction(buf, userdata);
}

GEOCLUE_MAP_RETURNCODE geoclue_map_gtk_get_gdk_pixbuf (const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, GEOCLUE_MAP_GTK_CALLBACK func, void* userdatain )
{    
 callbackfunction = func;   
 return geoclue_map_get_map(IN_latitude,IN_longitude,IN_width,IN_height, IN_zoom, geoclue_map_gtk_callback, userdatain);
}
