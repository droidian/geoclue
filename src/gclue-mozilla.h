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

#ifndef GCLUE_MOZILLA_H
#define GCLUE_MOZILLA_H

#include <glib.h>
#include <libsoup/soup.h>
#include "wpa_supplicant-interface.h"
#include "gclue-location.h"
#include "gclue-3g-tower.h"

G_BEGIN_DECLS

#define GCLUE_TYPE_MOZILLA            (gclue_mozilla_get_type())
#define GCLUE_MOZILLA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_MOZILLA, GClueMozilla))
#define GCLUE_MOZILLA_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_MOZILLA, GClueMozilla const))
#define GCLUE_MOZILLA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_MOZILLA, GClueMozillaClass))
#define GCLUE_IS_MOZILLA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_MOZILLA))
#define GCLUE_IS_MOZILLA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_MOZILLA))
#define GCLUE_MOZILLA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_MOZILLA, GClueMozillaClass))

typedef struct _GClueMozilla        GClueMozilla;
typedef struct _GClueMozillaClass   GClueMozillaClass;
typedef struct _GClueMozillaPrivate GClueMozillaPrivate;

struct _GClueMozilla
{
        GObject parent;

        /*< private >*/
        GClueMozillaPrivate *priv;
};

struct _GClueMozillaClass
{
        GObjectClass parent_class;
};

struct GClueWifi;
typedef struct _GClueWifi GClueWifi;

GType gclue_mozilla_get_type (void) G_GNUC_CONST;

GClueMozilla *gclue_mozilla_get_singleton (void);

void gclue_mozilla_set_wifi (GClueMozilla *mozilla,
                             GClueWifi *wifi);
gboolean
gclue_mozilla_test_set_wifi (GClueMozilla *mozilla,
                             GClueWifi *old, GClueWifi *new);
void gclue_mozilla_set_bss_dirty (GClueMozilla *mozilla);

void gclue_mozilla_set_tower (GClueMozilla *mozilla,
                              const GClue3GTower *tower);
gboolean
gclue_mozilla_has_tower (GClueMozilla *mozilla);
GClue3GTower *
gclue_mozilla_get_tower (GClueMozilla *mozilla);

SoupMessage *
gclue_mozilla_create_query (GClueMozilla  *mozilla,
                            const char *url,
                            gboolean skip_tower,
                            gboolean skip_bss,
                            const char **query_data_description,
                            GError      **error);
GClueLocation *
gclue_mozilla_parse_response (const char *json,
                              const char *location_description,
                              GError    **error);
SoupMessage *
gclue_mozilla_create_submit_query (GClueMozilla  *mozilla,
                                   const char    *url,
                                   GClueLocation *location,
                                   GError       **error);
gboolean
gclue_mozilla_parse_submit_response (const char  *response_contents,
                                     gint         status_code,
                                     GError     **error);
gboolean
gclue_mozilla_should_ignore_bss (WPABSS *bss);

G_END_DECLS

#endif /* GCLUE_MOZILLA_H */
