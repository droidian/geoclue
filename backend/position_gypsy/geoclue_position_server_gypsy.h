/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2007 OpenedHand Ltd
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

#ifndef __GEOCLUE_POSITION_SERVER_GYPSY_H__
#define __GEOCLUE_POSITION_SERVER_GYPSY_H__

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <glib.h>

#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-accuracy.h>

G_BEGIN_DECLS

#define GEOCLUE_POSGYPSY_DBUS_SERVICE   "org.freedesktop.geoclue.position.gypsy"
#define GEOCLUE_POSGYPSY_DBUS_PATH      "/org/freedesktop/geoclue/position/gypsy"
#define GEOCLUE_POSITION_DBUS_INTERFACE "org.freedesktop.geoclue.position"

#define GEOCLUE_POSGYPSY_TYPE            (geoclue_posgypsy_get_type ())
#define GEOCLUE_POSGYPSY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_POSGYPSY_TYPE, GeocluePosgypsy))
#define GEOCLUE_POSGYPSY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_POSGYPSY_TYPE, GeocluePosgypsyClass))
#define GEOCLUE_IS_POSGYPSY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_POSGYPSY_TYPE))
#define GEOCLUE_IS_POSGYPSY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_POSGYPSY_TYPE))
#define GEOCLUE_POSGYPSY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCLUE_POSGYPSY_TYPE, GeocluePosgypsyClass))

typedef struct GeocluePosgypsy GeocluePosgypsy;
typedef struct GeocluePosgypsyClass GeocluePosgypsyClass;

GType geoclue_position_get_type (void);
struct GeocluePosgypsy
{
	GObject parent;
	GMainLoop*  loop;
	
	GypsyControl *gypsy_ctrl;
	GypsyDevice *gypsy_dev;
	GypsyPosition *gypsy_pos;
	GypsyAccuracy *gypsy_acc;
	
	gint status;
	
};

struct GeocluePosgypsyClass
{
	GObjectClass parent;
	DBusGConnection *connection;
	
	/* Signals */
	void (*current_position_changed) (GeocluePosgypsy* server,
	                                  gint timestamp,
	                                  gdouble lat,
	                                  gdouble lon,
	                                  gdouble altitude );
	
	void (*civic_location_changed) (GeocluePosgypsy* server,
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
	
	void (*service_status_changed) (GeocluePosgypsy* server,
	                                gint status,
	                                char* user_message);
};

gboolean geoclue_posgypsy_version (GeocluePosgypsy* server,
                                   int* major,
                                   int* minor,
                                   int* micro, 
                                   GError **error);

gboolean geoclue_posgypsy_service_name (GeocluePosgypsy* server,
                                        char** name, 
                                        GError **error);       

gboolean geoclue_posgypsy_current_position (GeocluePosgypsy* server,
                                            gint* OUT_timestamp,
                                            gdouble* OUT_latitude,
                                            gdouble* OUT_longitude,
                                            gdouble* OUT_altitude, 
                                            GError **error);

gboolean geoclue_posgypsy_current_position_error (GeocluePosgypsy* server,
                                                  gdouble* OUT_latitude_error, 
                                                  gdouble* OUT_longitude_error, 
                                                  gdouble* OUT_altitude_error, 
                                                  GError **error);

gboolean geoclue_posgypsy_service_status (GeocluePosgypsy* server,
                                          gint* OUT_status, 
                                          char** OUT_string, 
                                          GError **error);    

gboolean geoclue_posgypsy_current_velocity (GeocluePosgypsy* server,
                                            gint* OUT_timestamp,
                                            gdouble* OUT_north_velocity, 
                                            gdouble* OUT_east_velocity,
                                            gdouble* OUT_altitude_velocity, 
                                            GError **error);

gboolean geoclue_posgypsy_satellites_data (GeocluePosgypsy* server,
                                           gint* OUT_timestamp,
                                           GArray** OUT_prn_number,
                                           GArray** OUT_elevation, 
                                           GArray** OUT_azimuth, 
                                           GArray** OUT_signal_noise_ratio, 
                                           GArray** OUT_differential, 
                                           GArray** OUT_ephemeris,
                                           GError **error);

gboolean geoclue_posgypsy_civic_location (GeocluePosgypsy* server, 
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

gboolean geoclue_posgypsy_shutdown (GeocluePosgypsy *obj,
                                    GError** error);

G_END_DECLS

#endif
