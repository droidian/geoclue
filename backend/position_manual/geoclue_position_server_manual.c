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

#include "geoclue_position_server_manual.h"
#include "../geoclue_position_error.h"
#include <geoclue_position_server_glue.h>
#include <geoclue_position_signal_marshal.h>
#include <geoclue/map.h>
#include <geoclue/map_gtk_layout.h>

#include <dbus/dbus-glib-bindings.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PROGRAM_HEIGHT 640
#define PROGRAM_WIDTH 480
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



G_DEFINE_TYPE(GeocluePosition, geoclue_position, G_TYPE_OBJECT)



enum {
  CURRENT_POSITION_CHANGED,
  SERVICE_STATUS_CHANGED,
  CIVIC_LOCATION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

//Default handler
static void geoclue_position_current_position_changed(	GeocluePosition* server,
														gint timestamp,
														gdouble lat,
														gdouble lon,
														gdouble altitude )
{   
    g_print("Current Position Changed\n");
}
//Default handler
static void geoclue_position_civic_location_changed(	GeocluePosition* server,
														char* country, 
												       	char* region,
												       	char* locality,
												       	char* area,
												       	char* postalcode,
												       	char* street,
												       	char* building,
												       	char* floor,
												       	char* room,
												       	char* description,
												       	char* text )
{   
    g_print("civic_location_changed\n");
}
//Default handler
static void geoclue_position_service_status_changed(	GeocluePosition* server,
														gint status,
														char* user_message )
{
    g_print("service_status_changed\n");
}



static void
geoclue_position_init (GeocluePosition *obj)
{
	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeocluePositionClass *klass = GEOCLUE_POSITION_GET_CLASS(obj);
	guint request_ret;
	
	dbus_g_connection_register_g_object (klass->connection,
			GEOCLUE_POSITION_DBUS_PATH ,
			G_OBJECT (obj));

	driver_proxy = dbus_g_proxy_new_for_name (klass->connection,
			DBUS_SERVICE_DBUS,
			DBUS_PATH_DBUS,
			DBUS_INTERFACE_DBUS);


	if(!org_freedesktop_DBus_request_name (driver_proxy,
			GEOCLUE_POSITION_DBUS_SERVICE,
			0, &request_ret,    
			&error))
	{
		g_printerr("Unable to register geoclue service: %s", error->message);
		g_error_free (error);
	}	
}



static void
geoclue_position_class_init (GeocluePositionClass *klass)
{
	GError *error = NULL;

    klass->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);

	signals[CURRENT_POSITION_CHANGED] =
        g_signal_new ("current_position_changed",
                TYPE_GEOCLUE_POSITION,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GeocluePositionClass, current_position_changed),
                NULL, 
                NULL,
                _geoclue_position_VOID__INT_DOUBLE_DOUBLE_DOUBLE,
                G_TYPE_NONE, 
                4, 
                G_TYPE_INT,
                G_TYPE_DOUBLE, 
                G_TYPE_DOUBLE, 
                G_TYPE_DOUBLE);
	
	signals[SERVICE_STATUS_CHANGED] =
        g_signal_new ("service_status_changed",
                TYPE_GEOCLUE_POSITION,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GeocluePositionClass, service_status_changed),
                NULL, 
                NULL,
                _geoclue_position_VOID__INT_STRING,
                G_TYPE_NONE,
                2,
                G_TYPE_INT,
                G_TYPE_STRING);
	
	signals[CIVIC_LOCATION_CHANGED] =
        g_signal_new ("civic_location_changed",
                TYPE_GEOCLUE_POSITION,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GeocluePositionClass, civic_location_changed),
                NULL, 
                NULL,
                _geoclue_position_VOID__STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING,
                G_TYPE_NONE,
                11,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING,
                G_TYPE_STRING);
  
	klass->current_position_changed = geoclue_position_current_position_changed;
	klass->civic_location_changed = geoclue_position_civic_location_changed;
	klass->service_status_changed = geoclue_position_service_status_changed;
 
	if (klass->connection == NULL)
	{
		g_printerr("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}	

	dbus_g_object_type_install_info (TYPE_GEOCLUE_POSITION, &dbus_glib_geoclue_position_object_info);	
    
}


gboolean geoclue_position_version(	GeocluePosition* server,
									int* major,
									int* minor,
									int* micro, 
									GError **error)
{
    *major = 1;
    *minor = 0;
    *micro = 0;   
    return TRUE;
}


gboolean geoclue_position_service_name(	GeocluePosition* server,
										char** name, 
										GError **error)
{
    *name = "Manual user set";
    return TRUE;
}

gboolean geoclue_position_current_position (	GeocluePosition* server,
												gint* OUT_timestamp,
												gdouble* OUT_latitude,
												gdouble* OUT_longitude,
												gdouble* OUT_altitude, 
												GError **error)
{
	time_t ttime;
	time(&ttime);
	*OUT_timestamp = ttime;
    g_object_get(G_OBJECT(layout), "latitude", OUT_latitude , "longitude", OUT_longitude, NULL);   
    return TRUE;
}

gboolean geoclue_position_current_position_error(	GeocluePosition* server,
													gdouble* OUT_latitude_error, 
													gdouble* OUT_longitude_error, 
													gdouble* OUT_altitude_error, 
													GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}


gboolean geoclue_position_current_velocity (	GeocluePosition* server,
												gint* OUT_timestamp,
												gdouble* OUT_north_velocity, 
												gdouble* OUT_east_velocity,
												gdouble* OUT_altitude_velocity, 
												GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}



gboolean geoclue_position_satellites_data (	GeocluePosition* server,
											gint* OUT_timestamp,
											GArray** OUT_prn_number,
											GArray** OUT_elevation, 
											GArray** OUT_azimuth, 
											GArray** OUT_signal_noise_ratio, 
											GArray** OUT_differential, 
											GArray** OUT_ephemeris,
											GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_civic_location (	GeocluePosition* server, 
											char** OUT_country, 
						                    char** OUT_region,
						                    char** OUT_locality,
						                    char** OUT_area,
						                    char** OUT_postalcode,
						                    char** OUT_street,
						                    char** OUT_building,
						                    char** OUT_floor,
						                    char** OUT_description,
						                    char** OUT_room,
						                    char** OUT_text, 
						                    GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_service_status	 (	GeocluePosition* server,
												gint* OUT_status, 
												char** OUT_string, 
												GError **error)
{
	*OUT_status =     GEOCLUE_POSITION_LONGITUDE_AVAILABLE | GEOCLUE_POSITION_LATITUDE_AVAILABLE;    
	*OUT_string = strdup("Manual Position Set");
	return TRUE;
}

gboolean geoclue_position_shutdown(GeocluePosition *obj, GError** error)
{
    gtk_main_quit ();
    return TRUE;
}





static void destroy( GtkWidget *widget, gpointer   data )
{
    gtk_main_quit ();
}

static void grab_map_clicked( GtkWidget *widget, gpointer  data )
{
    //Let's Grab the Values from Entries
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



int main(int argc, char **argv) 
{
    gtk_init (&argc, &argv);

    if(geoclue_map_init())
    {   
        g_print("Error initializing map backend\n");       
    }    
    
    GeocluePosition* obj = NULL;  
    obj = GEOCLUE_POSITION(g_type_create_instance (geoclue_position_get_type()));   
    
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





