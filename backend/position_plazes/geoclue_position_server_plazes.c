/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2007 Jussi Kukkonen
 * 
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
#include <time.h>

/* for mac address lookup */
#include <stdio.h>

#define WEBSERVICE_API "http://plazes.com/suggestions.xml"

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



/*
 * Private internet connectivity monitoring functions
 */
static void init_net_connection_monitoring (GeocluePosition* server);
static void close_net_connection_monitoring (GeocluePosition* server);

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
static gboolean query_civic_location (char** OUT_country,
                                      char** OUT_region,
                                      char** OUT_locality,
                                      char** OUT_area,
                                      char** OUT_postalcode,
                                      char** OUT_street,
                                      char** OUT_building,
                                      char** OUT_floor,
                                      char** OUT_room,
                                      char** OUT_description,
                                      char** OUT_text,
                                      GError **error);

static void set_current_position (GeocluePosition *server, gdouble lat, gdouble lon);
static void set_civic_location (GeocluePosition *server,
                                char* OUT_country,
                                char* OUT_region,
                                char* OUT_locality,
                                char* OUT_area,
                                char* OUT_postalcode,
                                char* OUT_street,
                                char* OUT_building,
                                char* OUT_floor,
                                char* OUT_room,
                                char* OUT_description,
                                char* OUT_text);



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
    gchar *country, *region, *locality, *area, *postalcode, *street, *building, *floor, *room, *description, *text;

    g_return_if_fail (IS_GEOCLUE_POSITION (user_data));
    /* NOTE: this macro is broken in libconic 0.12
    g_return_if_fail (CON_IC_IS_CONNECTION_EVENT (event));
    */


    GeocluePosition* server = (GeocluePosition*)user_data;

    switch (con_ic_connection_event_get_status (event)) {
        case CON_IC_STATUS_CONNECTED:

            /* try to get a position */
            if (query_position (&lat, &lon, NULL)) {
                set_current_position (server, lat, lon);
            }
            
            if (query_civic_location (&country, &region, &locality, &area,
                                      &postalcode, &street, &building,
                                      &floor, &room, &description, &text,
                                      NULL)) {
                set_civic_location (server,
                                    country, region, locality, area,
                                    postalcode, street, building,
                                    floor, room, description, text);
            }
            break;
            
        case CON_IC_STATUS_DISCONNECTED:
            server->is_current_valid = FALSE;
            server->is_civic_valid = FALSE;
            break;
        default:
            break;
    }
}

static void init_net_connection_monitoring (GeocluePosition* server)
{
    g_return_if_fail (IS_GEOCLUE_POSITION (server));

    /* init dbus connection -- this needs to be done, 
       connection signals do not work otherwise */
    server->dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
    dbus_connection_setup_with_g_main(server->dbus_connection, NULL);
    /* TODO: dbus error handling */

    /* setup the connection signal callback */
    server->net_connection = con_ic_connection_new ();
    g_object_set (server->net_connection, "automatic-connection-events", 
                  TRUE, NULL);
    g_signal_connect (server->net_connection, "connection-event",
                      G_CALLBACK(net_connection_event_cb), server);

    g_debug ("Internet connection event monitoring started");
}

static void close_net_connection_monitoring (GeocluePosition* server)
{
    g_return_if_fail (IS_GEOCLUE_POSITION (server));
    g_object_unref (server->net_connection);
    dbus_connection_disconnect (server->dbus_connection);
    dbus_connection_unref (server->dbus_connection);
}

#else

#define OBSERVING_NET_CONNECTIONS FALSE
/* empty functions for non-libconic platforms */
static void init_net_connection_monitoring (GeocluePosition* server) {}
static void close_net_connection_monitoring (GeocluePosition* server) {}

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
        /* You are not allowed to free something from getenv g_free (proxy_env);*/
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


static gboolean query_civic_location (char** OUT_country,
                                      char** OUT_region,
                                      char** OUT_locality,
                                      char** OUT_area,
                                      char** OUT_postalcode,
                                      char** OUT_street,
                                      char** OUT_building,
                                      char** OUT_floor,
                                      char** OUT_room,
                                      char** OUT_description,
                                      char** OUT_text,
                                      GError **error)
{
    xmlXPathContextPtr xpathCtx;
    gboolean valid = FALSE;
    gchar* query_url;
    gchar* mac_address;

    *OUT_country = NULL;
    *OUT_region = NULL;
    *OUT_locality = NULL;
    *OUT_area = NULL;
    *OUT_postalcode = NULL;
    *OUT_street = NULL;
    *OUT_building = NULL;
    *OUT_floor = NULL;
    *OUT_room = NULL;
    *OUT_description = NULL;
    *OUT_text = NULL;


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
    valid = evaluate_xpath_string (OUT_description, xpathCtx, "//plaze/name") || valid;
    xmlXPathFreeContext(xpathCtx);

    g_debug ("location: %s, %s, %s, %s, %s", *OUT_country, *OUT_locality, *OUT_postalcode, *OUT_street, *OUT_description);

    if (!valid){
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NODATA,
                     "%s does not have civic location for this router MAC address.", WEBSERVICE_API);
        return FALSE;
    }

    return TRUE;
}


static void set_current_position (GeocluePosition *server, gdouble lat, gdouble lon)
{
    if ((lat != server->current_lat) || (lat != server->current_lat)) {
        server->current_lat = lat;
        server->current_lon = lon;
    	time_t ttime;
    	time(&ttime);
        g_signal_emit_by_name(server, "current_position_changed", ttime, lat, lon, 0);
    }

    /* if net connection is monitored, the validity of position can be guaranteed */
    server->is_current_valid = OBSERVING_NET_CONNECTIONS;
}


/* compare strings that are allowed to be null */
static gboolean is_same_string (char* a, char* b)
{
    if ((a != NULL) && (b == NULL)) return FALSE;
    if ((a == NULL) && (b != NULL)) return FALSE;
    if ((a != NULL) && (b != NULL)) {
        if (g_ascii_strcasecmp (a, b) != 0) {
            return FALSE;
        }
    }
    return TRUE;
}


static void set_civic_location (GeocluePosition *server,
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
                                char* text)
{
    if (!is_same_string (server->civic_country, country) ||
        !is_same_string (server->civic_region, region) ||
        !is_same_string (server->civic_locality, locality) ||
        !is_same_string (server->civic_area, area) ||
        !is_same_string (server->civic_postalcode, postalcode) ||
        !is_same_string (server->civic_street, street) ||
        !is_same_string (server->civic_building, building) ||
        !is_same_string (server->civic_floor, floor) ||
        !is_same_string (server->civic_room, room) ||
        !is_same_string (server->civic_description, description) ||
        !is_same_string (server->civic_text, text)) {

        server->civic_country = g_strdup (country);
        server->civic_region = g_strdup (region);
        server->civic_locality = g_strdup (locality);
        server->civic_area = g_strdup (area);
        server->civic_postalcode = g_strdup (postalcode);
        server->civic_street = g_strdup (street);
        server->civic_building = g_strdup (building);
        server->civic_floor = g_strdup (floor);
        server->civic_room = g_strdup (room);
        server->civic_description = g_strdup (description);
        server->civic_text = g_strdup (text);

        geoclue_position_civic_location_changed (server,
                                                 country, region, locality, area,
                                                 postalcode, street, building,
                                                 floor, room, description, text);
    }

    /* if net connection is monitored, the validity of location can be guaranteed */

    server->is_civic_valid = OBSERVING_NET_CONNECTIONS;
}


static void
geoclue_position_init (GeocluePosition *server)
{
	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeocluePositionClass *klass = GEOCLUE_POSITION_GET_CLASS(server);
	guint request_ret;
	
	dbus_g_connection_register_g_object (klass->connection,
			GEOCLUE_POSITION_DBUS_PATH ,
			G_OBJECT (server));

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
	
	init_net_connection_monitoring (server);
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
    *name = "plazes.com";
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
	
    if (server->is_current_valid) 
    {
        *OUT_latitude = server->current_lat;
        *OUT_longitude = server->current_lon;
        return TRUE;
    }
    else if (query_position (OUT_latitude, OUT_longitude, error)) {
        set_current_position (server, *OUT_latitude, *OUT_longitude);
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
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

gboolean geoclue_position_civic_location (GeocluePosition* server,
                                          char** OUT_country,
                                          char** OUT_region,
                                          char** OUT_locality,
                                          char** OUT_area,
                                          char** OUT_postalcode,
                                          char** OUT_street,
                                          char** OUT_building,
                                          char** OUT_floor,
                                          char** OUT_room,
                                          char** OUT_description,
                                          char** OUT_text,
                                          GError** error)
{
    if (server->is_civic_valid) {
        *OUT_country = g_strdup (server->civic_country);
        *OUT_region = g_strdup (server->civic_region);
        *OUT_locality = g_strdup (server->civic_locality);
        *OUT_area = g_strdup (server->civic_area);
        *OUT_postalcode = g_strdup (server->civic_postalcode);
        *OUT_street = g_strdup (server->civic_street);
        *OUT_building = g_strdup (server->civic_building);
        *OUT_floor = g_strdup (server->civic_floor);
        *OUT_room = g_strdup (server->civic_room);
        *OUT_description = g_strdup (server->civic_description);
        *OUT_text = g_strdup (server->civic_text);
        return TRUE;
    } else if (query_civic_location (OUT_country, OUT_region, OUT_locality, OUT_area,
                                     OUT_postalcode, OUT_street, OUT_building,
                                     OUT_floor, OUT_room, OUT_description, OUT_text,
                                     error)) {
        set_civic_location (server,
                            *OUT_country, *OUT_region, *OUT_locality, *OUT_area,
                            *OUT_postalcode, *OUT_street, *OUT_building,
                            *OUT_floor, *OUT_room, *OUT_description, *OUT_text);
        return TRUE;
    } else {
        return FALSE;
    }
}



/* TODO: Is this method sane? We have "GError**" in the call signatures:
   This means calling current_position and checking return value 
   (and reading error->message on FALSE) gives the exact same 
   information as this method... */

gboolean geoclue_position_service_status	 (	GeocluePosition* server,
												gint* OUT_status, 
												char** OUT_string, 
												GError **error)
{
    gdouble temp, temp2, temp3;
    gint time;
    
    geoclue_position_current_position(server, &time, &temp, &temp2, &temp3, error);
    if( temp == -999.99 || temp2 == -999.99)
    {
    	*OUT_status = GEOCLUE_POSITION_NO_SERVICE_AVAILABLE;    
    	*OUT_string = strdup("Cannot Connect to api.plazes.com\n");
    }
    else
    {
    	*OUT_status = GEOCLUE_POSITION_LONGITUDE_AVAILABLE | GEOCLUE_POSITION_LATITUDE_AVAILABLE;    
    	*OUT_string = strdup("Hostip connection is working");
    }   
    return TRUE;  
}


gboolean geoclue_position_shutdown(GeocluePosition *server, GError** error)
{
    close_net_connection_monitoring (server);
    g_main_loop_quit (server->loop);
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
      

    GeocluePosition* server = NULL; 
  
    server = GEOCLUE_POSITION(g_type_create_instance (geoclue_position_get_type()));
    server->loop = g_main_loop_new(NULL,TRUE);
    
    
    g_debug ("Running main loop");
    g_main_loop_run(server->loop);
    
    g_object_unref(server);   
    g_main_loop_unref(server->loop);
    
    xmlCleanupParser();
    
    return(0);
}
