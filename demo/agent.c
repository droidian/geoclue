/* vim: set et ts=8 sw=8: */
/* agent.c
 *
 * Copyright 2013 Red Hat, Inc.
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

#include <gio/gio.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <libnotify/notify.h>

#include "gclue-service-agent.h"

static GOptionEntry entries[] =
{
        { "version",
          0,
          0,
          G_OPTION_ARG_NONE,
          NULL,
          N_("Display version number"),
          NULL },
        G_OPTION_ENTRY_NULL
};

GClueServiceAgent *agent = NULL;

static gint
handle_local_options_cb (GApplication *app,
                         GVariantDict *options,
                         gpointer      user_data)
{
        gboolean version;

        if (g_variant_dict_lookup (options, "version", "b", &version)) {
                g_print ("%s\n", PACKAGE_VERSION);
                return EXIT_SUCCESS;
        }

        return -1;
}

static void
on_get_bus_ready (GObject      *source_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
        g_autoptr(GError) error = NULL;
        GDBusConnection *connection;

        connection = g_bus_get_finish (res, &error);
        if (connection == NULL) {
                g_critical ("Failed to get connection to system bus: %s",
                            error->message);

                exit (-2);
        }

        agent = gclue_service_agent_new (connection);
}

#define ABS_PATH ABS_SRCDIR "/agent"

static void
activate_cb (GApplication *app,
             gpointer      user_data)
{
        g_bus_get (G_BUS_TYPE_SYSTEM,
                   NULL,
                   on_get_bus_ready,
                   NULL);

        g_application_hold (app);
}

static void
is_registered_cb (GApplication *app,
                  GParamSpec   *pspec,
                  gpointer      user_data)
{
        if (g_application_get_is_registered (app) && g_application_get_is_remote (app))
                g_message ("Another instance of GeoClue DemoAgent is running.");
}

int
main (int argc, char **argv)
{
        g_autoptr (GApplication) app = NULL;
        int status = 0;

        setlocale (LC_ALL, "");

        textdomain (GETTEXT_PACKAGE);
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        g_set_application_name ("GeoClue Agent");

        notify_init (_("GeoClue"));

        app = g_application_new ("org.freedesktop.GeoClue2.DemoAgent",
                                 G_APPLICATION_DEFAULT_FLAGS);

        g_application_add_main_option_entries (app, entries);
        g_application_set_option_context_parameter_string (app, "- Geoclue Agent service");

        g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);
        g_signal_connect (app, "handle-local-options", G_CALLBACK (handle_local_options_cb), NULL);
        g_signal_connect (app, "notify::is-registered", G_CALLBACK (is_registered_cb), NULL);

        status = g_application_run (G_APPLICATION (app), argc, argv);

        if (agent != NULL)
                g_object_unref (agent);

        return status;
}
