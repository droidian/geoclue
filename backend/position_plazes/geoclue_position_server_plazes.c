/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2007 Jussi Kukkonen
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
 * License along with this library; if not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "geoclue_position_server_plazes.h"
#include "../geoclue_position_error.h"
#include <geoclue_position_server_glue.h>
#include <geoclue_position_signal_marshal.h>

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>

#ifdef HAVE_LIBCONIC
#include <conicconnectionevent.h>
#endif

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libsoup/soup.h>
#include <stdlib.h>

/* for mac address lookup */
#include <stdio.h>

#define WEBSERVICE_API "http://plazes.com/suggestions.xml"

G_DEFINE_TYPE(GeocluePosition, geoclue_position, G_TYPE_OBJECT)

enum {
  CURRENT_POSITION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* Default handler */
void geoclue_position_current_position_changed(GeocluePosition* obj, gdouble lat, gdouble lon)
{
    g_print("Current Position Changed\n");
}



/*
 * Private internet connectivity monitoring functions
 */
static void init_net_connection_monitoring (GeocluePosition* obj);
static void close_net_connection_monitoring (GeocluePosition* obj);

/*
 * Private helper functions for querying the web service and parsing the answer 
 * TODO: These could be moved to another file...
 */
static gboolean get_mac_address (gchar** mac);
static gboolean get_webservice_xml (xmlChar **xml, gchar* url, GError** error);
static xmlXPathContextPtr get_xpath_context (gchar* url, GError** error);
static gboolean evaluate_xpath_string (gchar** OUT_string, xmlXPathContextPtr xpathCtx, gchar* expr);
static gboolean evaluate_xpath_number (gdouble* OUT_double, xmlXPathContextPtr xpathCtx, gchar* expr);
static gboolean query_position (gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error);

static void set_current_position (GeocluePosition *obj, gdouble lat, gdouble lon);



/*
 *  Define the following for both libconic-enabled and other platforms:
 *    - OBSERVING_NET_CONNECTIONS
 *    - init_net_connection_monitoring ()
 *    - close_net_connection_monitoring ()
 */
#ifdef HAVE_LIBCONIC

#define OBSERVING_NET_CONNECTIONS TRUE

/* callback for libconic connection events */
static void net_connection_event_cb (ConIcConnection *connection, 
                                     ConIcConnectionEvent *event,
                                     gpointer user_data)
{
    gdouble lat, lon;

    g_return_if_fail (IS_GEOCLUE_POSITION (user_data));
    /* NOTE: this macro is broken in libconic 0.12
    g_return_if_fail (CON_IC_IS_CONNECTION_EVENT (event));
    */


    GeocluePosition* obj = (GeocluePosition*)user_data;

    switch (con_ic_connection_event_get_status (event)) {
        case CON_IC_STATUS_CONNECTED:

            /* try to get a position */
            if (query_position (&lat, &lon, NULL)) {
                set_current_position (obj, lat, lon);
            }
            break;
        case CON_IC_STATUS_DISCONNECTED:
            obj->is_current_valid = FALSE;
            break;
        default:
            break;
    }
}

static void init_net_connection_monitoring (GeocluePosition* obj)
{
    g_return_if_fail (IS_GEOCLUE_POSITION (obj));

    /* init dbus connection -- this needs to be done, 
       connection signals do not work otherwise */
    obj->dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
    dbus_connection_setup_with_g_main(obj->dbus_connection, NULL);
    /* TODO: dbus error handling */

    /* setup the connection signal callback */
    obj->net_connection = con_ic_connection_new ();
    g_object_set (obj->net_connection, "automatic-connection-events", 
                  TRUE, NULL);
    g_signal_connect (obj->net_connection, "connection-event",
                      G_CALLBACK(net_connection_event_cb), obj);

    g_debug ("Internet connection event monitoring started");
}

static void close_net_connection_monitoring (GeocluePosition* obj)
{
    g_return_if_fail (IS_GEOCLUE_POSITION (obj));
    g_object_unref (obj->net_connection);
    dbus_connection_disconnect (obj->dbus_connection);
    dbus_connection_unref (obj->dbus_connection);
}

#else

#define OBSERVING_NET_CONNECTIONS FALSE
/* empty functions for non-libconic platforms */
static void init_net_connection_monitoring (GeocluePosition* obj) {}
static void close_net_connection_monitoring (GeocluePosition* obj) {}

#endif



static gboolean get_mac_address (gchar** mac)
{
    /* this is fairly ugly, but it seems there is no easy ioctl-based way to get 
       mac address of the router. This implementation expects the system to have
       netstat, grep, awk, tr and /proc/net/arp.
     */
    
    FILE *in;
    gint mac_len = sizeof (gchar) * 18;
    
    if (!(in = popen ("ROUTER_IP=`netstat -rn | grep '^0.0.0.0 ' | awk '{ print $2 }'` && grep \"^$ROUTER_IP \" /proc/net/arp | awk '{print $4}' | tr A-Z a-z", "r"))) {
        g_debug ("mac address lookup failed");
        return FALSE;
    }
    
    *mac = g_malloc (mac_len);
    if (fgets (*mac, mac_len, in) == NULL) {
        g_debug ("mac address lookup returned nothing");
        g_free (*mac);
        return FALSE;
    }
    pclose(in);
    return TRUE;
}



static gboolean get_webservice_xml (xmlChar **xml, gchar* url, GError** error)
{
    SoupSession *session;
    SoupMessage *msg;
    const char *cafile = NULL;
    SoupUri *proxy = NULL;
    gchar *proxy_env;
    
    proxy_env = getenv ("http_proxy");
    if (proxy_env != NULL) {  
        proxy = soup_uri_new (proxy_env);
        session = soup_session_sync_new_with_options (
            SOUP_SESSION_SSL_CA_FILE, cafile,
            SOUP_SESSION_PROXY_URI, proxy,
            NULL);
        soup_uri_free (proxy);
        g_free (proxy_env);
    } else {
        session = soup_session_sync_new ();
    }
    
    if (!session) {
        g_debug ("no libsoup session");
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_FAILED,
                     "libsoup session creation failed");
        return FALSE;
    }    
    
    msg = soup_message_new ("GET", url);
    soup_session_send_message (session, msg);
    if (msg->response.length == 0) {
        g_debug ("no xml from libsoup, a connection problem perhaps?");
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NOSERVICE,
                     "No position data was received from %s.", url);
        return FALSE;
    }
    
    *xml = (xmlChar*)g_strndup (msg->response.body, msg->response.length);
    return TRUE;
}


/* builds an xpath context from xml.
   the context can be used in xpath evals */
static xmlXPathContextPtr get_xpath_context (gchar* url, GError** error)
{
    xmlChar* xml = NULL;
    xmlDocPtr doc; /*should this be freed or does context take care of it?*/
    xmlXPathContextPtr xpathCtx;
    
    g_debug ("Getting xml from %s", url);
    if (!get_webservice_xml (&xml, url, error)) {
        return NULL;
    }
    
    doc = xmlParseDoc (xml);
    if (!doc) {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_MALFORMEDDATA,
                     "Position data from %s could not be parsed:\n\n%s", WEBSERVICE_API, xml);
        g_free (xml);
        return NULL;
    }
    
    xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_FAILED,
                     "XPath context could not be created.");
        g_free (xml);
        return NULL;
    }
    
    g_free(xml);
    return xpathCtx;
}


static gboolean evaluate_xpath_string (gchar** OUT_string, xmlXPathContextPtr xpathCtx, gchar* expr)
{
    gboolean retval= FALSE;
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression ((xmlChar*)expr, xpathCtx);
    if (xpathObj) {
        if (xpathObj->nodesetval && !xmlXPathNodeSetIsEmpty (xpathObj->nodesetval)) {
            *OUT_string = g_strdup ((char*)xmlXPathCastNodeSetToString (xpathObj->nodesetval));
            retval = TRUE;
        }
        xmlXPathFreeObject (xpathObj);
    }
    return retval;
}


static gboolean evaluate_xpath_number (gdouble* OUT_double, xmlXPathContextPtr xpathCtx, gchar* expr)
{
    gboolean retval= FALSE;
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression ((xmlChar*)expr, xpathCtx);
    if (xpathObj) {
        if (xpathObj->nodesetval && !xmlXPathNodeSetIsEmpty (xpathObj->nodesetval)) {
            *OUT_double = xmlXPathCastNodeSetToNumber(xpathObj->nodesetval);
            retval = TRUE;
        }
        xmlXPathFreeObject (xpathObj);
    }
    return retval;
}


static gboolean query_position (gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error)
{
    xmlXPathContextPtr xpathCtx;
    gboolean valid = TRUE;
    gchar* query_url;
    gchar* mac_address;
    
    if (!get_mac_address (&mac_address)) {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_FAILED,
                     "Could not find out MAC address of the router");
        return FALSE;
    }
    query_url = g_strdup_printf("%s?mac_address=%s", WEBSERVICE_API, mac_address);
    g_free (mac_address);
    
    if (!(xpathCtx = get_xpath_context (query_url, error))) {
        return FALSE;
    }
    
    *OUT_latitude = -999.99;
    *OUT_longitude = -999.99;
    
    valid = evaluate_xpath_number (OUT_latitude, xpathCtx, "//plaze/latitude") && valid;
    valid = evaluate_xpath_number (OUT_longitude, xpathCtx, "//plaze/longitude") && valid;
    xmlXPathFreeContext(xpathCtx);
    
    if (!valid){
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NODATA,
                     "%s does not have position data for this router MAC address.", WEBSERVICE_API);
        return FALSE;
    }
    
    g_debug ("position acquired: %f, %f", *OUT_latitude, *OUT_longitude);
    return TRUE;
}

static void set_current_position (GeocluePosition *obj, gdouble lat, gdouble lon)
{
    if ((lat != obj->current_lat) || (lat != obj->current_lat)) {
        obj->current_lat = lat;
        obj->current_lon = lon;
        geoclue_position_current_position_changed (obj, lat, lon);
    }

    /* if net connection is monitored, the validity of position can be guaranteed */
    obj->is_current_valid = OBSERVING_NET_CONNECTIONS;
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
	
	init_net_connection_monitoring (obj);
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
    *name = "plazes.com";
    return TRUE;
}


gboolean geoclue_position_current_position (GeocluePosition *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error)
{
    if (obj->is_current_valid) 
    {
        *OUT_latitude = obj->current_lat;
        *OUT_longitude = obj->current_lon;
        return TRUE;
    }
    else if (query_position (OUT_latitude, OUT_longitude, error)) {
        set_current_position (obj, *OUT_latitude, *OUT_longitude);
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}

gboolean geoclue_position_current_position_error(GeocluePosition *obj, gdouble* OUT_latitude_error, gdouble* OUT_longitude_error, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_current_altitude(GeocluePosition *obj, gdouble* OUT_altitude, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_current_velocity(GeocluePosition *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_current_time(GeocluePosition *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_satellites_in_view(GeocluePosition *obj, GArray** OUT_prn_numbers, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_satellites_data(GeocluePosition *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_sun_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_sun_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_moon_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_moon_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}


gboolean geoclue_position_civic_location (GeocluePosition* obj,
                                          char** OUT_country,
                                          char** OUT_region,
                                          char** OUT_locality,
                                          char** OUT_area,
                                          char** OUT_postalcode,
                                          char** OUT_street,
                                          char** OUT_building,
                                          char** OUT_floor,
                                          char** OUT_room,
                                          char** OUT_text,
                                          GError** error)
{
    xmlXPathContextPtr xpathCtx;
    gboolean valid = FALSE; /* consider location valid if any data type is present */
    gchar* query_url;
    gchar* mac_address;

    if (!get_mac_address (&mac_address)) {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_FAILED,
                     "Could not find out MAC address of the router");
        return FALSE;
    }
    query_url = g_strdup_printf("%s?mac_address=%s", WEBSERVICE_API, mac_address);
    g_free (mac_address);

    if (!(xpathCtx = get_xpath_context (query_url, error))) {
        return FALSE;
    }
    
    valid = evaluate_xpath_string (OUT_country, xpathCtx, "//plaze/country") || valid;
    valid = evaluate_xpath_string (OUT_locality, xpathCtx, "//plaze/city") || valid;
    valid = evaluate_xpath_string (OUT_postalcode, xpathCtx, "//plaze/zip_code") || valid;
    valid = evaluate_xpath_string (OUT_street, xpathCtx, "//plaze/address") || valid;
    valid = evaluate_xpath_string (OUT_text, xpathCtx, "//plaze/name") || valid;
    xmlXPathFreeContext(xpathCtx);

    g_debug ("location: %s, %s, %s, %s, %s", *OUT_country, *OUT_locality, *OUT_postalcode, *OUT_street, *OUT_text);

    if (!valid){
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NODATA,
                     "%s does not have civic location for this router MAC address.", WEBSERVICE_API);
        return FALSE;
    }

    return TRUE;     
}


gboolean geoclue_position_civic_location_supports (GeocluePosition* obj,
                                                   gboolean* OUT_country,
                                                   gboolean* OUT_region,
                                                   gboolean* OUT_locality,
                                                   gboolean* OUT_area,
                                                   gboolean* OUT_postalcode,
                                                   gboolean* OUT_street,
                                                   gboolean* OUT_building,
                                                   gboolean* OUT_floor,
                                                   gboolean* OUT_room,
                                                   gboolean* OUT_text,
                                                   GError** error)
{

/* TODO: update this data to reflect reality */
    *OUT_country = TRUE;
    *OUT_region = FALSE;
    *OUT_locality = TRUE;
    *OUT_area = FALSE;
    *OUT_postalcode = TRUE;
    *OUT_street = TRUE;
    *OUT_building = FALSE;
    *OUT_floor = FALSE;
    *OUT_room = FALSE;
    *OUT_text = TRUE;
    return TRUE;
}


/* TODO: Is this method sane? We have "GError**" in the call signatures:
   This means calling current_position and checking return value 
   (and reading error->message on FALSE) gives the exact same 
   information as this method... */
   
gboolean geoclue_position_service_available(GeocluePosition *obj, gboolean* OUT_available, char** OUT_reason, GError** error)
{
    gdouble temp, temp2;
    
    geoclue_position_current_position(obj, &temp, &temp2, error);
    if( temp == -999.99 || temp2 == -999.99)
    {
        *OUT_available = FALSE;
        *OUT_reason = "Cannot Connect to api.plazes.com\n";
    }
    else
    {
        *OUT_available = TRUE;
    }   
    return TRUE;  
}

gboolean geoclue_position_shutdown(GeocluePosition *obj, GError** error)
{
    close_net_connection_monitoring (obj);
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
