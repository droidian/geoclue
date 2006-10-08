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
#include <geoclue_master.h>
#include <geoclue_map_server_glue.h>
#include <geoclue_map_signal_marshal.h>
#include <dbus/dbus-glib-bindings.h>




G_DEFINE_TYPE(Geoclue, geoclue, G_TYPE_OBJECT)


/* Filter signals and args */
enum {
  /* FILL ME */
  GET_MAP_FINISHED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];







static void
geoclue_init (Geoclue *obj)
{


	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeoclueClass *klass = GEOCLUE_GET_CLASS(obj);
	guint request_ret;
	
	/* Register DBUS path */
	dbus_g_connection_register_g_object (klass->connection,
			GEOCLUE_DBUS_PATH ,
			G_OBJECT (obj));

	/* Register the service name, the constant here are defined in dbus-glib-bindings.h */
	driver_proxy = dbus_g_proxy_new_for_name (klass->connection,
			DBUS_SERVICE_DBUS,
			DBUS_PATH_DBUS,
			DBUS_INTERFACE_DBUS);


	if(!org_freedesktop_DBus_request_name (driver_proxy,
			GEOCLUE_DBUS_SERVICE,
			0, &request_ret,    /* See tutorial for more infos about these */
			&error))
	{
		g_printerr("Unable to register geomap service: %s", error->message);
		g_error_free (error);
        exit(1);
	}	

  


}



static void
geoclue_class_init (GeoclueClass *klass)
{
	GError *error = NULL;

    klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

/*
	signals[GET_MAP_FINISHED] =
        g_signal_new ("get_map_finished",
                TYPE_GEOCLUE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GeoclueClass, get_map_finished),
                NULL, // accumulator 
                NULL, // accumulator data 
                _geoclue_map_VOID__INT_BOXED_STRING,
                G_TYPE_NONE, 3 ,G_TYPE_INT,  DBUS_TYPE_G_UCHAR_ARRAY , G_TYPE_STRING);
  
    klass->get_map_finished = geoclue_map_get_map_finished;
  
*/
   
    
    
	if (klass->connection == NULL)
	{
		g_printerr("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}	
	/* &dbus_glib__object_info is provided in the server-bindings.h file */
	/* OBJECT_TYPE_SERVER is the GType of your server object */
	dbus_g_object_type_install_info (TYPE_GEOCLUE, &dbus_glib_geoclue_map_object_info);	
    
}


gboolean geoclue_map_version (Geoclue *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error)
{

    
    return TRUE;
}


gboolean geoclue_map_service_provider(Geoclue *obj, char** name, GError **error)
{

    return TRUE;
}

gboolean geoclue_map_max_zoom(Geoclue *obj, int* max_zoom, GError **error)
{

    return TRUE;
}


gboolean geoclue_map_min_zoom(Geoclue *obj, int* min_zoom, GError **error)
{
    return TRUE;
}

gboolean geoclue_map_max_height(Geoclue *obj, int* max_height, GError **error)
{

    return TRUE;
}

gboolean geoclue_map_min_height(Geoclue *obj, int* min_height, GError **error)
{

    return TRUE;
}

gboolean geoclue_map_max_width(Geoclue *obj, int* max_width, GError **error)
{

    return TRUE;
}

gboolean geoclue_map_min_width(Geoclue *obj, int* min_width, GError **error)
{

    return TRUE;
}






gboolean geoclue_map_get_map (Geoclue *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, int* return_code, GError **error)
{
    
    return TRUE;
}

gboolean geoclue_map_latlong_to_offset(Geoclue *obj, const gdouble IN_latitude, const gdouble IN_longitude,  const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, int* OUT_x_offset, int* OUT_y_offset, GError **error)
{
    
    return TRUE;
}

gboolean geoclue_map_offset_to_latlong(Geoclue *obj, const int IN_x_offset,const int IN_y_offset, const gint IN_zoom, const gdouble IN_center_latitude, const gdouble IN_center_longitude, gdouble* OUT_latitude, gdouble* OUT_longitude,  GError **error )
{

    return TRUE;
}

gboolean geoclue_map_find_zoom_level (Geoclue *obj, const gdouble IN_latitude_top_left, const gdouble IN_longitude_top_left, const gdouble IN_latitude_bottom_right, const gdouble IN_longitude_bottom_right, const gint IN_width, const gint IN_height,  gint* OUT_zoom, GError** error)
{


    return TRUE;       
}






int main( int   argc,
          char *argv[] )
{
    guint request_name_result;
    
    g_type_init ();

    
    GMainLoop*  loop = g_main_loop_new(NULL,TRUE);    

    Geoclue* obj = NULL; 
  
    obj = GEOCLUE(g_type_create_instance (geoclue_get_type()));
        

    g_main_loop_run(loop);
    
    g_object_unref(obj);   
    g_main_loop_unref(loop);
    
    
    return 0;
}








