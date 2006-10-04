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

#include <geoclueserver-yahoo.h>
#include <geoclueserver_server_glue.h>
#include <geoclueserver_signal_marshal.h>
#include <dbus/dbus-glib-bindings.h>
#include <geoclue/geoclue.h>
#include <geomap/geomap_gtk_layout.h>


#include <gtk/gtk.h>
#include <geomap/geomap_gtk_layout.h>
#include <stdio.h>
#include <string.h>

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



G_DEFINE_TYPE(Geoclueserver, geoclueserver, G_TYPE_OBJECT)



enum {
  CURRENT_POSITION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

//Default handler
void geoclueserver_current_position_changed(Geoclueserver* obj, gdouble lat, gdouble lon)
{   
    g_print("Current Position Changed\n");
}

static void
geoclueserver_init (Geoclueserver *obj)
{
	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeoclueserverClass *klass = GEOCLUESERVER_GET_CLASS(obj);
	guint request_ret;
	
	dbus_g_connection_register_g_object (klass->connection,
			GEOCLUESERVER_DBUS_PATH ,
			G_OBJECT (obj));

	driver_proxy = dbus_g_proxy_new_for_name (klass->connection,
			DBUS_SERVICE_DBUS,
			DBUS_PATH_DBUS,
			DBUS_INTERFACE_DBUS);


	if(!org_freedesktop_DBus_request_name (driver_proxy,
			GEOCLUESERVER_DBUS_SERVICE,
			0, &request_ret,    
			&error))
	{
		g_printerr("Unable to register geoclue service: %s", error->message);
		g_error_free (error);
	}	
}



static void
geoclueserver_class_init (GeoclueserverClass *klass)
{
	GError *error = NULL;

    klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

	signals[CURRENT_POSITION_CHANGED] =
        g_signal_new ("current_position_changed",
                TYPE_GEOCLUESERVER,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GeoclueserverClass, current_position_changed),
                NULL, 
                NULL,
                _geoclueserver_VOID__DOUBLE_DOUBLE,
                G_TYPE_NONE, 2 ,G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  
    klass->current_position_changed = geoclueserver_current_position_changed;
 
	if (klass->connection == NULL)
	{
		g_printerr("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}	

	dbus_g_object_type_install_info (TYPE_GEOCLUESERVER, &dbus_glib_geoclueserver_object_info);	
    
}


gboolean geoclueserver_version (Geoclueserver *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error)
{
    *OUT_major = 1;
    *OUT_minor = 0;
    *OUT_micro = 0;   
    return TRUE;
}


gboolean geoclueserver_service_provider(Geoclueserver *obj, char** name, GError **error)
{
    *name = "Yahoo and NMEA";
    return TRUE;
}

gboolean geoclueserver_current_position(Geoclueserver *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error )
{
    g_object_get(G_OBJECT(layout), "latitude", OUT_latitude , "longitude", OUT_longitude, NULL);   
    return TRUE;
}

gboolean geoclueserver_current_position_error(Geoclueserver *obj, gdouble* OUT_latitude_error, gdouble* OUT_longitude_error, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_current_altitude(Geoclueserver *obj, gdouble* OUT_altitude, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_current_velocity(Geoclueserver *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_current_time(Geoclueserver *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_satellites_in_view(Geoclueserver *obj, GArray** OUT_prn_numbers, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_satellites_data(Geoclueserver *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_sun_rise(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_sun_set(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_moon_rise(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_moon_set(Geoclueserver *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_geocode(Geoclueserver *obj, const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error )
{
    return FALSE;
}

gboolean geoclueserver_geocode_free_text(Geoclueserver *obj, const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error )
{
    return FALSE;
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
    geomap_gtk_layout_zoom( GEOMAP_GTK_LAYOUT(layout),  zoo);
    geomap_gtk_layout_lat_lon( GEOMAP_GTK_LAYOUT(layout),lat ,lon );
}



void zoom_in_clicked( GtkWidget *widget, gpointer   data )
{
    geomap_gtk_layout_zoom_in(GEOMAP_GTK_LAYOUT(layout));
}

void zoom_out_clicked( GtkWidget *widget, gpointer   data )
{
    geomap_gtk_layout_zoom_out(GEOMAP_GTK_LAYOUT(layout));
}



int main(int argc, char **argv) 
{
    gtk_init (&argc, &argv);

    if(geomap_init())
    {   
        g_print("Error Opening Geomap\n");       
    }    
    
    Geoclueserver* obj = NULL;  
    obj = GEOCLUESERVER(g_type_create_instance (geoclueserver_get_type()));   
    
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


    // This is a little weird to get the geomap layout widget to work
    // You create an event box and use that for the constructor
    // Then you add the eventbox to whatever widget you want the
    // Geo layout to appear    
    GtkWidget* eventbox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (vbox),eventbox , TRUE,TRUE, 0);  
    layout = geomap_gtk_layout_new( GTK_EVENT_BOX(eventbox), DEFAULT_LAT, DEFAULT_LON, DEFAULT_ZOOM );

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





