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

#ifndef __GEOCLUE_POSITION_SERVER_H__
#define __GEOCLUE_POSITION_SERVER_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus-glib.h>
#include <glib.h>

#include "../common/geoclue_web_service.h"

#define GEOCLUE_POSITION_DBUS_SERVICE     "org.freedesktop.geoclue.position.hostip"
#define GEOCLUE_POSITION_DBUS_PATH        "/org/freedesktop/geoclue/position/hostip"
#define GEOCLUE_POSITION_DBUS_INTERFACE   "org.freedesktop.geoclue.position"

G_BEGIN_DECLS

typedef struct GeocluePosition GeocluePosition;
typedef struct GeocluePositionClass GeocluePositionClass;

GType geoclue_position_get_type (void);
struct GeocluePosition
{
    GObject parent;
    GMainLoop* loop;
    
    GeoclueWebService *web_service;
    
    gdouble current_lat;
    gdouble current_lon;
    gboolean is_current_valid;
    
    gchar* civic_country;
    gchar* civic_region;
    gchar* civic_locality;
    gchar* civic_area;
    gchar* civic_postalcode;
    gchar* civic_street;
    gchar* civic_building;
    gchar* civic_floor;
    gchar* civic_room;
    gchar* civic_description;
    gchar* civic_text;
    gboolean is_civic_valid;
};

struct GeocluePositionClass
{
  GObjectClass parent;
  DBusGConnection *connection;

	/* Signals */
	void (*current_position_changed) (	GeocluePosition* server,
										gint timestamp,
										gdouble lat,
										gdouble lon,
										gdouble altitude );
	
	void (*civic_location_changed) (	GeocluePosition* server,
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
	                                   	char* text );
	
	void (*service_status_changed) (	GeocluePosition* server,
										gint status,
										char* user_message );
};

#define TYPE_GEOCLUE_POSITION              (geoclue_position_get_type ())
#define GEOCLUE_POSITION(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOCLUE_POSITION, GeocluePosition))
#define GEOCLUE_POSITION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOCLUE_POSITION, GeocluePositionClass))
#define IS_GEOCLUE_POSITION(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOCLUE_POSITION))
#define IS_GEOCLUE_POSITION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOCLUE_POSITION))
#define GEOCLUE_POSITION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOCLUE_POSITION, GeocluePositionClass))


gboolean geoclue_position_version(	GeocluePosition* server,
									int* major,
									int* minor,
									int* micro, 
									GError **error);

gboolean geoclue_position_service_name(	GeocluePosition* server,
										char** name, 
										GError **error);       


gboolean geoclue_position_current_position (	GeocluePosition* server,
												gint* OUT_timestamp,
												gdouble* OUT_latitude,
												gdouble* OUT_longitude,
												gdouble* OUT_altitude, 
												GError **error);



gboolean geoclue_position_current_position_error(	GeocluePosition* server,
													gdouble* OUT_latitude_error, 
													gdouble* OUT_longitude_error, 
													gdouble* OUT_altitude_error, 
													GError **error);

gboolean geoclue_position_service_status	 (	GeocluePosition* server,
												gint* OUT_status, 
												char** OUT_string, 
												GError **error);    




gboolean geoclue_position_current_velocity (	GeocluePosition* server,
												gint* OUT_timestamp,
												gdouble* OUT_north_velocity, 
												gdouble* OUT_east_velocity,
												gdouble* OUT_altitude_velocity, 
												GError **error);



gboolean geoclue_position_satellites_data (	GeocluePosition* server,
											gint* OUT_timestamp,
											GArray** OUT_prn_number,
											GArray** OUT_elevation, 
											GArray** OUT_azimuth, 
											GArray** OUT_signal_noise_ratio, 
											GArray** OUT_differential, 
											GArray** OUT_ephemeris,
											GError **error);

gboolean geoclue_position_civic_location (	GeocluePosition* server, 
											char** OUT_country, 
						                    char** OUT_region,
						                    char** OUT_locality,
						                    char** OUT_area,
						                    char** OUT_postalcode,
						                    char** OUT_street,
						                    char** OUT_building,
						                    char** OUT_floor,
						                    char** OUT_description,
						                    char** OUT_room,
						                    char** OUT_text, 
						                    GError **error);

gboolean geoclue_position_shutdown(	GeocluePosition *obj,
									GError** error);


G_END_DECLS

#endif

