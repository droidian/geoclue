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
#include <fcntl.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <dbus/dbus-glib.h>
#include <geoclue/map.h>
#include <geoclue/map_gtk.h>
#include <geoclue/map_gtk_layout.h>
#include <stdio.h>
#include <string.h>

#define PROGRAM_HEIGHT 640  
#define PROGRAM_WIDTH 480
#define LAYOUT_WIDTH 4000
#define LAYOUT_HEIGHT 4000
#define LAYOUT_CENTER_Y 2000
#define LAYOUT_CENTER_X 2000
#define DEFAULT_LAT 38.857
#define DEFAULT_LON -94.8
#define DEFAULT_ZOOM 8
#define DEFAULT_LAT_STRING "38.857"
#define DEFAULT_LON_STRING "-94.8"
#define DEFAULT_ZOOM_STRING "8"




GtkWidget *window;
GtkWidget *latitude, *longitude,  *zoom, *vbox, *hbox, *button;
GError* error;
GtkWidget* layout;




static void destroy( GtkWidget *widget, gpointer   data )
{
    gtk_main_quit ();
}

void grab_map_clicked( GtkWidget *widget, gpointer   data )
{
    double lat = 0.0, lon = 0.0;
    gint zoo = 10;

    const char* temp;       
    temp = gtk_entry_get_text(GTK_ENTRY(latitude));
    g_print(temp);
    sscanf(temp, "%lf", &lat);
    
    temp = gtk_entry_get_text(GTK_ENTRY(longitude));
    g_print(temp);
    sscanf(temp, "%lf", &lon);
   
    temp = gtk_entry_get_text(GTK_ENTRY(zoom));
    g_print(temp);
    sscanf(temp, "%d", &zoo); 
    
    geoclue_map_gtk_layout_zoom( GEOCLUE_MAP_GTK_LAYOUT(layout),  zoo);
    geoclue_map_gtk_layout_lat_lon( GEOCLUE_MAP_GTK_LAYOUT(layout),lat ,lon );
}


void zoom_in_clicked( GtkWidget *widget, gpointer   data )
{
    geoclue_map_gtk_layout_zoom_in(GEOCLUE_MAP_GTK_LAYOUT(layout));
}
void zoom_out_clicked( GtkWidget *widget, gpointer   data )
{
    geoclue_map_gtk_layout_zoom_out(GEOCLUE_MAP_GTK_LAYOUT(layout));
}









int main(int argc, char **argv) {
   
    gtk_init (&argc, &argv);

    if( geoclue_map_init())
    {   
        g_print("Error Opening Geomap\n");
    }    
    
    latitude = gtk_entry_new(); 
    longitude = gtk_entry_new();  
    zoom = gtk_entry_new(); 
    
    gtk_entry_set_width_chars(GTK_ENTRY(latitude),6);
    gtk_entry_set_width_chars(GTK_ENTRY(longitude),6);
    gtk_entry_set_width_chars(GTK_ENTRY(zoom),2);
   
    gtk_entry_set_text(GTK_ENTRY(latitude),DEFAULT_LAT_STRING);
    gtk_entry_set_text(GTK_ENTRY(longitude),DEFAULT_LON_STRING);
    gtk_entry_set_text(GTK_ENTRY(zoom),DEFAULT_ZOOM_STRING);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
            g_signal_connect (G_OBJECT (window), "destroy",
              G_CALLBACK (destroy), NULL);
   
    gtk_window_set_default_size(GTK_WINDOW(window),PROGRAM_HEIGHT,PROGRAM_WIDTH);
   
   
   

    vbox =  gtk_vbox_new(FALSE, 2);
 
    hbox =  gtk_hbox_new(FALSE, 2);
    
    gtk_container_add (GTK_CONTAINER (window), vbox);
 
 

    // This is a little weird to get the geoclue_map layout widget to work
    // You create an event box and use that for the constructor
    // Then you add the eventbox to whatever widget you want the
    // Geo layout to appear  
        
    GtkWidget* eventbox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (vbox),eventbox , TRUE,TRUE, 0);
    layout = geoclue_map_gtk_layout_new( GTK_EVENT_BOX(eventbox), DEFAULT_LAT, DEFAULT_LON, DEFAULT_ZOOM );

    GtkWidget* temp_label =  gtk_label_new("test");
    gtk_widget_set_size_request(temp_label, 100, 20);
    geoclue_map_gtk_layout_add_widget( GEOCLUE_MAP_GTK_LAYOUT(layout), temp_label, DEFAULT_LAT + 0.05, DEFAULT_LON );



    GtkWidget* tempvbox;
    tempvbox =  gtk_vbox_new(TRUE, 2);
    
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(hbox),FALSE, FALSE, 3);
    
    gtk_container_add(GTK_CONTAINER (hbox), tempvbox);
    
    GtkWidget* label =  gtk_label_new("latitude");
    gtk_container_add (GTK_CONTAINER (tempvbox),label );  
    gtk_container_add (GTK_CONTAINER (tempvbox),latitude );
    
    tempvbox =  gtk_vbox_new(TRUE, 2);
    gtk_container_add(GTK_CONTAINER (hbox), tempvbox);
    label =  gtk_label_new("longitude");
    gtk_container_add (GTK_CONTAINER (tempvbox),label );
    gtk_container_add (GTK_CONTAINER (tempvbox),longitude );
    
    tempvbox =  gtk_vbox_new(TRUE, 2);
    gtk_container_add(GTK_CONTAINER (hbox), tempvbox);
    label =  gtk_label_new("zoom");
    gtk_container_add (GTK_CONTAINER (tempvbox),label );
    gtk_container_add (GTK_CONTAINER (tempvbox),zoom);
    button = gtk_button_new_with_label ("Grab Map");
    g_signal_connect (G_OBJECT (button), "clicked",
              G_CALLBACK (grab_map_clicked), NULL);
    gtk_container_add (GTK_CONTAINER (hbox),button);

    button = gtk_button_new_with_label ("+");
    g_signal_connect (G_OBJECT (button), "clicked",
              G_CALLBACK (zoom_in_clicked), NULL);
    gtk_container_add (GTK_CONTAINER (hbox),button);
    
        button = gtk_button_new_with_label ("-");
    g_signal_connect (G_OBJECT (button), "clicked",
              G_CALLBACK (zoom_out_clicked), NULL);
    gtk_container_add (GTK_CONTAINER (hbox),button);
    
    gtk_widget_show_all (window);
    
    gtk_main ();

 
    return(0);
}


