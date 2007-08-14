/* Geomap - A DBus api and wrapper for getting geography pictures
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
#ifndef __GEOCLUE_MASTER_H__
#define __GEOCLUE_MASTER_H__

#define DBUS_API_SUBJECT_TO_CHANGE


#include <dbus/dbus-glib.h>
#include <config.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <gconf/gconf-client.h>

#define GEOCLUE_MASTER_DBUS_SERVICE     "org.freedesktop.geoclue.master"
#define GEOCLUE_MASTER_DBUS_PATH        "/org/freedesktop/geoclue/master"
#define GEOCLUE_MASTER_DBUS_INTERFACE   "org.freedesktop.geoclue.master"   




G_BEGIN_DECLS

typedef struct GeoclueMaster GeoclueMaster;
typedef struct GeoclueMasterClass GeoclueMasterClass;

GType geoclueserver_master_get_type (void);
struct GeoclueMaster
{
    GObject parent;   
    GList* position_backends;  
    GList* map_backends;
    GList* geocode_backends;
    GList* find_backends;

    
    GConfClient* client;
};

struct GeoclueMasterClass
{
  GObjectClass parent;
  DBusGConnection *connection;
   

};

#define TYPE_GEOCLUE_MASTER              (geoclueserver_master_get_type ())
#define GEOCLUE_MASTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GEOCLUE_MASTER, GeoclueMaster))
#define GEOCLUE_MASTER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEOCLUE_MASTER, GeoclueMasterClass))
#define IS_GEOCLUE_MASTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GEOCLUE_MASTER))
#define IS_GEOCLUE_MASTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEOCLUE_MASTER))
#define GEOCLUE_MASTER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEOCLUE_MASTER, GeoclueMasterClass))

gboolean geoclue_master_version (GeoclueMaster *obj, gint* OUT_major, gint* OUT_minor, gint* OUT_micro, GError **error);

gboolean geoclue_master_get_default_position_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error);
gboolean geoclue_master_get_all_position_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error);
gboolean geoclue_master_position_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error);

gboolean geoclue_master_get_default_map_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error);
gboolean geoclue_master_get_all_map_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error);
gboolean geoclue_master_map_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error);

gboolean geoclue_master_get_default_geocode_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error);
gboolean geoclue_master_get_all_geocode_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error);
gboolean geoclue_master_geocode_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error);

gboolean geoclue_master_get_default_find_provider (GeoclueMaster *obj, char ** OUT_service, char ** OUT_path, char ** OUT_description, GError **error);
gboolean geoclue_master_get_all_find_providers (GeoclueMaster *obj, char *** OUT_service, char *** OUT_path, char *** OUT_description,  GError **error);
gboolean geoclue_master_find_provider_update (GeoclueMaster *obj, const char * IN_service, const char * IN_path, const gint IN_accuracy, const gboolean IN_active, GError **error);




G_END_DECLS




#endif

