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

#include "geoclue_position_server_manual.h"
#include "../geoclue_position_error.h"
#include <geoclue_position_server_glue.h>
#include <geoclue_position_signal_marshal.h>

#include <string.h>

#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>
#include <gconf/gconf-client.h>
#include <geoclue/geocode.h>


#define GCONF_POS_MANUAL "/apps/geoclue/position/manual"
#define GCONF_POS_MANUAL_COUNTRY GCONF_POS_MANUAL"/country"
#define GCONF_POS_MANUAL_REGION GCONF_POS_MANUAL"/region"
#define GCONF_POS_MANUAL_LOCALITY GCONF_POS_MANUAL"/locality"
#define GCONF_POS_MANUAL_AREA GCONF_POS_MANUAL"/area"
#define GCONF_POS_MANUAL_POSTALCODE GCONF_POS_MANUAL"/postalcode"
#define GCONF_POS_MANUAL_STREET GCONF_POS_MANUAL"/street"
#define GCONF_POS_MANUAL_BUILDING GCONF_POS_MANUAL"/building"
#define GCONF_POS_MANUAL_FLOOR GCONF_POS_MANUAL"/floor"
#define GCONF_POS_MANUAL_ROOM GCONF_POS_MANUAL"/room"
#define GCONF_POS_MANUAL_DESCRIPTION GCONF_POS_MANUAL"/description"
#define GCONF_POS_MANUAL_TEXT GCONF_POS_MANUAL"/text"
#define GCONF_POS_MANUAL_LAT GCONF_POS_MANUAL"/latitude"
#define GCONF_POS_MANUAL_LON GCONF_POS_MANUAL"/longitude"
#define GCONF_POS_MANUAL_TIMESTAMP GCONF_POS_MANUAL"/timestamp"
#define GCONF_POS_MANUAL_VALID_UNTIL GCONF_POS_MANUAL"/valid-until"
        
G_DEFINE_TYPE(GeocluePosition, geoclue_position, G_TYPE_OBJECT)

enum {
    CURRENT_POSITION_CHANGED,
    SERVICE_STATUS_CHANGED,
    CIVIC_LOCATION_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* Default handlers */
static void geoclue_position_current_position_changed (GeocluePosition* server, 
                                                       gdouble timestamp, 
                                                       gdouble lat, 
                                                       gdouble lon,
                                                       gdouble altitude)
{
    g_print("Current Position Changed\n");
}

static void geoclue_position_civic_location_changed (GeocluePosition* server,
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
    g_print("Civic location Changed\n");
}

static void geoclue_position_service_status_changed (GeocluePosition* server,
                                                     gint status,
                                                     char* user_message)
{
    g_print("service_status_changed\n");
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

/* returns true on successful reading of gconf value that is 
   longer than zero). If key does not exist, sets string to "" */
static gboolean read_gconf_string (GeocluePosition *obj, const gchar *gconf_key, gchar **string)
{
    *string = gconf_client_get_string (obj->gconf, gconf_key, NULL);
    if (*string == NULL){
        *string = g_strdup("");
        return FALSE;
    }else if (strlen(*string) == 0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*returns true on successful reading of gconf key */
static gboolean read_gconf_float (GeocluePosition *obj, const gchar *gconf_key, gdouble *value)
{
    /* gconf_client_get_float returns 0.0 if key is unset 
       so need to work around... */
    GConfValue *gconfval = gconf_client_get (obj->gconf, gconf_key, NULL);
    if (!gconfval) {
        return FALSE;
    }
    g_free (gconfval);

    GError *err = NULL;
    *value = gconf_client_get_float (obj->gconf, gconf_key, &err);
    if (err) {
        g_clear_error (&err);
        return FALSE;
    } else {
        return TRUE;
    }
}

static gboolean is_time_in_future (gint valid_until)
{
    GTimeVal curr_time;
    g_get_current_time (&curr_time);
    return curr_time.tv_sec <= valid_until;
}

static void gconf_manual_position_changed (GConfClient* client,
                                           guint cnxn_id,
                                           GConfEntry* entry,
                                           gpointer data)
{
    gdouble latitude, longitude;
    gint valid_until;
    gchar *country, *region, *locality, *area, *postalcode, *street,
          *building, *floor, *room, *description, *text;
    gint ret_code;
    gboolean civic_available = FALSE;
    gboolean coords_available = TRUE;
    
    GeocluePosition *obj = (GeocluePosition*)data;
    g_return_if_fail (obj);

    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_COUNTRY, &country) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_REGION, &region) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_LOCALITY, &locality) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_AREA, &area) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_POSTALCODE, &postalcode) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_STREET, &street) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_BUILDING, &building) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_FLOOR, &floor) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_ROOM, &room) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_DESCRIPTION, &description) || civic_available;
    civic_available = read_gconf_string (obj, GCONF_POS_MANUAL_TEXT, &text) || civic_available;

    coords_available = read_gconf_float (obj, GCONF_POS_MANUAL_LAT, &latitude) && coords_available;
    coords_available = read_gconf_float (obj, GCONF_POS_MANUAL_LON, &longitude) && coords_available;

    valid_until = gconf_client_get_int (obj->gconf, GCONF_POS_MANUAL_VALID_UNTIL, NULL);

    obj->current_position_set = coords_available;
    obj->civic_location_set = civic_available;

    if (is_time_in_future (valid_until)) {
        /* get coordinates from geocoding if necessary  */
        if (civic_available && !coords_available) {
            if (geoclue_geocode_init () == GEOCLUE_GEOCODE_SUCCESS) {
                gchar* freetext = g_strdup_printf ("%s%s%s%s%s%s%s",
                                                   street,
                                                   (strlen (street) > 0) ? ", " : "",
                                                   postalcode,
                                                   (strlen (postalcode) > 0) ? ", " : "",
                                                   locality,
                                                   (strlen (locality) > 0) ? ", " : "",
                                                   country);
                g_debug ("geocoding with string '%s'", freetext);
                if (strlen (freetext) > 0) {
                    /*should be using geocode_to_lat_lon here... for some reason yahoo api doesn't seem to
                    geocode some addresses with that */
                    if (geoclue_geocode_free_text_to_lat_lon (freetext, &latitude, &longitude, &ret_code) == GEOCLUE_GEOCODE_SUCCESS) {
                        g_debug ("geocoded to lat/lon: %f, %f", latitude, longitude);
                        obj->current_position_set = TRUE;
                    }
                }
            }
        }

        /* get address from geocoding if no civic location data was provided */
        if (coords_available && !civic_available) {
            if (geoclue_geocode_init () == GEOCLUE_GEOCODE_SUCCESS) {
                g_debug ("geocoding with lat/lon: %f, %f", latitude, longitude);
                if (geoclue_geocode_lat_lon_to_address (latitude, longitude, &street, &locality, &region, &postalcode, &ret_code) == GEOCLUE_GEOCODE_SUCCESS) {
                    g_debug ("geocoded to address: %s, %s, %s, %s", street, locality, region, postalcode);
                    obj->civic_location_set = TRUE;
                }
            }
        }

    }

    /* save the values */
    obj->valid_until = valid_until;
    obj->timestamp = gconf_client_get_int (obj->gconf, GCONF_POS_MANUAL_TIMESTAMP, NULL);

    if (!is_same_string (obj->country, country) ||
        !is_same_string (obj->region, region) ||
        !is_same_string (obj->locality, locality) ||
        !is_same_string (obj->area, area) ||
        !is_same_string (obj->postalcode, postalcode) ||
        !is_same_string (obj->street, street) ||
        !is_same_string (obj->building, building) ||
        !is_same_string (obj->floor, floor) ||
        !is_same_string (obj->room, room) ||
        !is_same_string (obj->description, description) ||
        !is_same_string (obj->text, text)) {
        
        obj->country = country; 
        obj->region = region;
        obj->locality = locality;
        obj->area = area;
        obj->postalcode = postalcode;
        obj->street = street;
        obj->building = building;
        obj->floor = floor;
        obj->room = room;
        obj->description = description;
        obj->text = text;

        g_signal_emit_by_name (obj, "civic_location_changed",  
                               obj->country, obj->region, obj->locality, obj->area, obj->postalcode, obj->street, 
                               obj->building, obj->floor, obj->room, obj->description, obj->text);
    }

    if (obj->latitude != latitude  || obj->longitude != longitude) {        

        obj->latitude = latitude;
        obj->longitude = longitude;
        g_signal_emit_by_name (obj, "current_position_changed", obj->latitude, obj->longitude);
    }
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
                                           0, &request_ret, &error))
    {
        g_printerr("Unable to register geoclue service: %s", error->message);
        g_error_free (error);
    }

    server->gconf = gconf_client_get_default ();
    /*set gconf signal handlers*/
    gconf_client_add_dir (server->gconf,
                          GCONF_POS_MANUAL,
                          GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
    gconf_client_notify_add (server->gconf,
                             GCONF_POS_MANUAL,
                             (GConfClientNotifyFunc)gconf_manual_position_changed,
                             (gpointer)server,
                             NULL, NULL);

    /* run callback manually to get info from gconf */
    gconf_manual_position_changed (server->gconf, 0, NULL, server);
}


static void
geoclue_position_class_init (GeocluePositionClass *klass)
{
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
    klass->service_status_changed = geoclue_position_service_status_changed;
    klass->civic_location_changed = geoclue_position_civic_location_changed;
 

    GError *error = NULL;
    klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (klass->connection == NULL)
    {
        g_printerr("Unable to connect to dbus: %s", error->message);
        g_error_free (error);
        return;
    }	

    dbus_g_object_type_install_info (TYPE_GEOCLUE_POSITION, &dbus_glib_geoclue_position_object_info);	
    
}


gboolean geoclue_position_version (GeocluePosition *server, 
                                   gint* OUT_major, 
                                   gint* OUT_minor, 
                                   gint* OUT_micro, 
                                   GError **error)
{
    *OUT_major = 1;
    *OUT_minor = 0;
    *OUT_micro = 0;   
    return TRUE;
}


gboolean geoclue_position_service_provider (GeocluePosition *server, 
                                            char** name, 
                                            GError **error)
{
    *name = "Manual input";
    return TRUE;
}


gboolean geoclue_position_current_position (GeocluePosition *server, 
                                            gint* OUT_timestamp,
                                            gdouble* OUT_latitude, 
                                            gdouble* OUT_longitude, 
                                            gdouble* OUT_altitude, 
                                            GError **error)
{
    if (server->current_position_set && is_time_in_future (server->valid_until)) {
        *OUT_latitude = server->latitude;
        *OUT_longitude = server->longitude;
/*        *OUT_altitude = server->altitude; */
        *OUT_timestamp = server->timestamp;
        g_debug ("valid position");
        return TRUE;
    } else {
        g_debug ("position not valid or not set");
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NODATA,
                     "Valid manual position data was not found.");
        return FALSE;
    }
}

gboolean geoclue_position_current_position_error (GeocluePosition* server,
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

gboolean geoclue_position_current_velocity (GeocluePosition* server,
                                            gint* OUT_timestamp,
                                            gdouble* OUT_north_velocity, 
                                            gdouble* OUT_east_velocity,
                                            gdouble* OUT_altitude_velocity, 
                                            GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_satellites_in_view (GeocluePosition *server, 
                                              GArray** OUT_prn_numbers, 
                                              GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
}

gboolean geoclue_position_satellites_data (GeocluePosition *server, 
                                           const gint IN_prn_number, 
                                           gdouble* OUT_elevation, 
                                           gdouble* OUT_azimuth, 
                                           gdouble* OUT_signal_noise_ratio, 
                                           gboolean* OUT_differential, 
                                           gboolean* OUT_ephemeris, 
                                           GError **error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
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
    if (server->civic_location_set && is_time_in_future (server->valid_until)) {
        *OUT_country = g_strdup (server->country);
        *OUT_region = g_strdup (server->region);
        *OUT_locality = g_strdup (server->locality);
        *OUT_area = g_strdup (server->area);
        *OUT_postalcode = g_strdup (server->postalcode);
        *OUT_street = g_strdup (server->street);
        *OUT_building = g_strdup (server->building);
        *OUT_floor = g_strdup (server->floor);
        *OUT_room = g_strdup (server->room);
        *OUT_description = g_strdup (server->description);
        *OUT_text = g_strdup (server->text);
        return TRUE;
    } else {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NODATA,
                     "Valid manual position data is not available.");
        return FALSE;
    }
}


gboolean geoclue_position_civic_location_supports (GeocluePosition* server,
                                                   gboolean* OUT_country,
                                                   gboolean* OUT_region,
                                                   gboolean* OUT_locality,
                                                   gboolean* OUT_area,
                                                   gboolean* OUT_postalcode,
                                                   gboolean* OUT_street,
                                                   gboolean* OUT_building,
                                                   gboolean* OUT_floor,
                                                   gboolean* OUT_room,
                                                   gboolean* OUT_description,
                                                   gboolean* OUT_text,
                                                   GError** error)
{
    *OUT_country = TRUE;
    *OUT_region = TRUE;
    *OUT_locality = TRUE;
    *OUT_area = TRUE;
    *OUT_postalcode = TRUE;
    *OUT_street = TRUE;
    *OUT_building = TRUE;
    *OUT_floor = TRUE;
    *OUT_room = TRUE;
    *OUT_description = TRUE;
    *OUT_text = TRUE;
    return TRUE;
}

gboolean geoclue_position_service_status (GeocluePosition* server,
                                          gint* OUT_status, 
                                          char** OUT_string, 
                                          GError **error)
{

/*  TODO: make this work dynamically, based on what data server has */

    *OUT_status =     GEOCLUE_POSITION_LONGITUDE_AVAILABLE | GEOCLUE_POSITION_LATITUDE_AVAILABLE;
    *OUT_string = g_strdup("Manual Position Set");
    return TRUE;
}

gboolean geoclue_position_shutdown (GeocluePosition *obj, GError** error)
{
    g_main_loop_quit (obj->loop);
    return TRUE;
}


int main (int argc, char **argv) 
{
    GeocluePosition* server;
    
    g_type_init ();
    g_thread_init (NULL);
    
    server = GEOCLUE_POSITION (g_type_create_instance (geoclue_position_get_type ()));
    server->loop = g_main_loop_new (NULL,TRUE);
    
    g_debug ("Running main loop");
    g_main_loop_run (server->loop);
    
    g_main_loop_unref (server->loop);
    g_object_unref (server);
    
    return(0);
}
