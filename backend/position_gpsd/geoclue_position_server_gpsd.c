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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <geoclue_position_server_gpsd.h>
#include <geoclue_position_server_glue.h>
#include <geoclue_position_signal_marshal.h>
#include <dbus/dbus-glib-bindings.h>

#include <fcntl.h>
#include <stdlib.h>
#include <math.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_LIBGPSBT
#include <gpsbt.h>
#endif

#include "../geoclue_position_error.h"

#define GPSBT_MAX_ERROR_BUF_LEN 255



G_DEFINE_TYPE(GeocluePosition, geoclue_position, G_TYPE_OBJECT)


/* NOTE:
 *  "GeocluePosition* obj" should be in defined in main(), but it's here because it's
 *  needed in gps_callback (for emitting the gobject signal)... A smarter solution 
 *  would be nice 
 */
GeocluePosition* obj = NULL;

enum {
  CURRENT_POSITION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

//Default handler
void geoclue_position_current_position_changed (GeocluePosition* obj, gdouble lat, gdouble lon)
{   
    g_print("Current Position Changed\n");
}

/* callback for gpsd signals */
static void gps_callback (struct gps_data_t *gpsdata, char *message, size_t len, int level)
{
    if (gpsdata->set & LATLON_SET) {

        /* FIXME: should only emit the signal if lat/lon have actually changed */
        g_signal_emit_by_name (obj, "current_position_changed", gpsdata->fix.latitude, gpsdata->fix.longitude);
        
        /* clear the flag */
        gpsdata->set &= ~LATLON_SET;
    }
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
    *name = "gpsd";
    return TRUE;
}

gboolean geoclue_position_current_position(GeocluePosition *obj, gdouble* OUT_latitude, gdouble* OUT_longitude, GError **error )
{
    if (obj->gpsdata->online == 0) {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NOSERVICE,
                     "GPS not online.");
        return FALSE;
    }
    if (obj->gpsdata->status == STATUS_NO_FIX) {
        g_set_error (error,
                     GEOCLUE_POSITION_ERROR,
                     GEOCLUE_POSITION_ERROR_NODATA,
                     "GPS fix not aqcuired.");
        return FALSE;
    }

    *OUT_latitude = obj->gpsdata->fix.latitude;
    *OUT_longitude = obj->gpsdata->fix.longitude;
        
    g_debug ("Sending back %f %f", *OUT_latitude, *OUT_longitude);
    return TRUE;
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
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_current_velocity(GeocluePosition *obj, gdouble* OUT_north_velocity, gdouble* OUT_east_velocity, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_current_time(GeocluePosition *obj, gint* OUT_year, gint* OUT_month, gint* OUT_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_satellites_in_view(GeocluePosition *obj, GArray** OUT_prn_numbers, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_satellites_data(GeocluePosition *obj, const gint IN_prn_number, gdouble* OUT_elevation, gdouble* OUT_azimuth, gdouble* OUT_signal_noise_ratio, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_sun_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_sun_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_moon_rise(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return FALSE;
}

gboolean geoclue_position_moon_set(GeocluePosition *obj, const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_year, const gint IN_month, const gint IN_day, gint* OUT_hours, gint* OUT_minutes, gint* OUT_seconds, GError **error )
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
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
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
                 "Backend does not implement this method.");
    return FALSE;
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
    *OUT_country = FALSE;
    *OUT_region = FALSE;
    *OUT_locality = FALSE;
    *OUT_area = FALSE;
    *OUT_postalcode = FALSE;
    *OUT_street = FALSE;
    *OUT_building = FALSE;
    *OUT_floor = FALSE;
    *OUT_room = FALSE;
    *OUT_text = FALSE;
    return TRUE;
}


gboolean geoclue_position_service_available(GeocluePosition *obj, gboolean* OUT_available, char** OUT_reason, GError** error)
{
    g_set_error (error,
                 GEOCLUE_POSITION_ERROR,
                 GEOCLUE_POSITION_ERROR_FAILED,
                 "Method not implemented yet.");
    return TRUE;  
}

gboolean geoclue_position_shutdown(GeocluePosition *obj, GError** error)
{
    g_debug("position_shutdown");
    g_main_loop_quit (obj->loop);
    return TRUE;
}



int main(int argc, char **argv) 
{
    char* server = NULL;
    char* port = DEFAULT_GPSD_PORT;
    pthread_t th_gps;

    g_type_init ();
    g_thread_init (NULL);
    

#ifdef HAVE_LIBGPSBT
    /* prepare for starting gpsd on systems using libgpsbt */
    int st;
    char errbuf[GPSBT_MAX_ERROR_BUF_LEN+1];
    memset (errbuf, 0, GPSBT_MAX_ERROR_BUF_LEN+1);
    gpsbt_t bt_ctx = { {0} };
    
    setenv ("GPSD_PROG", "/usr/sbin/gpsd", 1); /* hack to bypass maemo bug */
    g_debug ("Running gpsbt_start");
    st = gpsbt_start (NULL, 0, 0, 0, errbuf, GPSBT_MAX_ERROR_BUF_LEN, 0, &bt_ctx);
    if (st<0) {
        g_printerr ("Error %d in gpsbt_start: %s (%s)\n", errno, strerror(errno), errbuf);
        return (1);
    }

    /* wait for gpsd to get started */
    sleep (1);
#endif

    obj = GEOCLUE_POSITION(g_type_create_instance (geoclue_position_get_type()));
    obj->loop = g_main_loop_new(NULL,TRUE);
    g_debug ("Starting GPSD\n");
    obj->gpsdata = gps_open(server, port);

    if(obj->gpsdata)
    {  
        g_debug ("Attaching callback, running main loop...\n");
        gps_set_callback (obj->gpsdata, gps_callback, &th_gps);

        g_main_loop_run(obj->loop);
       
        gps_close(obj->gpsdata);
     
        g_object_unref(obj);
        g_main_loop_unref(obj->loop);
    }
    else
    {
        g_printerr("Cannot Find GPSD\n");
    }

#ifdef HAVE_LIBGPSBT
    g_debug("gpsbt_stop...");
    gpsbt_stop (&bt_ctx);
#endif
     
    return(0);
}
