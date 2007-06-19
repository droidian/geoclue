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

#include <geoclue_position_server_hostip.h>
#include <geoclue_position_server_glue.h>
#include <geoclue_position_signal_marshal.h>
#include <dbus/dbus-glib-bindings.h>

#include <libxml/xpath.h>
#include <libsoup/soup.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define HOSTIP_API "http://api.hostip.info/"



G_DEFINE_TYPE(GeocluePosition, geoclue_position, G_TYPE_OBJECT)



enum {
  CURRENT_POSITION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

//Default handler
void geoclue_position_current_position_changed(GeocluePosition* obj, gdouble lat, gdouble lon)
{   
    g_print("Current Position Changed\n");
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

    klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

	signals[CURRENT_POSITION_CHANGED] =
        g_signal_new ("current_position_changed",
                TYPE_GEOCLUE_POSITION,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GeocluePositionClass, current_position_changed),
                NULL, 
                NULL,
                _geoclue_position_VOID__DOUBLE_DOUBLE,
                G_TYPE_NONE, 2 ,G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  
    klass->current_position_changed = geoclue_position_current_position_changed;
 
	if (klass->connection == NULL)
	{
		g_printerr("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}	

	dbus_g_object_type_install_info (TYPE_GEOCLUE_POSITION, &dbus_glib_geoclue_position_object_info);	
    
}


gboolean geoclue_position_version (GeocluePosition *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error)
{
    *OUT_major = 1;
    *OUT_minor = 0;
    *OUT_micro = 0;   
    return TRUE;
}


gboolean geoclue_position_service_provider(GeocluePosition *obj, char** name, GError **error)
{
    *name = "www.hostip.info";
    return TRUE;
}


gchar* geoclue_position_get_hostip_xml()
{
    SoupSession *session;
    SoupMessage *msg;
    const char *cafile = NULL;
    SoupUri *proxy = NULL;    
    gchar *proxy_env;
    gchar *result; 

    proxy_env = getenv ("http_proxy");
    if (proxy_env != NULL) {  
        proxy = soup_uri_new (proxy_env);
        session = soup_session_sync_new_with_options (
            SOUP_SESSION_SSL_CA_FILE, cafile,
            SOUP_SESSION_PROXY_URI, proxy,
            NULL);
        g_free (proxy_env);
    }else{
        session = soup_session_sync_new ();
    }
    
    msg = soup_message_new ("GET", HOSTIP_API);
    soup_session_send_message(session, msg);
    result = g_strdup (msg->response.body);
   
    return result;
}

gboolean geoclue_position_current_position(GeocluePosition *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error )
{
    gboolean success = FALSE;
    gchar *xml;
    gchar *value;
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;   
    xmlXPathObjectPtr xpathObj; 

    *OUT_latitude = -999.99;
    *OUT_longitude = -999.99;

    xml = geoclue_position_get_hostip_xml();
    doc = xmlParseDoc (xml);
    g_free(xml);

    if (doc) {
        xpathCtx = xmlXPathNewContext(doc);
        if (xpathCtx) {
            // Register gml namespace and evaluate xpath
            xmlXPathRegisterNs (xpathCtx, "gml", "http://www.opengis.net/gml");
            xpathObj = xmlXPathEvalExpression ("//gml:coordinates", xpathCtx);
            
            if(!xpathObj) {
                // hostip probably does not have coordinates for this IP

            } else {
                if (xpathObj->nodesetval->nodeNr >= 1){
                    value = xpathObj->nodesetval->nodeTab[0]->children->content;
                    
                    printf ("%s\n", value);
                    // get first child (text node) of the only node in the nodeset
                    sscanf (value, "%lf,%lf", OUT_longitude , OUT_latitude);
                    g_free (value);
                    success = TRUE;
                }
                xmlXPathFreeObject(xpathObj);
            }
            xmlXPathFreeContext(xpathCtx);
        }
	    xmlFreeDoc(doc);
    }        

    return success;
}

gboolean geoclue_position_current_position_error(GeocluePosition *obj, gdouble* OUT_latitude_error, gdouble* OUT_longitude_error, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_current_altitude(GeocluePosition *obj, gdouble* OUT_altitude, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_current_velocity(GeocluePosition *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_current_time(GeocluePosition *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_satellites_in_view(GeocluePosition *obj, GArray** OUT_prn_numbers, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_satellites_data(GeocluePosition *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_sun_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_sun_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_moon_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}

gboolean geoclue_position_moon_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    return FALSE;
}



gboolean geoclue_position_service_available(GeocluePosition *obj, gboolean* OUT_available, char** OUT_reason, GError** error)
{
    gdouble temp, temp2;
    
    geoclue_position_current_position(obj, &temp, &temp2, error);
    if( temp == -999.99 || temp2 == -999.99)
    {
        *OUT_available = FALSE;
        *OUT_reason = "Cannot Connect to api.hostip.info\n";
    }
    else
    {
        *OUT_available = TRUE;
    }   
    return TRUE;  
}

gboolean geoclue_position_shutdown(GeocluePosition *obj, GError** error)
{
    g_main_loop_quit (obj->loop);
    return TRUE;
}






int main(int argc, char **argv) 
{
    g_type_init ();
    g_thread_init (NULL);


    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    xmlInitParser();
    LIBXML_TEST_VERSION
      

    GeocluePosition* obj = NULL; 
  
    obj = GEOCLUE_POSITION(g_type_create_instance (geoclue_position_get_type()));
    obj->loop = g_main_loop_new(NULL,TRUE);
    
    
    g_main_loop_run(obj->loop);
    
    g_object_unref(obj);   
    g_main_loop_unref(obj->loop);
    
    xmlCleanupParser();
    
    return(0);
}
