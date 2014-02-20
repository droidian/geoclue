/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2013 Satabdi Das
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
 * Authors: Satabdi Das <satabdidas@gmail.com>
 *          Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#ifndef GCLUE_IPCLIENT_H
#define GCLUE_IPCLIENT_H

#include <glib.h>
#include <gio/gio.h>
#include "gclue-web-source.h"

G_BEGIN_DECLS

#define GCLUE_IPCLIENT_ACCURACY_LEVEL GCLUE_ACCURACY_LEVEL_CITY

GType gclue_ipclient_get_type (void) G_GNUC_CONST;

#define GCLUE_TYPE_IPCLIENT                  (gclue_ipclient_get_type ())
#define GCLUE_IPCLIENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_IPCLIENT, GClueIpclient))
#define GCLUE_IS_IPCLIENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_IPCLIENT))
#define GCLUE_IPCLIENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GCLUE_TYPE_IPCLIENT, GClueIpclientClass))
#define GCLUE_IS_IPCLIENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GCLUE_TYPE_IPCLIENT))
#define GCLUE_IPCLIENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GCLUE_TYPE_IPCLIENT, GClueIpclientClass))

/**
 * GClueIpclient:
 *
 * All the fields in the #GClueIpclient structure are private and should never be accessed directly.
**/
typedef struct _GClueIpclient        GClueIpclient;
typedef struct _GClueIpclientClass   GClueIpclientClass;
typedef struct _GClueIpclientPrivate GClueIpclientPrivate;

struct _GClueIpclient {
        /* <private> */
        GClueWebSource parent_instance;
        GClueIpclientPrivate *priv;
};

/**
 * GClueIpclientClass:
 *
 * All the fields in the #GClueIpclientClass structure are private and should never be accessed directly.
**/
struct _GClueIpclientClass {
        /* <private> */
        GClueWebSourceClass parent_class;
};

GClueIpclient *gclue_ipclient_get_singleton (void);

G_END_DECLS

#endif /* GCLUE_IPCLIENT_H */
