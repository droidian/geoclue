/*
 * Geoclue
 * gc-provider.h - A provider object that handles the basic D-Bus required for
 *                 a provider.
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GC_PROVIDER
#define _GC_PROVIDER

G_BEGIN_DECLS

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#define GC_TYPE_PROVIDER (gc_provider_get_type ())

#define GC_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_PROVIDER, GcProvider))
#define GC_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_PROVIDER, GcProviderClass))
#define GC_IS_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_PROVIDER))
#define GC_IS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_PROVIDER))

typedef struct _GcProvider {
	GObject parent_class;
	
	DBusGConnection *connection;
} GcProvider;

typedef struct _GcProviderClass {
	GObjectClass parent_class;
} GcProviderClass;

GType gc_provider_get_type (void);

void gc_provider_set_details (GcProvider *provider,
			      const char *service,
			      const char *path);

G_END_DECLS

#endif
