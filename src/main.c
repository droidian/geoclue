/*
 * Geoclue
 * main.c - Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <gconf/gconf-client.h>

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "master.h"

static GMainLoop *mainloop;
static GHashTable *options;
static GcMaster *master;


#define GEOCLUE_GCONF_TOP "/apps/geoclue/master"
#define GEOCLUE_MASTER_NAME "org.freedesktop.Geoclue.Master"

static void
gconf_key_changed (GConfClient *client,
		   guint cnxn_id,
		   GConfEntry *entry,
		   gpointer user_data)
{
	const char *key, *value;
	GConfValue *v;

	key = gconf_entry_get_key (entry);
	v = gconf_entry_get_value (entry);
	if (v->type != GCONF_VALUE_STRING)
		return;
	value = gconf_value_get_string (v);

	g_message ("gconf key changed %s", key);

	/* Don't add empty strings in the hashtable */
	if (value != NULL && value[0] == '\0')
		value = NULL;

	g_hash_table_insert (options, g_path_get_basename (key),
			     g_strdup (value));

	g_signal_emit_by_name (G_OBJECT (master), "options-changed", options);
}

static GHashTable *
load_options (void)
{
        GHashTable *ht = NULL;
        GConfClient *client = gconf_client_get_default ();
        GSList *entries, *e;
        GError *error = NULL;

        gconf_client_add_dir (client, GEOCLUE_GCONF_TOP,
        		      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

        entries = gconf_client_all_entries (client, GEOCLUE_GCONF_TOP, &error);
        if (error != NULL) {
                g_warning ("Error loading master options: %s", error->message);
                g_error_free (error);
                return NULL;
        }

        /* Setup keys monitoring */
	gconf_client_notify_add (client, GEOCLUE_GCONF_TOP,
				 (GConfClientNotifyFunc) gconf_key_changed,
				 NULL, NULL, NULL);

        ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
        g_print ("Master options:\n");
        for (e = entries; e; e = e->next) {
                GConfEntry *entry = e->data;
                const char *key, *value;
                GConfValue *v;

                key = gconf_entry_get_key (entry);
                v = gconf_entry_get_value (entry);
                if (v->type != GCONF_VALUE_STRING)
                	continue;
                value = gconf_value_get_string (v);

                if (value != NULL && value[0] == '\0')
                	value = NULL;

                g_print ("  %s = %s\n", key, value);
                g_hash_table_insert (ht, g_path_get_basename (key), 
                                     g_strdup (value));
                 gconf_entry_free (entry);
         }
         g_slist_free (entries);

         return ht;
 }

GHashTable *
geoclue_get_main_options (void)
{
        return options;
}

int
main (int    argc,
      char **argv)
{
	DBusGConnection *conn;
	DBusGProxy *proxy;
	GError *error = NULL;
	guint32 request_name_ret;

	g_type_init ();

	mainloop = g_main_loop_new (NULL, FALSE);

	conn = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (!conn) {
		g_error ("Error getting bus: %s", error->message);
		return 1;
	}

	proxy = dbus_g_proxy_new_for_name (conn,
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);
	if (!org_freedesktop_DBus_request_name (proxy, GEOCLUE_MASTER_NAME,
						0, &request_name_ret, &error)) {
		g_error ("Error registering D-Bus service %s: %s",
			 GEOCLUE_MASTER_NAME, error->message);
		return 1;
	}

	/* Just quit if master is already running */
	if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		return 1;
	}

        /* Load options */
        options = load_options ();

	master = g_object_new (GC_TYPE_MASTER, NULL);
	dbus_g_connection_register_g_object (conn, 
					     "/org/freedesktop/Geoclue/Master", 
					     G_OBJECT (master));

	g_main_loop_run (mainloop);
	return 0;
}
