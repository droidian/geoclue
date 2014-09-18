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
#include <libmm-glib.h>
#include "gclue-modem.h"
#include "gclue-marshal.h"

/**
 * SECTION:gclue-modem
 * @short_description: Modem handler
 * @include: gclue-glib/gclue-modem.h
 *
 * This class is used by GClue3G and GClueModemGPS to deal with modem through
 * ModemManager.
 **/

G_DEFINE_TYPE (GClueModem, gclue_modem, G_TYPE_OBJECT)

struct _GClueModemPrivate {
        MMManager *manager;
        MMObject *mm_object;
        MMModem *modem;
        MMModemLocation *modem_location;
        MMLocation3gpp *location_3gpp;

        GCancellable *cancellable;

        MMModemLocationSource caps; /* Caps we set or are going to set */
};

enum
{
        PROP_0,
        PROP_IS_3G_AVAILABLE,
        PROP_IS_GPS_AVAILABLE,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

enum {
        FIX_3G,
        FIX_GPS,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static void
gclue_modem_finalize (GObject *gmodem)
{
        GClueModem *modem = GCLUE_MODEM (gmodem);
        GClueModemPrivate *priv = modem->priv;

        G_OBJECT_CLASS (gclue_modem_parent_class)->finalize (gmodem);

        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
        g_clear_object (&priv->manager);
        g_clear_object (&priv->mm_object);
        g_clear_object (&priv->modem);
        g_clear_object (&priv->modem_location);
}

static void
gclue_modem_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
        GClueModem *modem = GCLUE_MODEM (object);

        switch (prop_id) {
        case PROP_IS_3G_AVAILABLE:
                g_value_set_boolean (value,
                                     gclue_modem_get_is_3g_available (modem));
                break;

        case PROP_IS_GPS_AVAILABLE:
                g_value_set_boolean (value,
                                     gclue_modem_get_is_gps_available (modem));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_modem_constructed (GObject *object);

static void
gclue_modem_class_init (GClueModemClass *klass)
{
        GObjectClass *gmodem_class = G_OBJECT_CLASS (klass);

        gmodem_class->get_property = gclue_modem_get_property;
        gmodem_class->finalize = gclue_modem_finalize;
        gmodem_class->constructed = gclue_modem_constructed;

        g_type_class_add_private (klass, sizeof (GClueModemPrivate));

        gParamSpecs[PROP_IS_3G_AVAILABLE] =
                                g_param_spec_boolean ("is-3g-available",
                                                      "Is3GAvailable",
                                                      "Is 3G Available?",
                                                      FALSE,
                                                      G_PARAM_READABLE);
        g_object_class_install_property (gmodem_class,
                                         PROP_IS_3G_AVAILABLE,
                                         gParamSpecs[PROP_IS_3G_AVAILABLE]);
        gParamSpecs[PROP_IS_GPS_AVAILABLE] =
                                g_param_spec_boolean ("is-gps-available",
                                                      "IsGPSAvailable",
                                                      "Is GPS Available?",
                                                      FALSE,
                                                      G_PARAM_READABLE);
        g_object_class_install_property (gmodem_class,
                                         PROP_IS_GPS_AVAILABLE,
                                         gParamSpecs[PROP_IS_GPS_AVAILABLE]);

        signals[FIX_3G] =
                g_signal_new ("fix-3g",
                              GCLUE_TYPE_MODEM,
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL,
                              NULL,
                              gclue_marshal_VOID__UINT_UINT_ULONG_ULONG,
                              G_TYPE_NONE,
                              4,
                              G_TYPE_UINT,
                              G_TYPE_UINT,
                              G_TYPE_ULONG,
                              G_TYPE_ULONG);

        signals[FIX_GPS] =
                g_signal_new ("fix-gps",
                              GCLUE_TYPE_MODEM,
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
}

static gboolean
is_location_3gpp_same (GClueModem *modem,
                       guint       new_mcc,
                       guint       new_mnc,
                       gulong      new_lac,
                       gulong      new_cell_id)
{
        GClueModemPrivate *priv = modem->priv;
        guint mcc, mnc;
        gulong lac, cell_id;

        if (priv->location_3gpp == NULL)
                return FALSE;

        mcc = mm_location_3gpp_get_mobile_country_code (priv->location_3gpp);
        mnc = mm_location_3gpp_get_mobile_network_code (priv->location_3gpp);
        lac = mm_location_3gpp_get_location_area_code (priv->location_3gpp);
        cell_id = mm_location_3gpp_get_cell_id (priv->location_3gpp);

        return (mcc == new_mcc &&
                mnc == new_mnc &&
                lac == new_lac &&
                cell_id == new_cell_id);
}

static void
on_get_3gpp_ready (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
        GClueModem *modem = GCLUE_MODEM (user_data);
        GClueModemPrivate *priv = modem->priv;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        MMLocation3gpp *location_3gpp;
        GError *error = NULL;
        guint mcc, mnc;
        gulong lac, cell_id;

        location_3gpp = mm_modem_location_get_3gpp_finish (modem_location,
                                                           res,
                                                           &error);
        if (error != NULL) {
                g_warning ("Failed to get location from 3GPP: %s",
                           error->message);
                g_error_free (error);
                return;
        }

        if (location_3gpp == NULL) {
                g_debug ("No 3GPP");
                return;
        }

        mcc = mm_location_3gpp_get_mobile_country_code (location_3gpp);
        mnc = mm_location_3gpp_get_mobile_network_code (location_3gpp);
        lac = mm_location_3gpp_get_location_area_code (location_3gpp);
        cell_id = mm_location_3gpp_get_cell_id (location_3gpp);

        if (is_location_3gpp_same (modem, mcc, mnc, lac, cell_id)) {
                g_debug ("New 3GPP location is same as last one");
                return;
        }
        g_clear_object (&priv->location_3gpp);
        priv->location_3gpp = location_3gpp;

        g_signal_emit (modem, signals[FIX_3G], 0, mcc, mnc, lac, cell_id);
}

static void
on_get_gps_nmea_ready (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        MMLocationGpsNmea *location_nmea;
        const char *gga;
        GError *error = NULL;

        location_nmea = mm_modem_location_get_gps_nmea_finish (modem_location,
                                                               res,
                                                               &error);
        if (error != NULL) {
                g_warning ("Failed to get location from NMEA information: %s",
                           error->message);
                g_error_free (error);
                return;
        }

        if (location_nmea == NULL) {
                g_debug ("No NMEA");
                return;
        }

        gga = mm_location_gps_nmea_get_trace (location_nmea, "$GPGGA");
        if (gga == NULL) {
                g_debug ("No GGA trace");
                goto out;
        }

        g_debug ("New GPGGA trace: %s", gga);
        g_signal_emit (GCLUE_MODEM (user_data), signals[FIX_GPS], 0, gga);
out:
        g_object_unref (location_nmea);
}

static void
on_location_changed (GObject    *modem_object,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        MMModemLocation *modem_location = MM_MODEM_LOCATION (modem_object);
        GClueModem *modem = GCLUE_MODEM (user_data);

        mm_modem_location_get_3gpp (modem_location,
                                    modem->priv->cancellable,
                                    on_get_3gpp_ready,
                                    modem);
        if ((modem->priv->caps & MM_MODEM_LOCATION_SOURCE_GPS_NMEA) != 0)
                mm_modem_location_get_gps_nmea (modem_location,
                                                modem->priv->cancellable,
                                                on_get_gps_nmea_ready,
                                                modem);
}

static void
on_modem_location_setup (GObject      *modem_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        GTask *task = G_TASK (user_data);
        GClueModem *modem;
        GClueModemPrivate *priv;
        GError *error = NULL;

        if (!mm_modem_location_setup_finish (MM_MODEM_LOCATION (modem_object),
                                             res,
                                             &error)) {
                g_task_return_error (task, error);

                goto out;
        }
        modem = GCLUE_MODEM (g_task_get_source_object (task));
        priv = modem->priv;
        g_debug ("Modem '%s' setup.", mm_object_get_path (priv->mm_object));

        on_location_changed (modem_object, NULL, modem);

        g_task_return_boolean (task, TRUE);
out:
        g_object_unref (task);
}

static void
on_modem_enabled (GObject      *modem_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
        GTask *task = G_TASK (user_data);
        GClueModemPrivate *priv;
        MMModemLocationSource caps;
        GError *error = NULL;

        if (!mm_modem_enable_finish (MM_MODEM (modem_object), res, &error)) {
                if (error->code == MM_CORE_ERROR_IN_PROGRESS)
                        /* Seems a previous async call hasn't returned yet. */
                        g_task_return_boolean (task, TRUE);
                else
                        g_task_return_error (task, error);
                g_object_unref (task);

                return;
        }
        priv = GCLUE_MODEM (g_task_get_source_object (task))->priv;
        g_debug ("modem '%s' enabled.", mm_object_get_path (priv->mm_object));

        caps = mm_modem_location_get_enabled (priv->modem_location) | priv->caps;
        mm_modem_location_setup (priv->modem_location,
                                 caps,
                                 TRUE,
                                 g_task_get_cancellable (task),
                                 on_modem_location_setup,
                                 task);
}

static void
enable_caps (GClueModem           *modem,
             MMModemLocationSource caps,
             GCancellable         *cancellable,
             GAsyncReadyCallback   callback,
             gpointer              user_data)
{
        GClueModemPrivate *priv = modem->priv;
        GTask *task;

        priv->caps |= caps;
        task = g_task_new (modem, cancellable, callback, user_data);

        mm_modem_enable (priv->modem,
                         cancellable,
                         on_modem_enabled,
                         task);
}

static gboolean
enable_caps_finish (GClueModem   *modem,
                    GAsyncResult *result,
                    GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM (modem), FALSE);
        g_return_val_if_fail (g_task_is_valid (result, modem), FALSE);

        return g_task_propagate_boolean (G_TASK (result), error);
}

static gboolean
clear_caps (GClueModem           *modem,
            MMModemLocationSource caps,
            GCancellable         *cancellable,
            GError              **error)
{
        GClueModemPrivate *priv;

        priv = modem->priv;

        if (priv->modem_location == NULL)
                return TRUE;

        priv->caps &= ~caps;

        return mm_modem_location_setup_sync (priv->modem_location,
                                             priv->caps,
                                             FALSE,
                                             cancellable,
                                             error);
}

static gboolean
modem_has_caps (GClueModem           *modem,
                MMModemLocationSource caps)
{
        MMModemLocation *modem_location = modem->priv->modem_location;
        MMModemLocationSource avail_caps;

        if (modem_location == NULL)
                return FALSE;

        avail_caps = mm_modem_location_get_capabilities (modem_location);

        return ((caps & avail_caps) != 0);
}

static void
on_mm_object_added (GDBusObjectManager *manager,
                    GDBusObject        *object,
                    gpointer            user_data)
{
        MMObject *mm_object = MM_OBJECT (object);
        GClueModem *modem = GCLUE_MODEM (user_data);
        MMModemLocation *modem_location;

        if (modem->priv->mm_object != NULL)
                return;

        g_debug ("New modem '%s'", mm_object_get_path (mm_object));
        modem_location = mm_object_peek_modem_location (mm_object);
        if (modem_location == NULL)
                return;

        g_debug ("Modem '%s' has location capabilities",
                 mm_object_get_path (mm_object));

        modem->priv->mm_object = g_object_ref (mm_object);
        modem->priv->modem = mm_object_get_modem (mm_object);
        modem->priv->modem_location = mm_object_get_modem_location (mm_object);

        g_signal_connect (G_OBJECT (modem->priv->modem_location),
                          "notify::location",
                          G_CALLBACK (on_location_changed),
                          modem);

        g_object_notify_by_pspec (G_OBJECT (modem), gParamSpecs[PROP_IS_3G_AVAILABLE]);
        g_object_notify_by_pspec (G_OBJECT (modem), gParamSpecs[PROP_IS_GPS_AVAILABLE]);
}

static void
on_mm_object_removed (GDBusObjectManager *manager,
                      GDBusObject        *object,
                      gpointer            user_data)
{
        MMObject *mm_object = MM_OBJECT (object);
        GClueModem *modem = GCLUE_MODEM (user_data);
        GClueModemPrivate *priv = modem->priv;

        if (priv->mm_object == NULL || priv->mm_object != mm_object)
                return;
        g_debug ("Modem '%s' removed.", mm_object_get_path (priv->mm_object));

        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->modem_location),
                                              G_CALLBACK (on_location_changed),
                                              user_data);
        g_clear_object (&priv->mm_object);
        g_clear_object (&priv->modem);
        g_clear_object (&priv->modem_location);

        g_object_notify_by_pspec (G_OBJECT (modem), gParamSpecs[PROP_IS_3G_AVAILABLE]);
        g_object_notify_by_pspec (G_OBJECT (modem), gParamSpecs[PROP_IS_GPS_AVAILABLE]);
}

static void
on_manager_new_ready (GObject      *modem_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
        GClueModemPrivate *priv = GCLUE_MODEM (user_data)->priv;
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
on_bus_get_ready (GObject      *modem_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
        GClueModemPrivate *priv = GCLUE_MODEM (user_data)->priv;
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
gclue_modem_constructed (GObject *object)
{
        GClueModemPrivate *priv = GCLUE_MODEM (object)->priv;

        G_OBJECT_CLASS (gclue_modem_parent_class)->constructed (object);

        priv->cancellable = g_cancellable_new ();

        g_bus_get (G_BUS_TYPE_SYSTEM,
                   priv->cancellable,
                   on_bus_get_ready,
                   object);
}

static void
gclue_modem_init (GClueModem *modem)
{
        modem->priv = G_TYPE_INSTANCE_GET_PRIVATE ((modem), GCLUE_TYPE_MODEM, GClueModemPrivate);
}

static void
on_modem_destroyed (gpointer data,
                    GObject *where_the_object_was)
{
        GClueModem **modem = (GClueModem **) data;

        *modem = NULL;
}

/**
 * gclue_modem_get_singleton:
 *
 * Get the #GClueModem singleton.
 *
 * Returns: (transfer full): a #GClueModem.
 **/
GClueModem *
gclue_modem_get_singleton (void)
{
        static GClueModem *modem = NULL;

        if (modem == NULL) {
                modem = g_object_new (GCLUE_TYPE_MODEM, NULL);
                g_object_weak_ref (G_OBJECT (modem),
                                   on_modem_destroyed,
                                   &modem);
        } else
                g_object_ref (modem);

        return modem;
}

gboolean
gclue_modem_get_is_3g_available (GClueModem *modem)
{
        return modem_has_caps (modem, MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI);
}

gboolean
gclue_modem_get_is_gps_available (GClueModem *modem)
{
        return modem_has_caps (modem, MM_MODEM_LOCATION_SOURCE_GPS_NMEA);
}

void
gclue_modem_enable_3g (GClueModem         *modem,
                       GCancellable       *cancellable,
                       GAsyncReadyCallback callback,
                       gpointer            user_data)
{
        g_return_if_fail (GCLUE_IS_MODEM (modem));
        g_return_if_fail (gclue_modem_get_is_3g_available (modem));

        enable_caps (modem,
                     MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI,
                     cancellable,
                     callback,
                     user_data);
}

gboolean
gclue_modem_enable_3g_finish (GClueModem   *modem,
                              GAsyncResult *result,
                              GError      **error)
{
        return enable_caps_finish (modem, result, error);
}

void
gclue_modem_enable_gps (GClueModem         *modem,
                        GCancellable       *cancellable,
                        GAsyncReadyCallback callback,
                        gpointer            user_data)
{
        g_return_if_fail (GCLUE_IS_MODEM (modem));
        g_return_if_fail (gclue_modem_get_is_gps_available (modem));

        enable_caps (modem,
                     MM_MODEM_LOCATION_SOURCE_GPS_NMEA,
                     cancellable,
                     callback,
                     user_data);
}

gboolean
gclue_modem_enable_gps_finish (GClueModem   *modem,
                               GAsyncResult *result,
                               GError      **error)
{
        return enable_caps_finish (modem, result, error);
}

gboolean
gclue_modem_disable_3g (GClueModem   *modem,
                        GCancellable *cancellable,
                        GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM (modem), FALSE);
        g_return_val_if_fail (gclue_modem_get_is_3g_available (modem), FALSE);

        g_clear_object (&modem->priv->location_3gpp);
        g_debug ("Clearing 3GPP location caps from modem");
        return clear_caps (modem,
                           MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI,
                           cancellable,
                           error);
}

gboolean
gclue_modem_disable_gps (GClueModem   *modem,
                         GCancellable *cancellable,
                         GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM (modem), FALSE);
        g_return_val_if_fail (gclue_modem_get_is_gps_available (modem), FALSE);

        g_debug ("Clearing GPS NMEA caps from modem");
        return clear_caps (modem,
                           MM_MODEM_LOCATION_SOURCE_GPS_NMEA,
                           cancellable,
                           error);
}
