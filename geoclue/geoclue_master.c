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
#include <geoclue_master_server_glue.h>
//#include <geoclue_map_signal_marshal.h>
#include <dbus/dbus-glib-bindings.h>


//#include <geoclue_map_client_glue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>





G_DEFINE_TYPE(GeoclueMaster, geoclueserver_master, G_TYPE_OBJECT)


/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


static void
geoclueserver_add_backend(GeoclueMaster *obj, char* path)
{
    printf("Added %s\n", path);
    int newfile = g_open(path, O_RDONLY,0);
    if(newfile != -2)
    {                

        char** backendstrings = malloc(sizeof(char*)*5);
        GIOChannel* channel = g_io_channel_unix_new(newfile);
        g_io_channel_read_line(channel,&backendstrings[0],NULL, NULL, NULL);
        g_io_channel_read_line(channel,&backendstrings[1],NULL, NULL, NULL);
        g_io_channel_read_line(channel,&backendstrings[2],NULL, NULL, NULL);
        g_io_channel_read_line(channel,&backendstrings[3],NULL, NULL, NULL);     
        backendstrings[4] = NULL;
        printf("Added %s\n", backendstrings[0]);
        obj->backends = g_list_prepend(obj->backends, (void*)backendstrings);      
    }
    
}




static void
geoclueserver_master_init (GeoclueMaster *obj)
{
    obj->client = gconf_client_get_default();
    gconf_client_add_dir(obj->client,
                       "/apps/geoclue",
                       GCONF_CLIENT_PRELOAD_RECURSIVE,
                       NULL);
    

    obj->backends = NULL;
	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeoclueMasterClass *klass = GEOCLUE_MASTER_GET_CLASS(obj);
	guint request_ret;
	
	/* Register DBUS path */
	dbus_g_connection_register_g_object (klass->connection,
			GEOCLUE_MASTER_DBUS_PATH ,
			G_OBJECT (obj));


	/* Register the service name, the constant here are defined in dbus-glib-bindings.h */
	driver_proxy = dbus_g_proxy_new_for_name (klass->connection,
			DBUS_SERVICE_DBUS,
			DBUS_PATH_DBUS,
			DBUS_INTERFACE_DBUS);


	if(!org_freedesktop_DBus_request_name (driver_proxy,
			GEOCLUE_MASTER_DBUS_SERVICE,
			0, &request_ret,    /* See tutorial for more infos about these */
			&error))
	{
		g_printerr("Unable to register geomap service: %s", error->message);
		g_error_free (error);
	}	

  GDir* dir = g_dir_open("/usr/local/share/geoclue/backend",0,NULL);
  const gchar* filename;
  if(dir != NULL)
  {
      filename = g_dir_read_name(dir);
      while(filename != NULL)
      {
        char* fullpath = g_strconcat("/usr/local/share/geoclue/backend", "/", filename, NULL);
        geoclueserver_add_backend(obj, fullpath);
        free(fullpath);
        filename = g_dir_read_name(dir);
      }
  }

  dir = g_dir_open("/usr/share/geoclue/backend",0,NULL);
  if(dir != NULL)
  {  
      filename = g_dir_read_name(dir);
      while(filename != NULL)
      {
        char* fullpath = g_strconcat("/usr/share/geoclue/backend", "/", filename, NULL);
        geoclueserver_add_backend(obj, fullpath);
        free(fullpath);
        filename = g_dir_read_name(dir);
      }
  }

  


}



static void
geoclueserver_master_class_init (GeoclueMasterClass *klass)
{
	GError *error = NULL;

    klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

   
    
    
	if (klass->connection == NULL)
	{
		g_printerr("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}	
	/* &dbus_glib__object_info is provided in the server-bindings.h file */
	/* OBJECT_TYPE_SERVER is the GType of your server object */
	dbus_g_object_type_install_info (TYPE_GEOCLUE_MASTER, &dbus_glib_geoclue_master_object_info);	
	    
}


gboolean geoclue_master_version (GeoclueMaster *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error)
{
   
    return TRUE;
}





gboolean geoclue_master_get_default_position_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error)
{
    guint length =  g_list_length(obj->backends);
    printf("length %d\n",length);
    int i;    
    for(i = 0; i < length; i++)
    {
        char** backend = g_list_nth_data(obj->backends, i);
        printf("Services \n\t%s\n\t%s\n\t%s\n\t%s\n", backend[0], backend[1], backend[2], backend[3]);
    }   


	*OUT_service = gconf_client_get_string(obj->client, "/apps/geoclue/position/defaultservice",NULL);
	*OUT_path = gconf_client_get_string(obj->client, "/apps/geoclue/position/defaultpath",NULL); 
		  
    return TRUE;
}
gboolean geoclue_master_get_all_position_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error)
{
    guint length =  g_list_length(obj->backends);
    printf("length %d\n",length);
    int i;
    for(i = 0; i < length; i++)
    {
        char** backend = g_list_nth_data(obj->backends, i);
        printf("Services \n\t%s\n\t%s\n\t%s\n\t%s\n", backend[0], backend[1], backend[2], backend[3]);
    }    

    
    
    
	*OUT_service = malloc(3 * sizeof(char*));
	*OUT_path = malloc(3 * sizeof(char*));
	(*OUT_service)[0] = strdup("org.foinse_project.geoclue.position.manual");
	(*OUT_path)[0] = strdup("/org/foinse_project/geoclue/position/manual"); 
	(*OUT_service)[1] = strdup("org.foinse_project.geoclue.position.hostip");
	(*OUT_path)[1] = strdup("/org/foinse_project/geoclue/position/hostip"); 	
	(*OUT_service)[2] = NULL;
	(*OUT_path)[2] = NULL;
    
    return TRUE;
}


gboolean geoclue_master_position_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error)
{   
    return TRUE;
}



gboolean geoclue_master_get_default_map_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error)
{   
    return TRUE;
}


gboolean geoclue_master_get_all_map_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error)
{   
    return TRUE;
}


gboolean geoclue_master_map_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error)
{   
    return TRUE;
}


gboolean geoclue_master_get_default_geocode_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error)
{   
    return TRUE;
}


gboolean geoclue_master_get_all_geocode_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error)
{   
    return TRUE;
}


gboolean geoclue_master_geocode_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error)
{   
    return TRUE;
}






int main( int   argc,
          char *argv[] )
{
    
    g_type_init ();
    gconf_init(argc, argv, NULL);
    
    
   
    GMainLoop*  loop = g_main_loop_new(NULL,TRUE);
    
    
      

    GeoclueMaster* obj = NULL; 
  
    obj = GEOCLUE_MASTER(g_type_create_instance (TYPE_GEOCLUE_MASTER));
        



    g_main_loop_run(loop);
    
    g_object_unref(obj);   
    g_main_loop_unref(loop);
    
    
    return 0;
}












