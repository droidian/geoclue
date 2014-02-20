/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2014 Red Hat, Inc.
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

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include "gclue-modem-source.h"

/**
 * SECTION:gclue-modem-source
 * @short_description: Modem-based geolocation
 * @include: gclue-glib/gclue-modem-source.h
 *
 * Baseclass for all sources that use a modem through ModemManager.
 **/

G_DEFINE_ABSTRACT_TYPE (GClueModemSource, gclue_modem_source, GCLUE_TYPE_LOCATION_SOURCE)

struct _GClueModemSourcePrivate {
        MMManager *manager;
        MMObject *mm_object;
        MMModem *modem;
        MMModemLocation *modem_location;

        GCancellable *cancellable;
};

static void
clear_caps (GClueModemSource *source)
{
        GClueModemSourcePrivate *priv = source->priv;
        GError *error = NULL;

        if (priv->modem_location == NULL)
                return;

        /* Remove all the caps */
        if (!mm_modem_location_setup_sync (priv->modem_location,
                                           0,
                                           FALSE,
                                           priv->cancellable,
                                           &error)) {
                g_warning ("Failed to clear caps from modem: %s",
                           error->message);
                g_error_free (error);
        }
}

static void
gclue_modem_source_finalize (GObject *gsource)
{
        GClueModemSource *source = GCLUE_MODEM_SOURCE (gsource);
        GClueModemSourcePrivate *priv = source->priv;

        clear_caps (source);

        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
        g_clear_object (&priv->manager);
        g_clear_object (&priv->mm_object);
        g_clear_object (&priv->modem);
        g_clear_object (&priv->modem_location);

        G_OBJECT_CLASS (gclue_modem_source_parent_class)->finalize (gsource);
}

static void
gclue_modem_source_constructed (GObject *object);

static void
gclue_modem_source_class_init (GClueModemSourceClass *klass)
{
        GObjectClass *gsource_class = G_OBJECT_CLASS (klass);

        gsource_class->finalize = gclue_modem_source_finalize;
        gsource_class->constructed = gclue_modem_source_constructed;

        g_type_class_add_private (klass, sizeof (GClueModemSourcePrivate));
}

static void
on_location_changed (GObject    *gobject,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        GClueModemSource *source = GCLUE_MODEM_SOURCE (user_data);
        GClueModemSourceClass *klass = GCLUE_MODEM_SOURCE_GET_CLASS (source);

        klass->modem_location_changed (source, source->priv->modem_location);
}

static void
on_modem_location_setup (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        GClueModemSourcePrivate *priv = GCLUE_MODEM_SOURCE (user_data)->priv;
        GError *error = NULL;

        if (!mm_modem_location_setup_finish (priv->modem_location, res, &error)) {
                g_warning ("Failed to setup modem: %s", error->message);
                g_error_free (error);

                return;
        }

        on_location_changed (G_OBJECT (priv->modem_location), NULL, user_data);
}

static void
on_modem_enabled (GObject      *source_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
        GClueModemSource *source= GCLUE_MODEM_SOURCE (user_data);
        GClueModemSourcePrivate *priv = source->priv;
        MMModemLocationSource caps;
        GError *error = NULL;

        if (!mm_modem_enable_finish (priv->modem, res, &error)) {
                if (error->code == MM_CORE_ERROR_IN_PROGRESS)
                        /* Seems another source instance is already on it */
                        g_signal_connect (G_OBJECT (priv->modem_location),
                                          "notify::location",
                                          G_CALLBACK (on_location_changed),
                                          user_data);
                else
                        g_warning ("Failed to enable modem: %s", error->message);
                g_error_free (error);

                return;
        }

        g_signal_connect (G_OBJECT (priv->modem_location),
                          "notify::location",
                          G_CALLBACK (on_location_changed),
                          user_data);

        caps = mm_modem_location_get_capabilities (priv->modem_location);

        mm_modem_location_setup (priv->modem_location,
                                 caps,
                                 TRUE,
                                 priv->cancellable,
                                 on_modem_location_setup,
                                 user_data);
}

static void
on_mm_object_added (GDBusObjectManager *manager,
                    GDBusObject        *object,
                    gpointer            user_data)
{
        MMObject *mm_object = MM_OBJECT (object);
        GClueModemSource *source= GCLUE_MODEM_SOURCE (user_data);
        GClueModemSourceClass *klass = GCLUE_MODEM_SOURCE_GET_CLASS (source);
        MMModemLocation *modem_location;
        MMModemLocationSource req_caps, caps;
        const char *caps_name;

        if (source->priv->mm_object != NULL)
                return;

        g_debug ("New modem '%s'", mm_object_get_path (mm_object));
        modem_location = mm_object_peek_modem_location (mm_object);
        if (modem_location == NULL)
                return;

        g_debug ("Modem '%s' has location capabilities",
                 mm_object_get_path (mm_object));
        caps = mm_modem_location_get_capabilities (modem_location);
        req_caps = klass->get_req_modem_location_caps (source, &caps_name);
        if ((caps & req_caps) == 0)
                return;

        g_debug ("Modem '%s' has %s capabilities",
                 mm_object_get_path (mm_object),
                 caps_name);
        source->priv->mm_object = g_object_ref (mm_object);
        source->priv->modem = mm_object_get_modem (mm_object);
        source->priv->modem_location = mm_object_get_modem_location (mm_object);

        mm_modem_enable (source->priv->modem,
                         source->priv->cancellable,
                         on_modem_enabled,
                         user_data);
}

static void
on_mm_object_removed (GDBusObjectManager *manager,
                      GDBusObject        *object,
                      gpointer            user_data)
{
        MMObject *mm_object = MM_OBJECT (object);
        GClueModemSourcePrivate *priv = GCLUE_MODEM_SOURCE (user_data)->priv;

        if (priv->mm_object == NULL || priv->mm_object != mm_object)
                return;

        g_clear_object (&priv->mm_object);
        g_clear_object (&priv->modem);
        g_clear_object (&priv->modem_location);
}

static void
on_manager_new_ready (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
        GClueModemSourcePrivate *priv = GCLUE_MODEM_SOURCE (user_data)->priv;
        GList *objects, *node;
        GError *error = NULL;

        priv->manager = mm_manager_new_finish (res, &error);
        if (priv->manager == NULL) {
                g_warning ("Failed to connect to ModemManager: %s",
                           error->message);
                g_error_free (error);

                return;
        }

        objects = g_dbus_object_manager_get_objects
                        (G_DBUS_OBJECT_MANAGER (priv->manager));
        for (node = objects; node != NULL; node = node->next) {
                on_mm_object_added (G_DBUS_OBJECT_MANAGER (priv->manager),
                                    G_DBUS_OBJECT (node->data),
                                    user_data);

                /* FIXME: Currently we only support 1 modem device */
                if (priv->modem != NULL)
                        break;
        }
        g_list_free_full (objects, g_object_unref);

        g_signal_connect (G_OBJECT (priv->manager),
                          "object-added",
                          G_CALLBACK (on_mm_object_added),
                          user_data);

        g_signal_connect (G_OBJECT (priv->manager),
                          "object-removed",
                          G_CALLBACK (on_mm_object_removed),
                          user_data);
}

static void
on_bus_get_ready (GObject      *source_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
        GClueModemSourcePrivate *priv = GCLUE_MODEM_SOURCE (user_data)->priv;
        GDBusConnection *connection;
        GError *error = NULL;

        connection = g_bus_get_finish (res, &error);
        if (connection == NULL) {
                g_warning ("Failed to connect to system D-Bus: %s",
                           error->message);
                g_error_free (error);

                return;
        }

        mm_manager_new (connection,
                        0,
                        priv->cancellable,
                        on_manager_new_ready,
                        user_data);
}

static void
gclue_modem_source_constructed (GObject *object)
{
        GClueModemSourcePrivate *priv = GCLUE_MODEM_SOURCE (object)->priv;

        priv->cancellable = g_cancellable_new ();

        g_bus_get (G_BUS_TYPE_SYSTEM,
                   priv->cancellable,
                   on_bus_get_ready,
                   object);
}

static void
gclue_modem_source_init (GClueModemSource *modem)
{
        modem->priv = G_TYPE_INSTANCE_GET_PRIVATE ((modem), GCLUE_TYPE_MODEM_SOURCE, GClueModemSourcePrivate);
}
