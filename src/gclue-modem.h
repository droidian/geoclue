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

#ifndef GCLUE_MODEM_H
#define GCLUE_MODEM_H

#include <gio/gio.h>

G_BEGIN_DECLS

GType gclue_modem_get_type (void) G_GNUC_CONST;

#define GCLUE_TYPE_MODEM            (gclue_modem_get_type ())
#define GCLUE_MODEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_MODEM, GClueModem))
#define GCLUE_IS_MODEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_MODEM))
#define GCLUE_MODEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GCLUE_TYPE_MODEM, GClueModemClass))
#define GCLUE_IS_MODEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GCLUE_TYPE_MODEM))
#define GCLUE_MODEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GCLUE_TYPE_MODEM, GClueModemClass))

/**
 * GClueModem:
 *
 * All the fields in the #GClueModem structure are private and should never be accessed directly.
**/
typedef struct _GClueModem        GClueModem;
typedef struct _GClueModemClass   GClueModemClass;
typedef struct _GClueModemPrivate GClueModemPrivate;

struct _GClueModem {
        /* <private> */
        GObject parent_instance;
        GClueModemPrivate *priv;
};

/**
 * GClueModemClass:
 *
 * All the fields in the #GClueModemClass structure are private and should never be accessed directly.
**/
struct _GClueModemClass {
        /* <private> */
        GObjectClass parent_class;
};

GClueModem * gclue_modem_get_singleton     (void);
gboolean     gclue_modem_get_is_3g_available  (GClueModem         *modem);
gboolean     gclue_modem_get_is_gps_available (GClueModem         *modem);
void         gclue_modem_enable_3g         (GClueModem         *modem,
                                            GCancellable       *cancellable,
                                            GAsyncReadyCallback callback,
                                            gpointer            user_data);
gboolean     gclue_modem_enable_3g_finish  (GClueModem         *modem,
                                            GAsyncResult       *result,
                                            GError            **error);
void         gclue_modem_enable_gps        (GClueModem         *modem,
                                            GCancellable       *cancellable,
                                            GAsyncReadyCallback callback,
                                            gpointer            user_data);
gboolean     gclue_modem_enable_gps_finish (GClueModem         *modem,
                                            GAsyncResult       *result,
                                            GError            **error);
gboolean     gclue_modem_disable_3g        (GClueModem         *modem,
                                            GCancellable       *cancellable,
                                            GError            **error);
gboolean     gclue_modem_disable_gps       (GClueModem         *modem,
                                            GCancellable       *cancellable,
                                            GError            **error);

G_END_DECLS

#endif /* GCLUE_MODEM_H */
