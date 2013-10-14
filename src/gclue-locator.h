/* vim: set et ts=8 sw=8: */
/* gclue-locator.h
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

#ifndef GCLUE_LOCATOR_H
#define GCLUE_LOCATOR_H

#include <gio/gio.h>
#include "geocode-location.h"

G_BEGIN_DECLS

#define GCLUE_TYPE_LOCATOR            (gclue_locator_get_type())
#define GCLUE_LOCATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATOR, GClueLocator))
#define GCLUE_LOCATOR_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATOR, GClueLocator const))
#define GCLUE_LOCATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_LOCATOR, GClueLocatorClass))
#define GCLUE_IS_LOCATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_LOCATOR))
#define GCLUE_IS_LOCATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_LOCATOR))
#define GCLUE_LOCATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_LOCATOR, GClueLocatorClass))

typedef struct _GClueLocator        GClueLocator;
typedef struct _GClueLocatorClass   GClueLocatorClass;
typedef struct _GClueLocatorPrivate GClueLocatorPrivate;

struct _GClueLocator
{
        GObject parent;

        /*< private >*/
        GClueLocatorPrivate *priv;
};

struct _GClueLocatorClass
{
        GObjectClass parent_class;
};

GType gclue_locator_get_type (void) G_GNUC_CONST;

GClueLocator *      gclue_locator_new           (void);
void                gclue_locator_start         (GClueLocator       *locator,
                                                 GCancellable       *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer            user_data);
gboolean            gclue_locator_start_finish  (GClueLocator *locator,
                                                 GAsyncResult *res,
                                                 GError      **error);
void                gclue_locator_stop          (GClueLocator       *locator,
                                                 GCancellable       *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer            user_data);
gboolean            gclue_locator_stop_finish   (GClueLocator *locator,
                                                 GAsyncResult *res,
                                                 GError      **error);
GeocodeLocation *   gclue_locator_get_location  (GClueLocator *locator);
guint               gclue_locator_get_threshold (GClueLocator *locator);
void                gclue_locator_set_threshold (GClueLocator *locator,
                                                 guint         threshold);

G_END_DECLS

#endif /* GCLUE_LOCATOR_H */
