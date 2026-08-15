/* vim: set et ts=8 sw=8: */
/*
 * Copyright 2014 Red Hat, Inc.
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
#include "gclue-modem-manager.h"
#include "gclue-nmea-utils.h"
#include "gclue-marshal.h"
#include "gclue-3g-tower.h"

/**
 * SECTION:gclue-modem-manager
 * @short_description: Modem handler
 *
 * This class is used by GClue3G and GClueModemGPS to deal with modem through
 * ModemManager.
 **/

static void
gclue_modem_interface_init (GClueModemInterface *iface);

struct _GClueModemManagerPrivate {
        MMManager *manager;

        GHashTable *modems_not_enabled;

        MMObject *mm_object;
        MMModem *modem;
        MMModemLocation *modem_location;
        MMLocation3gpp *location_3gpp;
        gboolean location_3gpp_ignore_previous;
        MMLocationGpsNmea *location_nmea;

        GCancellable *cancellable;

        MMModemLocationSource caps; /* Caps we set or are going to set */
        GClueTowerTec tec;

        guint time_threshold;
};

G_DEFINE_TYPE_WITH_CODE (GClueModemManager, gclue_modem_manager, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_MODEM,
                                                gclue_modem_interface_init)
                         G_ADD_PRIVATE (GClueModemManager))

enum
{
        PROP_0,
        PROP_IS_3G_AVAILABLE,
        PROP_IS_CDMA_AVAILABLE,
        PROP_IS_GPS_AVAILABLE,
        PROP_TIME_THRESHOLD,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

enum {
        FIX_3G,
        FIX_CDMA,
        FIX_GPS,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static gboolean
gclue_modem_manager_get_is_3g_available (GClueModem *modem);
static gboolean
gclue_modem_manager_get_is_cdma_available (GClueModem *modem);
static gboolean
gclue_modem_manager_get_is_gps_available (GClueModem *modem);
static guint
gclue_modem_manager_get_time_threshold (GClueModem *modem);
static void
gclue_modem_manager_set_time_threshold (GClueModem *modem,
                                        guint       time_threshold);
static void
gclue_modem_manager_enable_3g (GClueModem         *modem,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data);
static gboolean
gclue_modem_manager_enable_3g_finish (GClueModem   *modem,
                                      GAsyncResult *result,
                                      GError      **error);
static void
gclue_modem_manager_enable_cdma (GClueModem         *modem,
                                 GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data);
static gboolean
gclue_modem_manager_enable_cdma_finish (GClueModem   *modem,
                                        GAsyncResult *result,
                                        GError      **error);
static void
gclue_modem_manager_enable_gps (GClueModem         *modem,
                                GCancellable       *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer            user_data);
static gboolean
gclue_modem_manager_enable_gps_finish (GClueModem   *modem,
                                       GAsyncResult *result,
                                       GError      **error);
static gboolean
gclue_modem_manager_disable_3g (GClueModem   *modem,
                                GCancellable *cancellable,
                                GError      **error);
static gboolean
gclue_modem_manager_disable_cdma (GClueModem   *modem,
                                  GCancellable *cancellable,
                                  GError      **error);
static gboolean
gclue_modem_manager_disable_gps (GClueModem   *modem,
                                 GCancellable *cancellable,
                                 GError      **error);

static void
gclue_modem_manager_finalize (GObject *gmodem)
{
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (gmodem);
        GClueModemManagerPrivate *priv = manager->priv;

        G_OBJECT_CLASS (gclue_modem_manager_parent_class)->finalize (gmodem);

        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
        g_clear_object (&priv->manager);
        g_clear_object (&priv->mm_object);
        g_clear_object (&priv->modem);
        g_clear_object (&priv->modem_location);
        g_clear_pointer (&priv->modems_not_enabled, g_hash_table_unref);
}

static void
gclue_modem_manager_get_property (GObject    *object,
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

        case PROP_IS_CDMA_AVAILABLE:
                g_value_set_boolean (value,
                                     gclue_modem_get_is_cdma_available (modem));
                break;

        case PROP_IS_GPS_AVAILABLE:
                g_value_set_boolean (value,
                                     gclue_modem_get_is_gps_available (modem));
                break;

        case PROP_TIME_THRESHOLD:
                g_value_set_uint (value,
                                  gclue_modem_get_time_threshold (modem));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_modem_manager_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        GClueModem *modem = GCLUE_MODEM (object);

        switch (prop_id) {
        case PROP_TIME_THRESHOLD:
                gclue_modem_set_time_threshold (modem,
                                                g_value_get_uint (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_modem_manager_constructed (GObject *object);

static void
gclue_modem_manager_class_init (GClueModemManagerClass *klass)
{
        GObjectClass *gmodem_class = G_OBJECT_CLASS (klass);

        gmodem_class->get_property = gclue_modem_manager_get_property;
        gmodem_class->set_property = gclue_modem_manager_set_property;
        gmodem_class->finalize = gclue_modem_manager_finalize;
        gmodem_class->constructed = gclue_modem_manager_constructed;

        g_object_class_override_property (gmodem_class,
                                          PROP_IS_3G_AVAILABLE,
                                          "is-3g-available");
        gParamSpecs[PROP_IS_3G_AVAILABLE] =
                        g_object_class_find_property (gmodem_class,
                                                      "is-3g-available");
        g_object_class_override_property (gmodem_class,
                                          PROP_IS_CDMA_AVAILABLE,
                                          "is-cdma-available");
        gParamSpecs[PROP_IS_CDMA_AVAILABLE] =
                        g_object_class_find_property (gmodem_class,
                                                      "is-cdma-available");
        g_object_class_override_property (gmodem_class,
                                          PROP_IS_GPS_AVAILABLE,
                                          "is-gps-available");
        gParamSpecs[PROP_IS_GPS_AVAILABLE] =
                        g_object_class_find_property (gmodem_class,
                                                      "is-gps-available");
        g_object_class_override_property (gmodem_class,
                                          PROP_TIME_THRESHOLD,
                                          "time-threshold");
        gParamSpecs[PROP_TIME_THRESHOLD] =
                        g_object_class_find_property (gmodem_class,
                                                      "time-threshold");

        signals[FIX_3G] = g_signal_lookup ("fix-3g", GCLUE_TYPE_MODEM);
        signals[FIX_CDMA] = g_signal_lookup ("fix-cdma", GCLUE_TYPE_MODEM);
        signals[FIX_GPS] = g_signal_lookup ("fix-gps", GCLUE_TYPE_MODEM);
}

static void
gclue_modem_interface_init (GClueModemInterface *iface)
{
        iface->get_is_3g_available = gclue_modem_manager_get_is_3g_available;
        iface->get_is_cdma_available = gclue_modem_manager_get_is_cdma_available;
        iface->get_is_gps_available = gclue_modem_manager_get_is_gps_available;
        iface->get_time_threshold = gclue_modem_manager_get_time_threshold;
        iface->set_time_threshold = gclue_modem_manager_set_time_threshold;
        iface->enable_3g = gclue_modem_manager_enable_3g;
        iface->enable_3g_finish = gclue_modem_manager_enable_3g_finish;
        iface->enable_cdma = gclue_modem_manager_enable_cdma;
        iface->enable_cdma_finish = gclue_modem_manager_enable_cdma_finish;
        iface->enable_gps = gclue_modem_manager_enable_gps;
        iface->enable_gps_finish = gclue_modem_manager_enable_gps_finish;
        iface->disable_3g = gclue_modem_manager_disable_3g;
        iface->disable_cdma = gclue_modem_manager_disable_cdma;
        iface->disable_gps = gclue_modem_manager_disable_gps;
}

#if !MM_CHECK_VERSION(1, 18, 0)
static void
opc_from_mccmnc (MMLocation3gpp *location_3gpp,
                 gchar          *opc_buf)
{
        guint mcc, mnc;

        mcc = mm_location_3gpp_get_mobile_country_code (location_3gpp);
        mnc = mm_location_3gpp_get_mobile_network_code (location_3gpp);

        if (mcc < 1000 && mnc < 1000) {
                g_snprintf (opc_buf, GCLUE_3G_TOWER_OPERATOR_CODE_STR_LEN + 1,
                            "%03u%03u", mcc, mnc);
        } else {
                g_warning ("Invalid MCC or MNC value");
                opc_buf[0] = '\0';
        }
}
#endif

static gboolean
is_location_3gpp_same (GClueModemManager *manager,
                       const gchar       *new_opc,
                       gulong             new_lac,
                       gulong             new_cell_id,
                       GClueTowerTec      new_tec)
{
        GClueModemManagerPrivate *priv = manager->priv;
        const gchar *opc;
        gulong lac, cell_id;
#if !MM_CHECK_VERSION(1, 18, 0)
        gchar opc_buf[GCLUE_3G_TOWER_OPERATOR_CODE_STR_LEN + 1];
#endif

        if (priv->location_3gpp == NULL || priv->location_3gpp_ignore_previous)
                return FALSE;

#if MM_CHECK_VERSION(1, 18, 0)
        opc = mm_location_3gpp_get_operator_code (priv->location_3gpp);
#else
        opc_from_mccmnc (priv->location_3gpp, opc_buf);
        opc = opc_buf;
#endif
        lac = mm_location_3gpp_get_location_area_code (priv->location_3gpp);

        // Use the tracking area code in place of the
        // location area code for LTE.
        // https://ichnaea.readthedocs.io/en/latest/api/geolocate.html#cell-tower-fields
        if (priv->tec == GCLUE_TOWER_TEC_4G) {
                lac = mm_location_3gpp_get_tracking_area_code(priv->location_3gpp);
        }

        cell_id = mm_location_3gpp_get_cell_id (priv->location_3gpp);

        return (g_strcmp0 (opc, new_opc) == 0 &&
                lac == new_lac &&
                cell_id == new_cell_id &&
                priv->tec == new_tec);
}

static void clear_3gpp_location (GClueModemManager *manager)
{
        GClueModemManagerPrivate *priv = manager->priv;

        if (!priv->location_3gpp && !priv->location_3gpp_ignore_previous) {
                return;
        }

        g_clear_object (&priv->location_3gpp);
        g_signal_emit (manager, signals[FIX_3G], 0, NULL, 0, 0, GCLUE_TOWER_TEC_NO_FIX);
}

static void
on_get_3gpp_ready (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
        GClueModemManager *manager;
        GClueModemManagerPrivate *priv;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        MMModemAccessTechnology modem_access_tec;
        g_autoptr(MMLocation3gpp) location_3gpp = NULL;
        const gchar *opc;
        gulong lac, cell_id;
        GClueTowerTec tec;
#if !MM_CHECK_VERSION(1, 18, 0)
        g_autoptr(GError) error = NULL;
        gchar opc_buf[GCLUE_3G_TOWER_OPERATOR_CODE_STR_LEN + 1];

        location_3gpp = mm_modem_location_get_3gpp_finish (modem_location,
                                                           res,
                                                           &error);
        if (error != NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                        g_warning ("Failed to get location from 3GPP: %s",
                                   error->message);
                }

                return;
        }
#else
        location_3gpp = mm_modem_location_get_signaled_3gpp (modem_location);
#endif

        manager = GCLUE_MODEM_MANAGER (user_data);
        priv = manager->priv;

        if (location_3gpp == NULL) {
                g_debug ("No 3GPP");
                clear_3gpp_location (manager);
                priv->location_3gpp_ignore_previous = FALSE;
                return;
        }

#if MM_CHECK_VERSION(1, 18, 0)
        opc = mm_location_3gpp_get_operator_code (location_3gpp);
#else
        opc_from_mccmnc (location_3gpp, opc_buf);
        opc = opc_buf;
#endif
        if (!opc || !opc[0])
                return;

        lac = mm_location_3gpp_get_location_area_code (location_3gpp);

        cell_id = mm_location_3gpp_get_cell_id (location_3gpp);

        modem_access_tec = mm_modem_get_access_technologies(priv->modem);

        if (modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_GSM ||
            modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_GPRS ||
            modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_EDGE) {
                tec = GCLUE_TOWER_TEC_2G;
        } else if (modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_UMTS ||
                   modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_HSDPA ||
                   modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_HSUPA ||
                   modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_HSPA ||
                   modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS) {
                tec = GCLUE_TOWER_TEC_3G;
        } else if (modem_access_tec == MM_MODEM_ACCESS_TECHNOLOGY_LTE) {
                lac = mm_location_3gpp_get_tracking_area_code(location_3gpp);
                tec = GCLUE_TOWER_TEC_4G;
        } else {
                tec = GCLUE_TOWER_TEC_UNKNOWN;
        }

        if (is_location_3gpp_same (manager, opc, lac, cell_id, tec)) {
                g_debug ("New 3GPP location is same as last one");
                return;
        }
        g_clear_object (&priv->location_3gpp);
        priv->location_3gpp = g_steal_pointer (&location_3gpp);
        priv->location_3gpp_ignore_previous = FALSE;
        priv->tec = tec;

        g_signal_emit (manager, signals[FIX_3G], 0, opc, lac, cell_id, tec);
}

static void
on_location_changed_get_3gpp (GObject *modem_object,
                              GClueModemManager *manager)
{
#if MM_CHECK_VERSION(1, 18, 0)
        on_get_3gpp_ready(modem_object, NULL, manager);
#else
        mm_modem_location_get_3gpp (MM_MODEM_LOCATION (modem_object),
                                    manager->priv->cancellable,
                                    on_get_3gpp_ready,
                                    manager);
#endif
}

static void
on_get_cdma_ready (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
        GClueModemManager *manager;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        g_autoptr(MMLocationCdmaBs) location_cdma = NULL;
#if !MM_CHECK_VERSION(1, 18, 0)
        g_autoptr(GError) error = NULL;

        location_cdma = mm_modem_location_get_cdma_bs_finish (modem_location,
                                                              res,
                                                              &error);
        if (error != NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                        g_warning ("Failed to get location from CDMA: %s",
                                   error->message);
                }

                return;
        }
#else
        location_cdma = mm_modem_location_get_signaled_cdma_bs (modem_location);
#endif

        manager = GCLUE_MODEM_MANAGER (user_data);

        if (location_cdma == NULL) {
                g_debug ("No CDMA");
                return;
        }

        g_signal_emit (manager,
                       signals[FIX_CDMA],
                       0,
                       mm_location_cdma_bs_get_latitude (location_cdma),
                       mm_location_cdma_bs_get_longitude (location_cdma));
}

static void
on_location_changed_get_cdma (GObject *modem_object,
                              GClueModemManager *manager)
{
#if MM_CHECK_VERSION(1, 18, 0)
        on_get_cdma_ready(modem_object, NULL, manager);
#else
        mm_modem_location_get_cdma_bs (MM_MODEM_LOCATION (modem_object),
                                       manager->priv->cancellable,
                                       on_get_cdma_ready,
                                       manager);
#endif
}

static gboolean
is_location_gga_same (GClueModemManager *manager,
                       const char       *new_gga)
{
        GClueModemManagerPrivate *priv = manager->priv;
        const char *gga;

        if (priv->location_nmea == NULL)
                return FALSE;

        gga = mm_location_gps_nmea_get_trace (priv->location_nmea, "$GPGGA");
        return (g_strcmp0 (gga, new_gga) == 0);
}

static void
on_get_gps_nmea_ready (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
        GClueModemManager *manager;
        GClueModemManagerPrivate *priv;
        MMModemLocation *modem_location = MM_MODEM_LOCATION (source_object);
        g_autoptr(MMLocationGpsNmea) location_nmea = NULL;
        static const gchar *sentences[3];
        const gchar *gga, *rmc;
        gint i = 0;
#if !MM_CHECK_VERSION(1, 18, 0)
        g_autoptr(GError) error = NULL;

        location_nmea = mm_modem_location_get_gps_nmea_finish (modem_location,
                                                               res,
                                                               &error);
        if (error != NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                        g_warning ("Failed to get location from NMEA information: %s",
                                   error->message);
                }

                return;
        }
#else
        location_nmea = mm_modem_location_get_signaled_gps_nmea (modem_location);
#endif

        manager = GCLUE_MODEM_MANAGER (user_data);
        priv = manager->priv;

        if (location_nmea == NULL) {
                g_debug ("No NMEA");
                return;
        }

        gga = mm_location_gps_nmea_get_trace (location_nmea, "$GPGGA");
        if (gga != NULL && gclue_nmea_type_is (gga, "GGA")) {
                if (is_location_gga_same (manager, gga)) {
                        g_debug ("New GGA trace is same as last one: %s", gga);
                        return;
                }
                g_debug ("New GPGGA trace: %s", gga);
                sentences[i++] = gga;
        }
        rmc = mm_location_gps_nmea_get_trace (location_nmea, "$GPRMC");
        if (rmc != NULL && gclue_nmea_type_is (rmc, "RMC")) {
                g_debug ("New GPRMC trace: %s", rmc);
                sentences[i++] = rmc;
        }
        sentences[i] = NULL;

        if (sentences[0] == NULL)
                g_debug ("No GGA or RMC trace");
        else
                g_signal_emit (manager, signals[FIX_GPS], 0, sentences);

        g_clear_object (&priv->location_nmea);
        priv->location_nmea = g_steal_pointer (&location_nmea);
}

static void
on_location_changed_get_gps_nmea (GObject    *modem_object,
                                  GClueModemManager *manager)
{
#if MM_CHECK_VERSION(1, 18, 0)
        on_get_gps_nmea_ready(modem_object, NULL, manager);
#else
        mm_modem_location_get_gps_nmea (MM_MODEM_LOCATION (modem_object),
                                        manager->priv->cancellable,
                                        on_get_gps_nmea_ready,
                                        manager);
#endif
}

static void
on_location_changed (GObject    *modem_object,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (user_data);

        if ((manager->priv->caps & MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI) != 0)
                on_location_changed_get_3gpp (modem_object, manager);
        if ((manager->priv->caps & MM_MODEM_LOCATION_SOURCE_CDMA_BS) != 0)
                on_location_changed_get_cdma (modem_object, manager);
        if ((manager->priv->caps & MM_MODEM_LOCATION_SOURCE_GPS_NMEA) != 0)
                on_location_changed_get_gps_nmea (modem_object, manager);
}

static void
on_modem_location_setup (GObject      *modem_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        GTask *task = G_TASK (user_data);
        GClueModemManager *manager = GCLUE_MODEM_MANAGER
                (g_task_get_source_object (task));
        GClueModemManagerPrivate *priv = manager->priv;
        GError *error = NULL;

        if (!mm_modem_location_setup_finish (MM_MODEM_LOCATION (modem_object),
                                             res,
                                             &error)) {
                g_task_return_error (task, error);

                goto out;
        }

        g_debug ("Modem '%s' setup.", mm_object_get_path (priv->mm_object));

        /* Make sure that we actually emit that signal */
        priv->location_3gpp_ignore_previous = TRUE;
        on_location_changed (modem_object, NULL, manager);

        g_task_return_boolean (task, TRUE);
out:
        g_object_unref (task);
}

static void
enable_caps (GClueModemManager    *manager,
             MMModemLocationSource caps,
             GCancellable         *cancellable,
             GAsyncReadyCallback   callback,
             gpointer              user_data)
{
        GClueModemManagerPrivate *priv = manager->priv;
        GTask *task;

        priv->caps |= caps;
        task = g_task_new (manager, cancellable, callback, user_data);

        caps = mm_modem_location_get_enabled (priv->modem_location) | priv->caps;
        mm_modem_location_setup (priv->modem_location,
                                 caps,
                                 TRUE,
                                 g_task_get_cancellable (task),
                                 on_modem_location_setup,
                                 task);
}

static gboolean
enable_caps_finish (GClueModemManager   *manager,
                    GAsyncResult *result,
                    GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (manager), FALSE);
        g_return_val_if_fail (g_task_is_valid (result, manager), FALSE);

        return g_task_propagate_boolean (G_TASK (result), error);
}

static gboolean
clear_caps (GClueModemManager    *manager,
            MMModemLocationSource caps,
            GCancellable         *cancellable,
            GError              **error)
{
        GClueModemManagerPrivate *priv;

        priv = manager->priv;

        if (priv->modem_location == NULL)
                return TRUE;

        priv->caps &= ~caps;

        return mm_modem_location_setup_sync (priv->modem_location,
                                             priv->caps,
                                             TRUE,
                                             cancellable,
                                             error);
}

static gboolean
modem_has_caps (GClueModemManager    *manager,
                MMModemLocationSource caps)
{
        MMModemLocation *modem_location = manager->priv->modem_location;
        MMModemLocationSource avail_caps;

        if (modem_location == NULL)
                return FALSE;

        avail_caps = mm_modem_location_get_capabilities (modem_location);

        return ((caps & avail_caps) != 0);
}

static void
on_enable_agps_ready (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data)
{
        GClueModemManager *manager;
        g_autoptr(GError) error = NULL;

        g_return_if_fail (GCLUE_IS_MODEM_MANAGER (source));
        manager = GCLUE_MODEM_MANAGER (source);

        if (!enable_caps_finish (manager, result, &error)) {
                g_warning ("Failed to enable assisted GPS: %s", error->message);
                /* Clear AGPS caps so that subsequent calls to enable_caps do not fail */
                manager->priv->caps &= ~(MM_MODEM_LOCATION_SOURCE_AGPS_MSB | MM_MODEM_LOCATION_SOURCE_AGPS_MSA);
        }
}

static void
enable_agps (GClueModemManager *manager)
{
        MMModemLocationSource assistance_caps = MM_MODEM_LOCATION_SOURCE_NONE;

        g_return_if_fail (
                gclue_modem_manager_get_is_gps_available (GCLUE_MODEM (manager)));

        if (manager->priv->modem_location
            && mm_modem_location_get_supl_server (manager->priv->modem_location) != NULL) {
                MMModemLocationSource caps;

                caps = mm_modem_location_get_capabilities (manager->priv->modem_location);
                /* Prefer MSB assistance */
                if (caps & MM_MODEM_LOCATION_SOURCE_AGPS_MSB) {
                        assistance_caps |= MM_MODEM_LOCATION_SOURCE_AGPS_MSB;
                        g_debug ("Enabing MSB assisted GPS");
                } else if (caps & MM_MODEM_LOCATION_SOURCE_AGPS_MSA) {
                        assistance_caps |= MM_MODEM_LOCATION_SOURCE_AGPS_MSA;
                        g_debug ("Enabling MSA assisted GPS");
                }
        }
        if (assistance_caps == MM_MODEM_LOCATION_SOURCE_NONE) {
                g_debug ("Assisted GPS not available");
                return;
        }

        enable_caps (manager,
                     assistance_caps,
                     manager->priv->cancellable,
                     on_enable_agps_ready,
                     manager);
}

static void
disconnect_modem_location (GClueModemManager *manager)
{
        GClueModemManagerPrivate *priv = manager->priv;

        if (!priv->modem_location) {
                return;
        }

        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->modem_location),
                                              G_CALLBACK (on_location_changed),
                                              manager);

        g_clear_object (&priv->modem_location);
}

static void
on_gps_refresh_rate_set (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        g_autoptr(GError) error = NULL;

        mm_modem_location_set_gps_refresh_rate_finish
                (MM_MODEM_LOCATION (source_object), res, &error);
        if (error && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                g_warning ("Failed to set GPS refresh rate: %s",
                           error->message);
                /* TODO: try selecting better modem if GPS is unsupported? */
        }
}

static gboolean
try_modem_location (GClueModemManager *manager,
                    MMObject *mm_object)
{
        const char *path = mm_object_get_path (mm_object);
        g_autoptr(MMModemLocation) modem_location = NULL;

        modem_location = mm_object_get_modem_location (mm_object);
        if (modem_location == NULL) {
                g_debug ("Modem '%s' does not have location capabilities", path);
                return FALSE;
        }

        /* TODO: check that modem actually has some usable capabilities, like GNSS */
        g_debug ("Modem '%s' has location capabilities", path);

        g_assert (!manager->priv->modem_location);
        manager->priv->modem_location = g_object_ref (modem_location);

        mm_modem_location_set_gps_refresh_rate (manager->priv->modem_location,
                                                manager->priv->time_threshold,
                                                manager->priv->cancellable,
                                                on_gps_refresh_rate_set,
                                                NULL);

        g_signal_connect (G_OBJECT (manager->priv->modem_location),
                          "notify::location",
                          G_CALLBACK (on_location_changed),
                          manager);

        return TRUE;
}

static void
try_modem (GClueModemManager *manager,
           MMObject *mm_object,
           MMModem *mm_modem,
           gboolean modem_is_enabled)
{
        const char *path = mm_object_get_path (mm_object);

        if (!try_modem_location (manager,
                                 mm_object)) {
                return;
        }

        manager->priv->mm_object = g_object_ref (mm_object);
        manager->priv->modem = g_object_ref (mm_modem);

        /* Has to be done after setting up manager->priv objects */
        g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_GPS_AVAILABLE]);

        if (modem_is_enabled) {
                g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_3G_AVAILABLE]);
                g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_CDMA_AVAILABLE]);
                enable_agps (manager);
        } else {
                g_debug ("3G or CDMA are not available on non-enabled modem '%s'", path);
        }
}

static void
on_mm_modem_state_notify (GObject    *gobject,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
        MMModem *mm_modem = MM_MODEM (gobject);
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (user_data);
        GClueModemManagerPrivate *priv = manager->priv;
        GDBusObjectManager *obj_manager = G_DBUS_OBJECT_MANAGER (priv->manager);
        const char *path = mm_modem_get_path (mm_modem);
        g_autoptr(MMObject) mm_object = NULL;

        if (mm_modem_get_state (mm_modem) < MM_MODEM_STATE_ENABLED)
                return;

        g_signal_handlers_disconnect_by_func (mm_modem,
                                              on_mm_modem_state_notify,
                                              user_data);

        if (priv->modem != NULL && priv->modem != mm_modem) {
                g_debug ("Ignoring enabled modem '%s' as already have another one",
                         path);
                return;
        }

        g_debug ("Modem '%s' now enabled", path);

        mm_object = MM_OBJECT (g_dbus_object_manager_get_object (obj_manager, path));
        g_assert (mm_object);

        if (priv->mm_object == NULL) {
                try_modem (manager, mm_object, mm_modem, TRUE);
        } else {
                /* MM re-initializes the location interface so we have to re-connect */
                disconnect_modem_location (manager);
                if (!try_modem_location (manager, mm_object)) {
                        /* Notify that sadly GPS is no longer available */
                        g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_GPS_AVAILABLE]);
                        /* TODO: try next modem */
                        return;
                }

                g_debug ("Enabling 3G and CDMA location on modem '%s'", path);
                g_assert (manager->priv->modem_location);
                g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_3G_AVAILABLE]);
                g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_CDMA_AVAILABLE]);
                enable_agps (manager);
        }
}

static void
on_mm_object_added (GDBusObjectManager *object_manager,
                    GDBusObject        *object,
                    gpointer            user_data)
{
        MMObject *mm_object = MM_OBJECT (object);
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (user_data);
        g_autoptr(MMModem) mm_modem = NULL;
        const char *path = mm_object_get_path (mm_object);
        gboolean modem_is_enabled;

        if (manager->priv->mm_object != NULL) {
                g_debug ("New modem '%s' but already have one", path);
                return;
        }

        if (g_hash_table_lookup (manager->priv->modems_not_enabled, path)) {
                g_warning ("New modem '%s' notification for an existing non-enabled modem",
                           path);
                return;
        }

        g_debug ("New modem '%s'", path);
        mm_modem = mm_object_get_modem (mm_object);
        modem_is_enabled = mm_modem_get_state (mm_modem) >= MM_MODEM_STATE_ENABLED;
        if (!modem_is_enabled) {
                g_debug ("Modem '%s' not enabled", path);

                g_hash_table_insert (manager->priv->modems_not_enabled,
                                     g_strdup (path), g_object_ref (mm_modem));

                g_signal_connect_object (mm_modem,
                                         "notify::state",
                                         G_CALLBACK (on_mm_modem_state_notify),
                                         manager,
                                         0);
        }

        try_modem (manager, mm_object, mm_modem, modem_is_enabled);
}

static void
on_mm_object_removed (GDBusObjectManager *object_manager,
                      GDBusObject        *object,
                      gpointer            user_data)
{
        MMObject *mm_object = MM_OBJECT (object);
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (user_data);
        GClueModemManagerPrivate *priv = manager->priv;
        const char *path = mm_object_get_path (mm_object);

        g_hash_table_remove (manager->priv->modems_not_enabled, path);

        if (priv->mm_object == NULL || priv->mm_object != mm_object) {
                g_debug ("Unused modem '%s' removed.", path);
                return;
        }
        g_debug ("Modem '%s' removed.", path);

        clear_3gpp_location (manager);

        disconnect_modem_location (manager);

        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->modem),
                                              on_mm_modem_state_notify,
                                              user_data);

        g_clear_object (&priv->mm_object);
        g_clear_object (&priv->modem);

        priv->caps = 0;

        g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_3G_AVAILABLE]);
        g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_CDMA_AVAILABLE]);
        g_object_notify_by_pspec (G_OBJECT (manager), gParamSpecs[PROP_IS_GPS_AVAILABLE]);

        /* TODO: try next modem */
}

static void
on_manager_new_ready (GObject      *modem_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
        MMManager *mmmanager;
        GClueModemManager *manager;
        GClueModemManagerPrivate *priv;
        GList *objects, *node;
        g_autoptr(GError) error = NULL;

        mmmanager = mm_manager_new_finish (res, &error);
        if (mmmanager == NULL) {
                if (error && !g_error_matches (error, G_IO_ERROR,
                                               G_IO_ERROR_CANCELLED)) {
                        g_warning ("Failed to connect to ModemManager: %s",
                                   error->message);
                }

                return;
        }

        manager = GCLUE_MODEM_MANAGER (user_data);
        priv = manager->priv;
        priv->manager = mmmanager;

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

        g_signal_connect_object (G_OBJECT (priv->manager),
                                 "object-added",
                                 G_CALLBACK (on_mm_object_added),
                                 manager, 0);

        g_signal_connect_object (G_OBJECT (priv->manager),
                                 "object-removed",
                                 G_CALLBACK (on_mm_object_removed),
                                 manager, 0);
}

static void
on_bus_get_ready (GObject      *modem_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
        GClueModemManagerPrivate *priv;
        g_autoptr(GDBusConnection) connection = NULL;
        g_autoptr(GError) error = NULL;

        connection = g_bus_get_finish (res, &error);
        if (connection == NULL) {
                if (error && !g_error_matches (error, G_IO_ERROR,
                                               G_IO_ERROR_CANCELLED)) {
                        g_warning ("Failed to connect to system D-Bus: %s",
                                   error->message);
                }

                return;
        }

        priv = GCLUE_MODEM_MANAGER (user_data)->priv;
        mm_manager_new (connection,
                        0,
                        priv->cancellable,
                        on_manager_new_ready,
                        user_data);
}

static void
gclue_modem_manager_constructed (GObject *object)
{
        GClueModemManagerPrivate *priv = GCLUE_MODEM_MANAGER (object)->priv;

        G_OBJECT_CLASS (gclue_modem_manager_parent_class)->constructed (object);

        priv->cancellable = g_cancellable_new ();

        g_bus_get (G_BUS_TYPE_SYSTEM,
                   priv->cancellable,
                   on_bus_get_ready,
                   object);
}

static void
gclue_modem_manager_init (GClueModemManager *manager)
{
        manager->priv = gclue_modem_manager_get_instance_private (manager);
        manager->priv->modems_not_enabled = g_hash_table_new_full (g_str_hash,
                                                                   g_str_equal,
                                                                   g_free,
                                                                   g_object_unref);
}

static void
on_modem_destroyed (gpointer data,
                    GObject *where_the_object_was)
{
        GClueModemManager **manager = (GClueModemManager **) data;

        *manager = NULL;
}

/**
 * gclue_modem_manager_get_singleton:
 *
 * Get the #GClueModemManager singleton.
 *
 * Returns: (transfer full): a #GClueModemManager as #GClueModem.
 **/
GClueModem *
gclue_modem_manager_get_singleton (void)
{
        static GClueModemManager *manager = NULL;

        if (manager == NULL) {
                manager = g_object_new (GCLUE_TYPE_MODEM_MANAGER, NULL);
                g_object_weak_ref (G_OBJECT (manager),
                                   on_modem_destroyed,
                                   &manager);
        } else
                g_object_ref (manager);

        return GCLUE_MODEM (manager);
}

static gboolean
gclue_modem_manager_get_is_3g_available (GClueModem *modem)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (modem);

        return manager->priv->modem != NULL
                && mm_modem_get_state (manager->priv->modem) >= MM_MODEM_STATE_ENABLED
                && modem_has_caps (manager, MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI);
}

static gboolean
gclue_modem_manager_get_is_cdma_available (GClueModem *modem)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);
        GClueModemManager *manager = GCLUE_MODEM_MANAGER (modem);

        return manager->priv->modem != NULL
                && mm_modem_get_state (manager->priv->modem) >= MM_MODEM_STATE_ENABLED
                && modem_has_caps (manager, MM_MODEM_LOCATION_SOURCE_CDMA_BS);
}

static gboolean
gclue_modem_manager_get_is_gps_available (GClueModem *modem)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);

        return modem_has_caps (GCLUE_MODEM_MANAGER (modem),
                               MM_MODEM_LOCATION_SOURCE_GPS_NMEA);
}

static guint
gclue_modem_manager_get_time_threshold (GClueModem *modem)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), 0);

        return GCLUE_MODEM_MANAGER (modem)->priv->time_threshold;
}

static void
gclue_modem_manager_set_time_threshold (GClueModem *modem,
                                        guint       time_threshold)
{
        GClueModemManager *manager;

        g_return_if_fail (GCLUE_IS_MODEM_MANAGER (modem));

        manager = GCLUE_MODEM_MANAGER (modem);
        manager->priv->time_threshold = time_threshold;

        if (manager->priv->modem_location != NULL) {
                mm_modem_location_set_gps_refresh_rate
                        (manager->priv->modem_location,
                         time_threshold,
                         manager->priv->cancellable,
                         on_gps_refresh_rate_set,
                         NULL);
        }

        g_object_notify_by_pspec (G_OBJECT (manager),
                                  gParamSpecs[PROP_TIME_THRESHOLD]);
        g_debug ("%s: New time-threshold:  %u",
                 G_OBJECT_TYPE_NAME (manager),
                 time_threshold);
}

static void
gclue_modem_manager_enable_3g (GClueModem         *modem,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data)
{
        g_return_if_fail (GCLUE_IS_MODEM_MANAGER (modem));
        g_return_if_fail (gclue_modem_manager_get_is_3g_available (modem));

        enable_caps (GCLUE_MODEM_MANAGER (modem),
                     MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI,
                     cancellable,
                     callback,
                     user_data);
}

static gboolean
gclue_modem_manager_enable_3g_finish (GClueModem   *modem,
                                      GAsyncResult *result,
                                      GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);

        return enable_caps_finish (GCLUE_MODEM_MANAGER (modem),
                                   result,
                                   error);
}

static void
gclue_modem_manager_enable_cdma (GClueModem         *modem,
                                 GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data)
{
        g_return_if_fail (GCLUE_IS_MODEM_MANAGER (modem));
        g_return_if_fail (gclue_modem_manager_get_is_cdma_available (modem));

        enable_caps (GCLUE_MODEM_MANAGER (modem),
                     MM_MODEM_LOCATION_SOURCE_CDMA_BS,
                     cancellable,
                     callback,
                     user_data);
}

static gboolean
gclue_modem_manager_enable_cdma_finish (GClueModem   *modem,
                                        GAsyncResult *result,
                                        GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);

        return enable_caps_finish (GCLUE_MODEM_MANAGER (modem),
                                   result,
                                   error);
}

static void
gclue_modem_manager_enable_gps (GClueModem         *modem,
                                GCancellable       *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer            user_data)
{
        g_return_if_fail (GCLUE_IS_MODEM_MANAGER (modem));
        g_return_if_fail (gclue_modem_manager_get_is_gps_available (modem));

        enable_caps (GCLUE_MODEM_MANAGER (modem),
                     MM_MODEM_LOCATION_SOURCE_GPS_NMEA,
                     cancellable,
                     callback,
                     user_data);
}

static gboolean
gclue_modem_manager_enable_gps_finish (GClueModem   *modem,
                                       GAsyncResult *result,
                                       GError      **error)
{
        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);

        return enable_caps_finish (GCLUE_MODEM_MANAGER (modem),
                                   result,
                                   error);
}

static gboolean
gclue_modem_manager_disable_3g (GClueModem   *modem,
                                GCancellable *cancellable,
                                GError      **error)
{
        GClueModemManager *manager;

        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);
        g_return_val_if_fail (gclue_modem_manager_get_is_3g_available (modem), FALSE);
        manager = GCLUE_MODEM_MANAGER (modem);

        clear_3gpp_location (manager);
        g_debug ("Clearing 3GPP location caps from modem");
        return clear_caps (manager,
                           MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI,
                           cancellable,
                           error);
}

static gboolean
gclue_modem_manager_disable_cdma (GClueModem   *modem,
                                  GCancellable *cancellable,
                                  GError      **error)
{
        GClueModemManager *manager;

        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);
        g_return_val_if_fail (gclue_modem_manager_get_is_cdma_available (modem), FALSE);
        manager = GCLUE_MODEM_MANAGER (modem);

        clear_3gpp_location (manager);
        g_debug ("Clearing CDMA location caps from modem");
        return clear_caps (manager,
                           MM_MODEM_LOCATION_SOURCE_CDMA_BS,
                           cancellable,
                           error);
}


static gboolean
gclue_modem_manager_disable_gps (GClueModem   *modem,
                                 GCancellable *cancellable,
                                 GError      **error)
{
        GClueModemManager *manager;

        g_return_val_if_fail (GCLUE_IS_MODEM_MANAGER (modem), FALSE);
        g_return_val_if_fail (gclue_modem_manager_get_is_gps_available (modem), FALSE);
        manager = GCLUE_MODEM_MANAGER (modem);

        g_clear_object (&manager->priv->location_nmea);
        g_debug ("Clearing GPS NMEA caps from modem");
        return clear_caps (manager,
                           MM_MODEM_LOCATION_SOURCE_GPS_NMEA,
                           cancellable,
                           error);
}
