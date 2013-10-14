/* vim: set et ts=8 sw=8: */
/* where-am-i.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <config.h>

#include <stdlib.h>
#include <glib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

GMainLoop *main_loop;

static void
on_location_proxy_ready (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        GDBusProxy *manager = G_DBUS_PROXY (user_data);
        GDBusProxy *location = G_DBUS_PROXY (source_object);
        GVariant *value;
        gdouble latitude, longitude, accuracy;
        const char *desc;
        gsize desc_len;
        GError *error = NULL;

        location = g_dbus_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-5);
        }

        value = g_dbus_proxy_get_cached_property (location, "Latitude");
        latitude = g_variant_get_double (value);
        value = g_dbus_proxy_get_cached_property (location, "Longitude");
        longitude = g_variant_get_double (value);
        value = g_dbus_proxy_get_cached_property (location, "Accuracy");
        accuracy = g_variant_get_double (value);

        g_print ("Latitude: %f\nLongitude: %f\nAccuracy (in meters): %f\n",
                 latitude,
                 longitude,
                 accuracy);

        value = g_dbus_proxy_get_cached_property (location, "Description");
        desc = g_variant_get_string (value, &desc_len);
        if (desc_len > 0)
                g_print ("Description: %s\n", desc);

        g_object_unref (location);
        g_object_unref (manager);

        g_main_loop_quit (main_loop);
}

static void
on_client_signal (GDBusProxy *client,
                  gchar      *sender_name,
                  gchar      *signal_name,
                  GVariant   *parameters,
                  gpointer    user_data)
{
        char *location_path;
        if (g_strcmp0 (signal_name, "LocationUpdated") != 0)
                return;

        g_assert (g_variant_n_children (parameters) > 1);
        g_variant_get_child (parameters, 1, "&o", &location_path);

        g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  NULL,
                                  "org.freedesktop.GeoClue2",
                                  location_path,
                                  "org.freedesktop.GeoClue2.Location",
                                  NULL,
                                  on_location_proxy_ready,
                                  user_data);
        g_object_unref (client);
}

static void
on_start_ready (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
        GDBusProxy *client = G_DBUS_PROXY (source_object);
        GVariant *results;
        GError *error = NULL;

        results = g_dbus_proxy_call_finish (client, res, &error);
        if (results == NULL) {
            g_critical ("Failed to start GeoClue2 client: %s", error->message);

            exit (-4);
        }

        g_variant_unref (results);
}

static void
on_client_proxy_ready (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
        GDBusProxy *client;
        GError *error = NULL;

        client = g_dbus_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-3);
        }

        g_signal_connect (client, "g-signal",
                          G_CALLBACK (on_client_signal), user_data);

        g_dbus_proxy_call (client,
                           "Start",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           on_start_ready,
                           user_data);
}

static void
on_get_client_ready (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
        GDBusProxy *manager = G_DBUS_PROXY (source_object);
        GVariant *results;
        const char *client_path;
        GError *error = NULL;

        results = g_dbus_proxy_call_finish (manager, res, &error);
        if (results == NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-2);
        }

        g_assert (g_variant_n_children (results) > 0);
        g_variant_get_child (results, 0, "&o", &client_path);

        g_print ("Client object: %s\n", client_path);

        g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  NULL,
                                  "org.freedesktop.GeoClue2",
                                  client_path,
                                  "org.freedesktop.GeoClue2.Client",
                                  NULL,
                                  on_client_proxy_ready,
                                  manager);
        g_variant_unref (results);
}

static void
on_manager_proxy_ready (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
        GDBusProxy *manager;
        GError *error = NULL;

        manager = g_dbus_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-1);
        }

        g_dbus_proxy_call (manager,
                           "GetClient",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           on_get_client_ready,
                           NULL);
}

gint
main (gint argc, gchar *argv[])
{
        setlocale (LC_ALL, "");
        textdomain (GETTEXT_PACKAGE);
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        g_set_application_name (_("Where Am I"));

        g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  NULL,
                                  "org.freedesktop.GeoClue2",
                                  "/org/freedesktop/GeoClue2/Manager",
                                  "org.freedesktop.GeoClue2.Manager",
                                  NULL,
                                  on_manager_proxy_ready,
                                  NULL);

        main_loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (main_loop);

        return EXIT_SUCCESS;
}
