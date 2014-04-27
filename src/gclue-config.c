/* vim: set et ts=8 sw=8: */
/* gclue-config.c
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
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <glib/gi18n.h>

#include "gclue-config.h"

#define CONFIG_FILE_PATH SYSCONFDIR "/geoclue/geoclue.conf"

/* This class will be responsible for fetching configuration. */

G_DEFINE_TYPE (GClueConfig, gclue_config, G_TYPE_OBJECT)

struct _GClueConfigPrivate
{
        GKeyFile *key_file;
};

static void
gclue_config_finalize (GObject *object)
{
        GClueConfigPrivate *priv;

        priv = GCLUE_CONFIG (object)->priv;

        g_clear_pointer (&priv->key_file, g_key_file_unref);

        G_OBJECT_CLASS (gclue_config_parent_class)->finalize (object);
}

static void
gclue_config_class_init (GClueConfigClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_config_finalize;
        g_type_class_add_private (object_class, sizeof (GClueConfigPrivate));
}

static void
gclue_config_init (GClueConfig *config)
{
        GError *error = NULL;

        config->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (config,
                                            GCLUE_TYPE_CONFIG,
                                            GClueConfigPrivate);
        config->priv->key_file = g_key_file_new ();
        g_key_file_load_from_file (config->priv->key_file,
                                   CONFIG_FILE_PATH,
                                   0,
                                   &error);
        if (error != NULL) {
                g_critical ("Failed to load configuration file '%s': %s",
                            CONFIG_FILE_PATH, error->message);
                g_error_free (error);
        }
}

GClueConfig *
gclue_config_get_singleton (void)
{
        static GClueConfig *config = NULL;

        if (config == NULL)
                config = g_object_new (GCLUE_TYPE_CONFIG, NULL);

        return config;
}

gboolean
gclue_config_is_agent_allowed (GClueConfig     *config,
                               const char      *desktop_id,
                               GClueClientInfo *agent_info)
{
        GClueConfigPrivate *priv = config->priv;
        char **agents;
        gsize num_agents, i;
        gboolean allowed = FALSE;
        GError *error = NULL;

        agents = g_key_file_get_string_list (priv->key_file,
                                             "agent",
                                             "whitelist",
                                             &num_agents,
                                             &error);
        if (error != NULL) {
                g_critical ("Failed to read 'agent/whitelist' key: %s",
                            error->message);
                g_error_free (error);

                return FALSE;
        }

        for (i = 0; i < num_agents; i++) {
                if (g_strcmp0 (desktop_id, agents[i]) == 0) {
                        allowed = TRUE;

                        break;
                }
        }
        g_strfreev (agents);

        return allowed;
}

gboolean
gclue_config_is_app_allowed (GClueConfig     *config,
                             const char      *desktop_id,
                             GClueClientInfo *app_info)
{
        GClueConfigPrivate *priv = config->priv;
        int* users = NULL;
        guint64 uid;
        gsize num_users, i;
        gboolean allowed = FALSE;
        GError *error = NULL;

        g_return_val_if_fail (desktop_id != NULL, FALSE);

        allowed = g_key_file_get_boolean (priv->key_file,
                                          desktop_id,
                                          "allowed",
                                          &error);
        if (error != NULL || !allowed) {
                g_debug ("'%s' not in configuration or not allowed", desktop_id);
                goto out;
        }

        uid = gclue_client_info_get_user_id (app_info);
        users = g_key_file_get_integer_list (priv->key_file,
                                             desktop_id,
                                             "users",
                                             &num_users,
                                             &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                goto out;
        }
        if (num_users == 0) {
                allowed = TRUE;
                goto out;
        }

        for (i = 0; i < num_users; i++) {
                if (users[i] == uid) {
                        allowed = TRUE;

                        break;
                }
        }

out:
        g_clear_pointer (&users, g_free);
        g_clear_error (&error);

        return allowed;
}

#define DEFAULT_WIFI_URL "https://location.services.mozilla.com/v1/geolocate?key=geoclue"

char *
gclue_config_get_wifi_url (GClueConfig *config)
{
        char *url;
        GError *error = NULL;

        url = g_key_file_get_string (config->priv->key_file,
                                     "wifi",
                                     "url",
                                     &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                g_error_free (error);
                url = g_strdup (DEFAULT_WIFI_URL);
        }

        return url;
}

char *
gclue_config_get_wifi_submit_url (GClueConfig *config)
{
        char *url;
        GError *error = NULL;

        url = g_key_file_get_string (config->priv->key_file,
                                     "wifi",
                                     "submission-url",
                                     &error);
        if (error != NULL) {
                g_debug ("No wifi submission URL: %s", error->message);
                g_error_free (error);
        }

        return url;
}

char *
gclue_config_get_wifi_submit_nick (GClueConfig *config)
{
        char *nick;
        GError *error = NULL;

        nick = g_key_file_get_string (config->priv->key_file,
                                     "wifi",
                                     "submission-nick",
                                     &error);
        if (error != NULL) {
                g_debug ("No wifi submission nick: %s", error->message);
                g_error_free (error);
        }

        return nick;
}
