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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


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


GList* current_images = NULL;

GtkWidget  *image;
GdkPixbuf *imagepix;
GtkWidget *window;
GtkWidget *latitude, *longitude,  *zoom, *vbox, *hbox, *button;
GError* error;
GtkWidget* layout;
gboolean pending = FALSE;
gint last_width = -1;
gint last_height = -1;
gint width = 0;
gint height = 0;
gdouble motion_click_x = 0;
gdouble motion_click_y = 0;

gdouble last_lat = 0;
gdouble last_lon = 0;
gint last_zoom = 0;

gdouble reference_lat = -1234;
gdouble reference_lon = -1234;
gint reference_x = 0;
gint reference_y = 0;


typedef struct {
   gint x;
   gint y;
}xypoint;



GtkAdjustment* adjustmentx;
GtkAdjustment* adjustmenty; 


void update( gdouble IN_latitude, gdouble IN_longitude, gint IN_zoom);


/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}





static void update_map(GdkPixbuf* map_buffer, void* userdata)
{  
    xypoint* new_pos = (xypoint*)userdata;
    //(int)gtk_adjustment_get_value(adjustmentx)
    //(int)gtk_adjustment_get_value(adjustmenty)
    

    printf("Adding map\n");
    image = gtk_image_new_from_pixbuf(map_buffer);
    current_images = g_list_append(current_images, (gpointer)image);
    //gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET(image)); 
    
    
    
    gtk_layout_put( GTK_LAYOUT(layout),GTK_WIDGET(image),new_pos->x ,new_pos->y );
    gtk_widget_show(GTK_WIDGET(image));
    gtk_widget_show_all(GTK_WIDGET(window));      
    
    free(new_pos);
    
    pending = FALSE;
    if(width != last_width || height != last_height)
    {
        //printf("Finished grab, need to regrab current %d %d last %d %d\n", width, height, last_width, last_height);
        pending = TRUE;
        update(last_lat,last_lon, last_zoom);
    }
    
    
    
}


void update( gdouble IN_latitude, gdouble IN_longitude, gint IN_zoom)
{                
 
    xypoint* new_pos =malloc(sizeof(xypoint));
    
    new_pos->x = (int)gtk_adjustment_get_value(adjustmentx);
    new_pos->y = (int)gtk_adjustment_get_value(adjustmenty); 
    //new_pos->x = (int)gtk_adjustment_get_value(adjustmentx)
    //new_pos->y = (int)gtk_adjustment_get_value(adjustmenty)     

      g_print("grabbing map for %d %d\n", new_pos->x,  new_pos->y); 
            
    geoclue_map_gtk_get_gdk_pixbuf(IN_latitude,IN_longitude,width,height,IN_zoom,update_map,new_pos);
    last_width = width;
    last_height = height;        
    last_lat = IN_latitude;
    last_lon = IN_longitude;
    last_zoom = IN_zoom;               
}



void grab_map_clicked( GtkWidget *widget,
                   gpointer   data )
{
        double lat = 0.0, lon = 0.0;
        gint zoo = 10;
    
        const char* temp;
        
        temp = gtk_entry_get_text(GTK_ENTRY(latitude));
        g_print(temp);
        sscanf(temp, "%lf", &lat);
     //   free(temp);
        
        temp = gtk_entry_get_text(GTK_ENTRY(longitude));
        g_print(temp);
        sscanf(temp, "%lf", &lon);
     //   free(temp);
        
        
        temp = gtk_entry_get_text(GTK_ENTRY(zoom));
        g_print(temp);
        sscanf(temp, "%d", &zoo); 
        
        update(lat,lon,zoo);
}




void image_size_callback(GtkWidget*widget, GtkAllocation *event, gpointer data )
{
    printf("Size Allocated %d %d\n", event->width, event->height);

    width = event->width;
    height = event->height;              
   
    if(pending == FALSE && (width != last_width || height != last_height))
    {
        printf("Size Changed Starting update %d %d to %d %d\n", last_width ,last_height, event->width, event->height);
        pending = TRUE;
        if(reference_lat == -1234)
        {
            reference_lat = DEFAULT_LAT;
            reference_lon = DEFAULT_LON;
        }
        reference_x = gtk_adjustment_get_value(adjustmentx) + width/2 - last_width/2;
        reference_y = gtk_adjustment_get_value(adjustmenty) + height/2 - last_height/2;

        
        update(reference_lat,reference_lon,DEFAULT_ZOOM);
    }  
}
                                        




gboolean image_press_callback( GtkWidget      *widget, 
                                       GdkEventButton *event,
                                       gpointer        data )
{

//printf("image press %f %f\n", event->x, event->y); 

motion_click_x = event->x;
motion_click_y = event->y;

                   
return FALSE;    
}


gboolean image_motion_callback(GtkWidget      *widget,
                                            GdkEventMotion *event,
                                            gpointer        user_data) 
{
  //printf("motion %f %f\n", event->x, event->y); 
 
    gdouble next_x = gtk_adjustment_get_value(adjustmentx) + motion_click_x - event->x;
    gdouble next_y = gtk_adjustment_get_value(adjustmenty) + motion_click_y - event->y;   
   
   motion_click_x = event->x;
    motion_click_y = event->y;
    
 //   printf("changing to  %f %f\n", next_x, next_y);
    
    gtk_adjustment_set_value        (adjustmentx,next_x );
    
    gtk_adjustment_set_value        (adjustmenty,next_y );
 
     gtk_widget_show_all (window);
   
 return FALSE;    
}





gboolean image_release_callback( GtkWidget      *widget, 
                                       GdkEventButton *event,
                                       gpointer        data )
{

//printf("image release %f %f\n", event->x, event->y); 
        pending = TRUE;
         
        gint delta_x = (gtk_adjustment_get_value(adjustmentx) + width/2) - reference_x;
        gint delta_y = reference_y -(gtk_adjustment_get_value(adjustmenty) + height/2) ;
        gdouble new_lat;
        gdouble new_lon;
        printf("shifting %d %d\n", delta_x, delta_y);
        geoclue_map_offset_to_latlong(delta_x, delta_y, DEFAULT_ZOOM, reference_lat, reference_lon, &new_lat, &new_lon);
        reference_lat = new_lat;
        reference_lon = new_lon;
        reference_x += delta_x;
        reference_y += delta_y;
        printf("New Latitude %f %f\n",new_lat, new_lon );       
        update(new_lat,new_lon,DEFAULT_ZOOM);
        
        
        //update(NULL,NULL);
return FALSE;   
}




int main(int argc, char **argv) {
   
    image = NULL;
  //  g_type_init ();
  //  g_thread_init (NULL);
    gtk_init (&argc, &argv);
        

    if( geoclue_map_init())
    {   
        g_print("Error initializing map backend\n");
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
   
   
   
    /* This packs the button into the window (a gtk container). */
    vbox =  gtk_vbox_new                    (FALSE,
                                             2);
 
     hbox =  gtk_hbox_new                    (FALSE,
                                             2);
    
    gtk_container_add (GTK_CONTAINER (window), vbox);
    
    adjustmentx =   GTK_ADJUSTMENT(gtk_adjustment_new(LAYOUT_CENTER_X, 0,LAYOUT_WIDTH, 2,500 ,20));
    adjustmenty =   GTK_ADJUSTMENT(gtk_adjustment_new(LAYOUT_CENTER_Y, 0,LAYOUT_HEIGHT, 2,500 ,20)); 

    layout = gtk_layout_new( GTK_ADJUSTMENT(adjustmentx), GTK_ADJUSTMENT(adjustmenty) );
    
    gtk_layout_set_size (GTK_LAYOUT(layout), LAYOUT_WIDTH, LAYOUT_HEIGHT);
    
    gtk_adjustment_set_value        (adjustmentx,LAYOUT_CENTER_X);
    
    gtk_adjustment_set_value        (adjustmenty,LAYOUT_CENTER_Y);    
    
    
  


    //gtk_container_add (GTK_CONTAINER (vbox), GTK_WIDGET(layout));
    
    GtkWidget*  eventbox = GTK_WIDGET(gtk_event_box_new());

    gtk_box_pack_start(GTK_BOX (vbox), eventbox, TRUE,TRUE, 0);
    gtk_container_add(GTK_CONTAINER (eventbox), layout);

    gtk_event_box_set_above_child   (GTK_EVENT_BOX(eventbox), TRUE);

    gtk_signal_connect( GTK_OBJECT(eventbox), "button-press-event",
                        GTK_SIGNAL_FUNC(image_press_callback), 
                        NULL);
                        
    gtk_signal_connect( GTK_OBJECT(eventbox), "button-release-event",
                        GTK_SIGNAL_FUNC(image_release_callback), 
                        NULL);
    gtk_signal_connect( GTK_OBJECT(eventbox), "motion-notify-event",
                        GTK_SIGNAL_FUNC(image_motion_callback), 
                        NULL);

    gtk_signal_connect( GTK_OBJECT(eventbox), "size-allocate",
                        GTK_SIGNAL_FUNC(image_size_callback), 
                        NULL);



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
 

    
    


    /* and the window */
    gtk_widget_show_all (window);
    
    //update(NULL, NULL);
     
     

     
    

    gtk_main ();

 
    return(0);
}


