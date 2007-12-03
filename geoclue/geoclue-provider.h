/* 
 * Geoclue
 * geoclue-provider.h -
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GEOCLUE_PROVIDER_H
#define _GEOCLUE_PROVIDER_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_PROVIDER (geoclue_provider_get_type ())
#define GEOCLUE_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_PROVIDER, GeoclueProvider))
#define GEOCLUE_IS_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_PROVIDER))

typedef struct _GeoclueProvider {
	GObject object;

	DBusGProxy *proxy;
} GeoclueProvider;

typedef struct _GeoclueProviderClass {
	GObjectClass object_class;
} GeoclueProviderClass;

GType geoclue_provider_get_type (void);

G_END_DECLS

#endif
