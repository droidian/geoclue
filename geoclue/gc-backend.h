/*
 * Geoclue
 * gc-backend.h - A backend object that handles the basic D-Bus required for
 *                a backend.
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GC_BACKEND
#define _GC_BACKEND

G_BEGIN_DECLS

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#define GC_TYPE_BACKEND (gc_backend_get_type ())

#define GC_BACKEND(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_BACKEND, GcBackend))
#define GC_BACKEND_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_BACKEND, GcBackendClass))
#define GC_IS_BACKEND(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_BACKEND))
#define GC_IS_BACKEND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_BACKEND))

typedef struct _GcBackend {
	GObject parent_class;
	
	DBusGConnection *connection;
} GcBackend;

typedef struct _GcBackendClass {
	GObjectClass parent_class;
} GcBackendClass;

GType gc_backend_get_type (void);

void gc_backend_set_details (GcBackend  *backend,
			     const char *service,
			     const char *path);

G_END_DECLS

#endif
