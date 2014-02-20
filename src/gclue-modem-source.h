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

#ifndef GCLUE_MODEM_SOURCE_H
#define GCLUE_MODEM_SOURCE_H

#include <glib.h>
#include <libmm-glib.h>
#include "gclue-location-source.h"

G_BEGIN_DECLS

GType gclue_modem_source_get_type (void) G_GNUC_CONST;

#define GCLUE_TYPE_MODEM_SOURCE                  (gclue_modem_source_get_type ())
#define GCLUE_MODEM_SOURCE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_MODEM_SOURCE, GClueModemSource))
#define GCLUE_IS_MODEM_SOURCE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_MODEM_SOURCE))
#define GCLUE_MODEM_SOURCE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GCLUE_TYPE_MODEM_SOURCE, GClueModemSourceClass))
#define GCLUE_IS_MODEM_SOURCE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GCLUE_TYPE_MODEM_SOURCE))
#define GCLUE_MODEM_SOURCE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GCLUE_TYPE_MODEM_SOURCE, GClueModemSourceClass))

/**
 * GClueModemSource:
 *
 * All the fields in the #GClueModemSource structure are private and should never be accessed directly.
**/
typedef struct _GClueModemSource        GClueModemSource;
typedef struct _GClueModemSourceClass   GClueModemSourceClass;
typedef struct _GClueModemSourcePrivate GClueModemSourcePrivate;

struct _GClueModemSource {
        /* <private> */
        GClueLocationSource parent_instance;
        GClueModemSourcePrivate *priv;
};

/**
 * GClueModemSourceClass:
 *
 * All the fields in the #GClueModemSourceClass structure are private and should never be accessed directly.
**/
struct _GClueModemSourceClass {
        /* <private> */
        GClueLocationSourceClass parent_class;

        MMModemLocationSource
        (*get_req_modem_location_caps) (GClueModemSource *source,
                                        const char      **caps_name);

        void (*modem_location_changed) (GClueModemSource *source,
                                        MMModemLocation  *modem_location);
};

G_END_DECLS

#endif /* GCLUE_MODEM_SOURCE_H */
