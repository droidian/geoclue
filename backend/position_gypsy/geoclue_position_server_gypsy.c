/**
 *  TODO (methods):
 *    * velocity (geoclue api is weird here)
 *    * satellites-data
 *    
 *    * could avoid starting the different gypsy interfaces until they're 
 *      needed (say accuracy might never be queried...)
 * 
 **/



/* Gypsy position provider for geoclue
 * Copyright (C) 2007 Openedhand Ltd
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
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

#include <dbus/dbus-glib-bindings.h>
#include <gconf/gconf-client.h>

#include "geoclue_position_server_gypsy.h"
#include "geoclue_position_server_glue.h"
#include "geoclue_position_signal_marshal.h"

#include "../common/geoclue_position_error.h"

#define GCONF_POS_GYPSY "/apps/geoclue/position/gypsy"
#define GCONF_POS_GYPSY_DEVICE GCONF_POS_GYPSY"/default_device"


G_DEFINE_TYPE (GeocluePosgypsy, geoclue_posgypsy, G_TYPE_OBJECT);

/*signals*/
enum {
	CURRENT_POSITION_CHANGED,
	SERVICE_STATUS_CHANGED,
	CIVIC_LOCATION_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

//Default handler
static void 
geoclue_posgypsy_current_position_changed (GeocluePosgypsy *server,
                                           gint             timestamp,
                                           gdouble          lat,
                                           gdouble          lon,
                                           gdouble          alt)
{
	/*g_debug ("pos: %f,  %f,  %f",lat, lon, alt);*/
}

//Default handler
static void geoclue_posgypsy_civic_location_changed (GeocluePosgypsy *server,
                                                     char            *country, 
                                                     char            *region,
                                                     char            *locality,
                                                     char            *area,
                                                     char            *postalcode,
                                                     char            *street,
                                                     char            *building,
                                                     char            *floor,
                                                     char            *room,
                                                     char            *description,
                                                     char            *text)
{
}

//Default handler
static void 
geoclue_posgypsy_service_status_changed (GeocluePosgypsy *server,
                                         gint             status,
                                         char            *user_message)
{
	/*g_debug ("status: %d",status);*/
}


static void
geoclue_posgypsy_gypsy_pos_callback (GypsyPosition       *position,
                                     GypsyPositionFields  fields_set,
                                     int                  timestamp,
                                     double               latitude,
                                     double               longitude,
                                     double               altitude,
                                     gpointer             userdata)
{
	g_return_if_fail (GEOCLUE_IS_POSGYPSY(userdata));
	GeocluePosgypsy *server = (GeocluePosgypsy*)userdata;
	
	if ((fields_set & GYPSY_POSITION_FIELDS_LATITUDE) &&
	    (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE)) {
		g_signal_emit (G_OBJECT (server), 
		               signals[CURRENT_POSITION_CHANGED],
		               0,
		               timestamp,
		               latitude, 
		               longitude, 
		               altitude);
	}
}

static void
geoclue_posgypsy_gypsy_conn_callback (GypsyDevice *device, 
                                      gboolean     connected,
                                      gpointer     userdata)
{
	g_return_if_fail (GEOCLUE_IS_POSGYPSY(userdata));
	GeocluePosgypsy *server = (GeocluePosgypsy*)userdata;
	
	/* not sure if we should do something if connected = TRUE ... */
	if (!connected) {
		server->status = GEOCLUE_POSITION_NO_SERVICE_AVAILABLE;
		g_signal_emit (G_OBJECT (server), 
		               signals[SERVICE_STATUS_CHANGED],
		               0,
		               server->status,
		               "Connection to device is lost");
	}
}

static void
geoclue_posgypsy_gypsy_fix_callback (GypsyDevice *device, 
                                     int          fix,
                                     gpointer     userdata)
{
	g_return_if_fail (GEOCLUE_IS_POSGYPSY(userdata));
	GeocluePosgypsy *server = (GeocluePosgypsy*)userdata;
	
	/* TODO: set unique status messages for cases */
	gint new_status;
	switch (fix) {
		case GYPSY_DEVICE_FIX_STATUS_INVALID:
			/* This translates to no service I guess? */
			new_status = GEOCLUE_POSITION_NO_SERVICE_AVAILABLE;
			break;
		case GYPSY_DEVICE_FIX_STATUS_NONE:
			/* no fix, but acquiring one */
			new_status = GEOCLUE_POSITION_ACQUIRING_LATITUDE +
			             GEOCLUE_POSITION_ACQUIRING_LONGITUDE +
			             GEOCLUE_POSITION_ACQUIRING_ALTITUDE;
			break;
		case GYPSY_DEVICE_FIX_STATUS_2D:
			/* lat/lon available, but are we really acquiring alt? */
			new_status = GEOCLUE_POSITION_LATITUDE_AVAILABLE +
			             GEOCLUE_POSITION_LONGITUDE_AVAILABLE +
			             GEOCLUE_POSITION_ACQUIRING_ALTITUDE;
			break;
		case GYPSY_DEVICE_FIX_STATUS_3D:
			new_status = GEOCLUE_POSITION_LATITUDE_AVAILABLE +
			             GEOCLUE_POSITION_LONGITUDE_AVAILABLE +
			             GEOCLUE_POSITION_ALTITUDE_AVAILABLE;
			break;
		default:
			g_assert_not_reached();
	}
	if (new_status != server->status) {
		g_signal_emit (G_OBJECT (server), 
		               signals[SERVICE_STATUS_CHANGED],
		               0,
		               new_status,
		               "Gypsy fix status changed");
		server->status = new_status;
	}
}

gboolean 
geoclue_posgypsy_version (GeocluePosgypsy  *server,
                          int              *OUT_major,
                          int              *OUT_minor,
                          int              *OUT_micro,
                          GError          **error)
{
	*OUT_major = 1;
	*OUT_minor = 0;
	*OUT_micro = 0;
	return TRUE;
}

gboolean
geoclue_posgypsy_service_name (GeocluePosgypsy  *server,
                               char            **OUT_name,
                               GError          **error)
{
	*OUT_name = g_strdup ("Gypsy");
	return TRUE;
}

gboolean
geoclue_posgypsy_current_position (GeocluePosgypsy *server,
                                   int             *OUT_timestamp,
                                   double          *OUT_latitude,
                                   double          *OUT_longitude,
                                   double          *OUT_altitude,
                                   GError         **error)
{
	GypsyPositionFields mask;
	mask = gypsy_position_get_position (server->gypsy_pos,
	                                    OUT_timestamp, 
	                                    OUT_latitude, OUT_longitude, 
	                                    OUT_altitude,
	                                    error);
	return ((mask & GYPSY_POSITION_FIELDS_LATITUDE) &&
	        (mask & GYPSY_POSITION_FIELDS_LONGITUDE));
}

gboolean
geoclue_posgypsy_current_position_error (GeocluePosgypsy  *server,
                                         double           *OUT_latitude_error,
                                         double           *OUT_longitude_error,
                                         double           *OUT_altitude_error,
                                         GError          **error)
{
	gdouble pdop, hdop, vdop;
	GypsyAccuracyFields mask;
	mask = gypsy_accuracy_get_accuracy (server->gypsy_acc,
	                                    &pdop, &hdop, &vdop, error);
	if (!error) {
		g_debug ("pdop: %s\n", pdop);
		g_debug ("hdop: %s\n", hdop);
		g_debug ("vdop: %s\n", vdop);
	}
	g_set_error (error,
	             GEOCLUE_POSITION_ERROR,
	             GEOCLUE_POSITION_ERROR_FAILED,
	             "Method not implemented yet.");
	return FALSE;
}

gboolean
geoclue_posgypsy_current_velocity (GeocluePosgypsy  *server,
                                   int              *OUT_timestamp,
                                   double           *OUT_north_velocity,
                                   double           *OUT_east_velocity,
                                   double           *OUT_altitude_velocity,
                                   GError          **error)
{
	g_set_error (error,
	             GEOCLUE_POSITION_ERROR,
	             GEOCLUE_POSITION_ERROR_FAILED,
	             "Method not implemented yet.");
	return FALSE;
}

gboolean
geoclue_posgypsy_satellites_data (GeocluePosgypsy  *server,
                                  int              *OUT_timestamp,
                                  GArray          **OUT_prn_number,
                                  GArray          **OUT_elevation,
                                  GArray          **OUT_azimuth,
                                  GArray          **OUT_snr,
                                  GArray          **OUT_differential,
                                  GArray          **OUT_ephemeris,
                                  GError          **error)
{
	g_set_error (error,
	             GEOCLUE_POSITION_ERROR,
	             GEOCLUE_POSITION_ERROR_FAILED,
	             "Method not implemented yet.");
	return FALSE;
}

gboolean
geoclue_posgypsy_civic_location (GeocluePosgypsy  *server,
                                 char            **OUT_country,
                                 char            **OUT_region,
                                 char            **OUT_locality,
                                 char            **OUT_area,
                                 char            **OUT_postalcode,
                                 char            **OUT_street,
                                 char            **OUT_building,
                                 char            **OUT_floor,
                                 char            **OUT_description,
                                 char            **OUT_room,
                                 char            **OUT_text,
                                 GError          **error)
{
	g_set_error (error,
	             GEOCLUE_POSITION_ERROR,
	             GEOCLUE_POSITION_ERROR_NOTSUPPORTED,
	             "Method not supported.");
	return FALSE;
}

gboolean
geoclue_posgypsy_service_status (GeocluePosgypsy  *server,
                                 int              *OUT_status,
                                 char            **OUT_string,
                                 GError          **error)
{
	/* status is saved in the gypsy fix status handler */
	*OUT_status = server->status;
	return TRUE;
}

gboolean
geoclue_posgypsy_shutdown (GeocluePosgypsy  *server,
                           GError          **error)
{
	g_main_loop_quit (server->loop);
	return TRUE;
}

static void
geoclue_posgypsy_class_init (GeocluePosgypsyClass *klass)
{
	GError *error = NULL;
	
	klass->current_position_changed = geoclue_posgypsy_current_position_changed;
	klass->civic_location_changed = geoclue_posgypsy_civic_location_changed;
	klass->service_status_changed = geoclue_posgypsy_service_status_changed;
	
	signals[CURRENT_POSITION_CHANGED] = 
	    g_signal_new ("current_position_changed",
	                  GEOCLUE_POSGYPSY_TYPE,
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeocluePosgypsyClass,
	                                   current_position_changed),
	                  NULL,
	                  NULL, 
	                  _geoclue_posgypsy_VOID__INT_DOUBLE_DOUBLE_DOUBLE,
	                  G_TYPE_NONE,
	                  4, 
	                  G_TYPE_INT,
	                  G_TYPE_DOUBLE, 
	                  G_TYPE_DOUBLE, 
	                  G_TYPE_DOUBLE);
	
	signals[SERVICE_STATUS_CHANGED] = 
	    g_signal_new ("service_status_changed",
	                  GEOCLUE_POSGYPSY_TYPE,
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeocluePosgypsyClass,
	                                   service_status_changed),
	                  NULL, 
	                  NULL,
	                  _geoclue_posgypsy_VOID__INT_STRING,
	                  G_TYPE_NONE, 
	                  2, 
	                  G_TYPE_INT, 
	                  G_TYPE_STRING);
	
	signals[CIVIC_LOCATION_CHANGED] =
	    g_signal_new ("civic_location_changed",
	                  GEOCLUE_POSGYPSY_TYPE,
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeocluePosgypsyClass,
	                                   civic_location_changed),
	                  NULL, NULL,
	                  _geoclue_posgypsy_VOID__STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING_STRING,
	                  G_TYPE_NONE,
	                  11, 
	                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                  G_TYPE_STRING, G_TYPE_STRING);
	
	
	klass->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (klass->connection == NULL) {
		g_warning ("Unable to connect to dbus: %s", error->message);
		g_error_free (error);
		return;
	}
	
	dbus_g_object_type_install_info (GEOCLUE_POSGYPSY_TYPE,
	                                 &dbus_glib_geoclue_posgypsy_object_info);
}

/*callback for async control creation*/
static void
geoclue_posgypsy_finish_gypsy_init (GypsyControl *control,
                                    gchar *path,
                                    gpointer user_data)
{
	GeocluePosgypsy *server = (GeocluePosgypsy*)user_data;
	GError *error;
	
	if (!path) {
		g_printerr ("Gypsy device creation failed");
		return;
	}
	
	server->gypsy_pos = gypsy_position_new (path);
	g_signal_connect (G_OBJECT (server->gypsy_pos), 
	                  "position-changed",
	                  (GCallback)geoclue_posgypsy_gypsy_pos_callback,
	                  server);
	
	server->gypsy_acc = gypsy_accuracy_new (path);
	/* no accuracy signals in geoclue yet */
	
	server->gypsy_dev = gypsy_device_new (path);
	g_signal_connect (G_OBJECT (server->gypsy_dev),
	                  "connection-changed",
	                  (GCallback)geoclue_posgypsy_gypsy_conn_callback,
	                  server);
	g_signal_connect (G_OBJECT (server->gypsy_dev),
	                  "fix-status-changed",
	                  (GCallback)geoclue_posgypsy_gypsy_fix_callback,
	                  server);
	if (!gypsy_device_start (server->gypsy_dev, &error)) {
		g_printerr ("Error starting gypsy device: %s", error->message);
		g_error_free (error);
	}
	return;
}

static gboolean
geoclue_posgypsy_start_gypsy (GeocluePosgypsy *server,
                              gchar *device,
                              GError *error)
{
	server->gypsy_ctrl = gypsy_control_get_default ();
	if (device) {
		char *path = gypsy_control_create (server->gypsy_ctrl, 
		                                   device, 
		                                   &error);
		if (error) {
			g_printerr ("Error creating gypsy control\n");
			return FALSE;
		}
		/*run rest of the initialization...*/
		geoclue_posgypsy_finish_gypsy_init (NULL, path, server);
	} else {
		/* NOTE: gypsy does not do device autoselection yet, 
		 * remove return and uncomment code path when it does */
		g_printerr ("No device path was specified\n");
		return FALSE;
		
		
		/* finish_gypsy_init will be called after control is created */
		/*
		if (!gypsy_control_create_async (server->gypsy_ctrl, 
		                                 G_CALLBACK (geoclue_posgypsy_finish_gypsy_init),
		                                 server,
		                                 &error)) {
			g_printerr ("Error creating gypsy control\n");
			return FALSE;
		}
		*/
	}
	return TRUE;
}

static void
geoclue_posgypsy_stop_gypsy (GeocluePosgypsy *server)
{
	GError *error = NULL;
	if (server->status != GEOCLUE_POSITION_NO_SERVICE_AVAILABLE) {
		server->status = GEOCLUE_POSITION_NO_SERVICE_AVAILABLE;
		g_signal_emit (G_OBJECT (server), 
		               signals[SERVICE_STATUS_CHANGED],
		               0,
		               server->status,
		               "Gypsy stopping");
		
	}
	
	if (server->gypsy_dev != NULL) {
		if (!gypsy_device_stop (server->gypsy_dev, &error)) {
			g_printerr ("Error stopping Gypsy device: %s",
			            error->message);
			g_error_free (error);
		}
		g_object_unref (server->gypsy_dev);
		server->gypsy_dev = NULL;
	}
	if (server->gypsy_pos) {
		g_object_unref (server->gypsy_pos);
		server->gypsy_pos = NULL;
	}
	if (server->gypsy_acc) {
		g_object_unref (server->gypsy_acc);
		server->gypsy_acc = NULL;
	}
	if (server->gypsy_ctrl) {
		g_object_unref (server->gypsy_ctrl);
		server->gypsy_ctrl = NULL;
	}
}

static void
geoclue_posgypsy_device_changed (GConfClient *gconf,
                                    guint id,
                                    GConfEntry *entry,
                                    gpointer userdata)
{
	gchar *addr = NULL;
	GError *error = NULL;
	
	g_return_if_fail (GEOCLUE_IS_POSGYPSY(userdata));
	GeocluePosgypsy *server = (GeocluePosgypsy*)userdata;
	
	g_debug ("Restarting Gypsy (preferred device changed)");
	
	addr = gconf_client_get_string (gconf, GCONF_POS_GYPSY_DEVICE, &error);
	if (!addr) {
		g_printerr ("Getting gconf string failed: %s", error->message);
		g_error_free (error);
		return;
	}
	
	geoclue_posgypsy_stop_gypsy (server);
	
	if (!geoclue_posgypsy_start_gypsy (server, addr, error)) {
		g_printerr ("Starting Gypsy failed: %s", error->message);
		g_error_free (error);
	}
	g_free (addr);
}

static void
geoclue_posgypsy_init (GeocluePosgypsy *server)
{
	GError *error = NULL;
	DBusGProxy *driver_proxy;
	GeocluePosgypsyClass *klass = GEOCLUE_POSGYPSY_GET_CLASS (server);
	guint request_ret;
	
	dbus_g_connection_register_g_object (klass->connection,
	                                     GEOCLUE_POSGYPSY_DBUS_PATH,
	                                     G_OBJECT (server));
	driver_proxy = dbus_g_proxy_new_for_name (klass->connection,
	                                          DBUS_SERVICE_DBUS,
	                                          DBUS_PATH_DBUS,
	                                          DBUS_INTERFACE_DBUS);
	
	if (!org_freedesktop_DBus_request_name (driver_proxy,
	                                        GEOCLUE_POSGYPSY_DBUS_SERVICE,
	                                        0, 
	                                        &request_ret,
	                                        &error)) {
		g_warning ("Unable to register geoclue service: %s",
		           error->message);
		g_error_free (error);
	}
	
	server->status = GEOCLUE_POSITION_NO_SERVICE_AVAILABLE;
	
	/* Add gconf notify on device key */
	GConfClient *gconf;
	gchar *device;
	
	gconf = gconf_client_get_default ();
	gconf_client_add_dir (gconf,
	                      GCONF_POS_GYPSY,
	                      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	/* TODO: save gconf connection and remove on shutdown */
	
	gconf_client_notify_add (gconf,
	                         GCONF_POS_GYPSY_DEVICE,
	                         (GConfClientNotifyFunc)geoclue_posgypsy_device_changed,
	                         (gpointer)server,
	                         NULL, &error);
	
	/* get device name from gconf */
	device = gconf_client_get_string (gconf, GCONF_POS_GYPSY_DEVICE, NULL);
	if (!device) {
		g_warning ("Getting preferred device from gconf failed");
	}
	
	g_debug ("Starting Gypsy");
	if (!geoclue_posgypsy_start_gypsy (server, device, error)) {
		g_printerr ("Could not start gypsy\n");
		if (error) {
			g_error_free (error);
		}
	}
	
	g_free (device);
	g_object_unref (gconf);
}

int
main (int    argc,
      char **argv)
{
	GeocluePosgypsy *server;
	
	g_type_init ();
	
	server = g_object_new (GEOCLUE_POSGYPSY_TYPE, NULL);
	server->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (server->loop);
	
	g_main_loop_unref (server->loop);
	g_object_unref (server);
	return 0;
}
