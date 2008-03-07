/*
 * Geoclue
 * gc-iface-geoclue.h - GInterface for org.freedesktop.Geoclue
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
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
	gboolean (*shutdown) (GcIfaceGeoclue *geoclue,
			      GError        **error);
};

GType gc_iface_geoclue_get_type (void);

void gc_iface_geoclue_emit_status_changed (GcIfaceGeoclue *gc,
					   GeoclueStatus   status);

G_END_DECLS

#endif
