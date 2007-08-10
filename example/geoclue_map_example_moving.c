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
#include <fcntl.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <dbus/dbus-glib.h>
#include <geoclue/map.h>
#include <geoclue/map_gtk.h>
#include <geoclue/position.h>
#include <stdio.h>
#include <string.h>


#define PROGRAM_HEIGHT 272
#define PROGRAM_WIDTH 480

GtkWidget  *image;
GdkPixbuf *imagepix;
GtkWidget *window;
GError* error;



/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}





static void update_map(GdkPixbuf* map_buffer, void* userdata)
{  
  
    printf("Adding map\n");
    if(image != NULL)
        gtk_container_remove (GTK_CONTAINER (window), GTK_WIDGET(image));    
    image = gtk_image_new_from_pixbuf(map_buffer);
    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET(image)); 
    
    
    gtk_widget_show_all(GTK_WIDGET(window));      
    
   
    
}




void newpos(gint timestamp,	gdouble lat, gdouble lon, gdouble altitude, void* userdata)
{
    
    geoclue_map_gtk_get_gdk_pixbuf(lat,lon,PROGRAM_WIDTH,PROGRAM_HEIGHT,8,update_map,NULL);    
  
}




int main(int argc, char **argv) {
   
    image = NULL;
    gtk_init (&argc, &argv);
        

    if( geoclue_map_init())
    {   
        g_print("Error Opening geoclue map\n");
        
    }    
    if( geoclue_position_init(NULL))
    {   
        g_print("Error Opening geoclue position\n");
        
    }    
    
    
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
            g_signal_connect (G_OBJECT (window), "destroy",
              G_CALLBACK (destroy), NULL);
   
    gtk_window_set_default_size(GTK_WINDOW(window),PROGRAM_WIDTH,PROGRAM_HEIGHT);
   
    geoclue_position_add_position_callback(NULL, newpos, NULL );    
     

    /* and the window */
    gtk_widget_show_all (window);
    
    //update(NULL, NULL);
     
     

     
    

    gtk_main ();

 
    return(0);
}


