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
#include <geoclue_geocode_server_yahoo.h>
#include <geoclue_geocode_server_glue.h>
#include <dbus/dbus-glib-bindings.h>



#include <libxml/xmlreader.h>
#include <libsoup/soup.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>



#define GEOCLUE_GEOCODE_MIN_HEIGHT 100
#define GEOCLUE_GEOCODE_MAX_HEIGHT 2000
#define GEOCLUE_GEOCODE_MIN_WIDTH 100
#define GEOCLUE_GEOCODE_MAX_WIDTH 3000
#define GEOCLUE_GEOCODE_MIN_ZOOM 1
#define GEOCLUE_GEOCODE_MAX_ZOOM 12

#define YAHOO_LAT "Latitude"
#define YAHOO_LON "Longitude"
#define YAHOO_ADDRESS "Address"
#define YAHOO_CITY "City"
#define YAHOO_STATE "State"
#define YAHOO_ZIP "Zip"
#define YAHOO_COUNTRY "Country"



G_DEFINE_TYPE(GeoclueGeocode, geoclue_geocode, G_TYPE_OBJECT)


/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];



static void
geoclue_geocode_init (GeoclueGeocode *obj)
{


	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeoclueGeocodeClass *klass = GEOCLUE_GEOCODE_GET_CLASS(obj);
	guint request_ret;
	
	/* Register DBUS path */
	dbus_g_connection_register_g_object (klass->connection,
			GEOCLUE_GEOCODE_DBUS_PATH ,
			G_OBJECT (obj));


	/* Register the service name, the constant here are defined in dbus-glib-bindings.h */
	driver_proxy = dbus_g_proxy_new_for_name (klass->connection,
			DBUS_SERVICE_DBUS,
			DBUS_PATH_DBUS,
			DBUS_INTERFACE_DBUS);


	if(!org_freedesktop_DBus_request_name (driver_proxy,
			GEOCLUE_GEOCODE_DBUS_SERVICE,
			0, &request_ret,    /* See tutorial for more infos about these */
			&error))
	{
		g_printerr("Unable to register geogeocode service: %s", error->message);
		g_error_free (error);
        exit(1);
	}	

if (request_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
{ 
    g_printerr("Yahoo geocodes service already running!\n");
}

  
    g_print("registered geocodeping interface \n");

}



static void
geoclue_geocode_class_init (GeoclueGeocodeClass *klass)
{
	GError *error = NULL;

    klass->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
    
    
	if (klass->connection == NULL)
	{
		g_printerr("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}	
	/* &dbus_glib__object_info is provided in the server-bindings.h file */
	/* OBJECT_TYPE_SERVER is the GType of your server object */
	dbus_g_object_type_install_info (TYPE_GEOCLUE_GEOCODE, &dbus_glib_geoclue_geocode_object_info);	
    
}


gboolean geoclue_geocode_version (GeoclueGeocode *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error)
{
        printf("Yahoo!!!\n");
    *OUT_major = 1;
    *OUT_minor = 0;
    *OUT_micro = 0;
    
    return TRUE;
}


gboolean geoclue_geocode_service_provider(GeoclueGeocode *obj, char** name, GError **error)
{
    *name = "Yahoo Geocodes";
    printf("Yahoo!!!\n");
    return TRUE;
}
gboolean geoclue_geocode_to_lat_lon (GeoclueGeocode *obj, const char * IN_street, const char * IN_city, const char * IN_state, const char * IN_zip, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error )
{

    SoupSession *session;
    SoupMessage *msg;
    const char *cafile = NULL;
    SoupUri *proxy = NULL;
    
    char* proxy_env;
    
    proxy_env = getenv ("http_proxy");
    
    printf("found proxy %s:end\n", proxy_env); 
        
    
    if (proxy_env != NULL) {  
        printf("added proxy %s\n", proxy_env); 
        proxy = soup_uri_new (proxy_env);
        session = soup_session_sync_new_with_options (
            SOUP_SESSION_SSL_CA_FILE, cafile,
            SOUP_SESSION_PROXY_URI, proxy,
            NULL);
    }
    else
    {
        session = soup_session_sync_new ();
           
    }
    
    
    char url[5000];

    snprintf(url, 5000,"http://api.local.yahoo.com/MapsService/V1/geocode?appid=YahooDemo&street=%s&city=%s&state=%s&zip=&%s\n",IN_street, IN_city,  IN_state, IN_zip);
    
    g_print(url);
    
    msg = soup_message_new ("GET", url);
    soup_session_send_message(session, msg);

    char *name, *value;
    xmlTextReaderPtr reader;
    int ret;

    
    reader = xmlReaderForMemory (msg->response.body, 
                         msg->response.length, 
                         NULL, 
                         NULL, 
                         0);
    
    

        ret = xmlTextReaderRead(reader);
        
        //FIXME: super hack because I don't know how to use the XML libraries.  This just works for now
        while (ret == 1) {
            
                name = (char*)xmlTextReaderConstName(reader);
                
                 printf("%s\n", name);

                   if (!strcmp(name,YAHOO_LAT))
                   {
                        //read next and grab text value.   
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                        
                        value = (char*)xmlTextReaderConstValue(reader);
                        printf("matched latitude %s %s\n", name, value);
                        sscanf(value, "%lf", OUT_latitude);   
                        
                        //skip closing tag
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                              
                   }
                   if (!strcmp(name,YAHOO_LON))
                   {
                        //read next and grab text value.  
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                        
                        value = (char*)xmlTextReaderConstValue(reader);
                        printf("matched longitude %s %s\n", name, value);
                        sscanf(value, "%lf", OUT_longitude);  
                          
                        //skip closing tag
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                           
                   }                
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);





    
    
    
    printf("Yahoo!!!\n");
    return TRUE;
}


gboolean geoclue_geocode_free_text_to_lat_lon (GeoclueGeocode *obj, const char * IN_free_text, gdouble* OUT_latitude, gdouble* OUT_longitude, gint* OUT_return_code, GError **error )
{
   SoupSession *session;
    SoupMessage *msg;
    const char *cafile = NULL;
    SoupUri *proxy = NULL;
    
    
    char* proxy_env;
    
    proxy_env = getenv ("http_proxy");
    
    printf("found proxy %s:end\n", proxy_env); 
        
    
    if (proxy_env != NULL) {  
        printf("added proxy %s\n", proxy_env); 
        proxy = soup_uri_new (proxy_env);
        session = soup_session_sync_new_with_options (
            SOUP_SESSION_SSL_CA_FILE, cafile,
            SOUP_SESSION_PROXY_URI, proxy,
            NULL);
    }
    else
    {
        session = soup_session_sync_new ();
           
    }
    
    
    char url[5000];

    snprintf(url, 5000,"http://api.local.yahoo.com/MapsService/V1/geocode?appid=YahooDemo&location=%s\n",IN_free_text);
    
    g_print(url);
    
    msg = soup_message_new ("GET", url);
    soup_session_send_message(session, msg);

    char *name, *value;
    xmlTextReaderPtr reader;
    int ret;
    
    
    reader = xmlReaderForMemory (msg->response.body, 
                         msg->response.length, 
                         NULL, 
                         NULL, 
                         0);
    
    
    
        ret = xmlTextReaderRead(reader);
        
        //FIXME: super hack because I don't know how to use the XML libraries.  This just works for now
        while (ret == 1) {
            
                name = (char*)xmlTextReaderConstName(reader);
                
                 printf("%s\n", name);
                    
                    if (!strcmp(name,YAHOO_LAT))
                    {
                        //read next and grab text value.   For some reason there is two so do it twice
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                        
                        value = (char*)xmlTextReaderConstValue(reader);
                        printf("matched latitude %s %s\n", name, value);
                        sscanf(value, "%lf", OUT_latitude);   
                        
                        //skip closing tag
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                              
                    }
                    if (!strcmp(name,YAHOO_LON))
                    {
                        //read next and grab text value.   For some reason there is two so do it twice
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                        
                        value = (char*)xmlTextReaderConstValue(reader);
                        printf("matched longitude %s %s\n", name, value);
                        sscanf(value, "%lf", OUT_longitude);  
                          
                        //skip closing tag
                        ret = xmlTextReaderRead(reader);
                        name = (char*)xmlTextReaderConstName(reader);
                           
                    }                
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
    
    
    
    printf("Yahoo!!!\n");
    return TRUE;
}

gboolean geoclue_geocode_lat_lon_to_address(GeoclueGeocode *obj, gdouble IN_latitude, gdouble IN_longitude, char ** OUT_street, char ** OUT_city, char ** OUT_state, char ** OUT_zip, gint* OUT_return_code, GError **error )
{
    *OUT_street = strdup("Unknown");
    *OUT_city = strdup("Unknown");
    *OUT_state = strdup("Unknown");
    *OUT_zip = strdup("Unknown");
    
    printf("Yahoo!!!\n");
    return TRUE;
}



gboolean geoclue_geocode_service_available(GeoclueGeocode *obj, gboolean* OUT_available, char** OUT_reason, GError** error)
{
    return TRUE;  
}


gboolean geoclue_geocode_shutdown(GeoclueGeocode *obj, GError** error)
{
    g_main_loop_quit (obj->loop);
    return TRUE;  
}





int main( int   argc,
          char *argv[] )
{
    g_type_init ();
    g_thread_init (NULL);
    
    
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION
      

    GeoclueGeocode* obj = NULL; 
  
    obj = GEOCLUE_GEOCODE(g_type_create_instance (geoclue_geocode_get_type()));

    obj->loop = g_main_loop_new(NULL,TRUE);
            
    
    g_main_loop_run(obj->loop);
    
    g_object_unref(obj);   
    g_main_loop_unref(obj->loop);
    
    
    return 0;
}








