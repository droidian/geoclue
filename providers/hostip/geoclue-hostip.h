/*
 * Geoclue
 * geoclue-hostip.h - An Address/Position provider for hostip.info
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _GEOCLUE_HOSTIP
#define _GEOCLUE_HOSTIP

#include <glib-object.h>
#include <geoclue/gc-web-service.h>
#include <geoclue/gc-provider.h>

G_BEGIN_DECLS


#define GEOCLUE_TYPE_HOSTIP (geoclue_hostip_get_type ())

#define GEOCLUE_HOSTIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_HOSTIP, GeoclueHostip))
#define GEOCLUE_HOSTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_TYPE_HOSTIP, GeoclueHostipClass))
#define GEOCLUE_IS_HOSTIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_HOSTIP))
#define GEOCLUE_IS_HOSTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_TYPE_HOSTIP))

typedef struct _GeoclueHostip {
	GcProvider parent;
	GMainLoop *loop;
	GcWebService *web_service;
} GeoclueHostip;

typedef struct _GeoclueHostipClass {
	GcProviderClass parent_class;
} GeoclueHostipClass;

GType geoclue_hostip_get_type (void);

G_END_DECLS

#endif
