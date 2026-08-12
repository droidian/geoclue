/* vim: set et ts=8 sw=8: */
/*
 * Copyright 2024 Teemu Ikonen
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
 * Authors: Teemu Ikonen <tpikonen@mailbox.org>
 */

#ifndef GCLUE_IP_H
#define GCLUE_IP_H

#include <glib.h>
#include <gio/gio.h>
#include "gclue-web-source.h"

G_BEGIN_DECLS

GType gclue_ip_get_type (void) G_GNUC_CONST;

#define GCLUE_TYPE_IP                   (gclue_ip_get_type ())
#define GCLUE_IP(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_IP, GClueIp))
#define GCLUE_IS_IP(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_IP))
#define GCLUE_IP_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GCLUE_TYPE_IP, GClueIpClass))
#define GCLUE_IS_IP_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GCLUE_TYPE_IP))
#define GCLUE_IP_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GCLUE_TYPE_IP, GClueIpClass))

/**
 * GClueIp:
 *
 * All the fields in the #GClueIp structure are private and should never be accessed directly.
**/
typedef struct _GClueIp        GClueIp;
typedef struct _GClueIpClass   GClueIpClass;
typedef struct _GClueIpPrivate GClueIpPrivate;

struct _GClueIp {
        /* <private> */
        GClueWebSource parent_instance;
        GClueIpPrivate *priv;
};

/**
 * GClueIpClass:
 *
 * All the fields in the #GClueIpClass structure are private and should never be accessed directly.
**/
struct _GClueIpClass {
        /* <private> */
        GClueWebSourceClass parent_class;
};

GClueIp *        gclue_ip_get_singleton      (void);

G_END_DECLS

#endif /* GCLUE_IP_H */
