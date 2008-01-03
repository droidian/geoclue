/*
 * Geoclue
 * main.c - Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GMainLoop *mainloop;

int
main (int    argc,
      char **argv)
{
	DBusGConnection *conn;
	GError *error = NULL;

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

	master = g_object_new (GEOCLUE_TYPE_MASTER, NULL);
	dbus_g_connection_register_g_object (conn, 
					     "/org/freedesktop/Geoclue/Master", 
					     G_OBJECT (master));

	g_main_loop_run (mainloop);
	return 0;
}
