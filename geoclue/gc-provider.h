/*
 * Geoclue
 * gc-provider.h - A provider object that handles the basic D-Bus required for
 *                 a provider.
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GC_PROVIDER
#define _GC_PROVIDER

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <geoclue/gc-iface-geoclue.h>

G_BEGIN_DECLS

#define GC_TYPE_PROVIDER (gc_provider_get_type ())

#define GC_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_PROVIDER, GcProvider))
#define GC_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_PROVIDER, GcProviderClass))
#define GC_IS_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_PROVIDER))
#define GC_IS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_PROVIDER))
#define GC_PROVIDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GC_TYPE_PROVIDER, GcProviderClass))

typedef struct _GcProvider {
	GObject parent_class;
	
	DBusGConnection *connection;
} GcProvider;

typedef struct _GcProviderClass {
	GObjectClass parent_class;

	/* Implements the GcIfaceGeoclue interface */
	gboolean (*get_status) (GcIfaceGeoclue *geoclue,
				GeoclueStatus  *status,
				GError        **error);
        gboolean (*set_options) (GcIfaceGeoclue *geoclue,
                                 GHashTable     *options,
                                 GError        **error);
	gboolean (*shutdown) (GcIfaceGeoclue *geoclue,
			      GError        **error);
} GcProviderClass;

GType gc_provider_get_type (void);

void gc_provider_set_details (GcProvider *provider,
			      const char *service,
			      const char *path,
			      const char *name,
			      const char *description);

G_END_DECLS

#endif
