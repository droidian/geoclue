/* vim: set et ts=8 sw=8: */
/*
 * Copyright 2023 The Droidian Project
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
 * Authors: Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdlib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <geoclue.h>

#define SAFE_UNREF(obj) if ((obj) != NULL) { g_object_unref((obj)); (obj) = NULL; }

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/local/share/locale"
#endif

#ifndef GETTEXT_PACKAGE
#define GETTEXT_PACKAGE "gclue-keeper"
#endif

static gboolean is_running = TRUE;
static gboolean verbose_mode = FALSE;
static gboolean quiet_mode = FALSE;
static gint toggle_interval = 60;

static GClueAccuracyLevel accuracy_level = GCLUE_ACCURACY_LEVEL_EXACT;
static gint time_threshold;

static GOptionEntry entries[] = {
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_mode, "Verbose mode", NULL},
    {"quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet_mode, "Quiet mode", NULL},  // New entry
    {"time", 't', 0, G_OPTION_ARG_INT, &toggle_interval, "Toggle activity interval in seconds", "N"},
    {NULL}
};

GClueSimple *simple = NULL;
GClueClient *client = NULL;
GMainLoop *main_loop;

static void print_location(GClueSimple *simple) {
    GClueLocation *location;

    location = gclue_simple_get_location(simple);

    if (quiet_mode) return;

    g_print("Accuracy: %f meters\n", gclue_location_get_accuracy(location));

    if (verbose_mode) {
        g_print("Latitude: %f°\nLongitude: %f°\n",
                gclue_location_get_latitude(location),
                gclue_location_get_longitude(location));
    }
}

static void on_client_active_notify(GClueClient *client, GParamSpec *pspec, gpointer user_data) {
        if (gclue_client_get_active(client)) {
                return;
        }

        g_print("Geolocation disabled. Quitting..\n");
        g_main_loop_quit(main_loop);
}

static void on_simple_ready(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;

    simple = gclue_simple_new_finish(res, &error);
    if (error != NULL) {
        g_critical("Failed to connect to GeoClue2 service: %s", error->message);
        g_error_free(error);
        return;
    }

    if (!GCLUE_IS_SIMPLE(simple)) {
        g_critical("Invalid object returned, expected a GClueSimple instance.");
        return;
    }

    client = gclue_simple_get_client(simple);
    g_object_ref(client);

    if (time_threshold > 0) {
        gclue_client_set_time_threshold(client, time_threshold);
    }

    print_location(simple);

    g_signal_connect(simple, "notify::location", G_CALLBACK(print_location), NULL);
    g_signal_connect(client, "notify::active", G_CALLBACK(on_client_active_notify), NULL);
}

static gboolean toggle_activity(gpointer user_data) {
    is_running = !is_running;

    if (is_running) {
        g_print("Starting gnss check\n");
        gclue_simple_new("geoclue-fix-keeper", accuracy_level, NULL, on_simple_ready, NULL);
    } else {
        g_print("Stopping gnss check\n");

        SAFE_UNREF(simple)
        SAFE_UNREF(client)
    }

    return TRUE;
}

gint main(gint argc, gchar *argv[]) {
    GOptionContext *context;
    GError *error = NULL;

    setlocale(LC_ALL, "");
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    context = g_option_context_new("- hybris GNSS fix keeper");
    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_critical("option parsing failed: %s\n", error->message);
        exit(-1);
    }

    g_option_context_free(context);

    gclue_simple_new("geoclue-fix-keeper", accuracy_level, NULL, on_simple_ready, NULL);

    main_loop = g_main_loop_new(NULL, FALSE);

    g_timeout_add_seconds(toggle_interval, toggle_activity, NULL);

    g_main_loop_run(main_loop);

    SAFE_UNREF(simple)
    SAFE_UNREF(client)

    return EXIT_SUCCESS;
}
