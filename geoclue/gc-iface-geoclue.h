/*
 * Geoclue
 * gc-iface-geoclue.h - GInterface for org.freedesktop.Geoclue
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
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

#ifndef _GC_IFACE_GEOCLUE_H
#define _GC_IFACE_GEOCLUE_H

#include <geoclue/geoclue-types.h>

G_BEGIN_DECLS

#define GC_TYPE_IFACE_GEOCLUE (gc_iface_geoclue_get_type ())
#define GC_IFACE_GEOCLUE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_IFACE_GEOCLUE, GcIfaceGeoclue))
#define GC_IFACE_GEOCLUE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_IFACE_GEOCLUE, GcIfaceGeoclueClass))
#define GC_IS_IFACE_GEOCLUE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_IFACE_GEOCLUE))
#define GC_IS_IFACE_GEOCLUE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_IFACE_GEOCLUE))
#define GC_IFACE_GEOCLUE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GC_TYPE_IFACE_GEOCLUE, GcIfaceGeoclueClass))

typedef struct _GcIfaceGeoclue GcIfaceGeoclue; /* Dummy typedef */
typedef struct _GcIfaceGeoclueClass GcIfaceGeoclueClass;

struct _GcIfaceGeoclueClass {
	GTypeInterface base_iface;

	/* signals */
	void (* status_changed) (GcIfaceGeoclue *geoclue,
				 GeoclueStatus   status);

	/* vtable */
	gboolean (*get_provider_info) (GcIfaceGeoclue  *gc,
				       gchar          **name,
				       gchar          **description,
				       GError         **error);
	gboolean (*get_status) (GcIfaceGeoclue *geoclue,
				GeoclueStatus  *status,
				GError        **error);
        gboolean (*set_options) (GcIfaceGeoclue *geoclue,
                                 GHashTable     *options,
                                 GError        **error);
        void (*add_reference) (GcIfaceGeoclue        *geoclue,
                               DBusGMethodInvocation *context);
        void (*remove_reference) (GcIfaceGeoclue        *geoclue,
                                  DBusGMethodInvocation *context);
};

GType gc_iface_geoclue_get_type (void);

void gc_iface_geoclue_emit_status_changed (GcIfaceGeoclue *gc,
					   GeoclueStatus   status);

G_END_DECLS

#endif
