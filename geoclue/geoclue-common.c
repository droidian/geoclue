/*
 * Geoclue
 * geoclue-common.c - Client API for accessing GcIfaceGeoclue
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <geoclue/geoclue-common.h>
#include <geoclue/geoclue-marshal.h>

#include "gc-iface-geoclue-bindings.h"

typedef struct _GeoclueCommonPrivate {
	int dummy;
} GeoclueCommonPrivate;

enum {
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_COMMON, GeoclueCommonPrivate))

G_DEFINE_TYPE (GeoclueCommon, geoclue_common, GEOCLUE_TYPE_PROVIDER);

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (geoclue_common_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	G_OBJECT_CLASS (geoclue_common_parent_class)->dispose (object);
}

static void
status_changed (DBusGProxy    *proxy,
		gboolean       active,
		GeoclueCommon *common)
{
	g_signal_emit (common, signals[STATUS_CHANGED], 0, active);
}

static GObject *
constructor (GType                  type,
	     guint                  n_props,
	     GObjectConstructParam *props)
{
	GObject *object;
	GeoclueProvider *provider;

	object = G_OBJECT_CLASS (geoclue_common_parent_class)->constructor
		(type, n_props, props);
	provider = GEOCLUE_PROVIDER (object);

	dbus_g_proxy_add_signal (provider->proxy, "StatusChanged",
				 G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (provider->proxy, "StatusChanged",
				     G_CALLBACK (status_changed),
				     object, NULL);
	
	return object;
}

static void
geoclue_common_class_init (GeoclueCommonClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;

	g_type_class_add_private (klass, sizeof (GeoclueCommonPrivate));

	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						G_STRUCT_OFFSET (GeoclueCommonClass, status_changed), 
						NULL, NULL,
						g_cclosure_marshal_VOID__BOOLEAN,
						G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
geoclue_common_init (GeoclueCommon *common)
{
}

/**
 * geoclue_common_new:
 * @service:
 * @path:
 *
 *
 * Return value:
 */
GeoclueCommon *
geoclue_common_new (const char *service,
		    const char *path)
{
	return g_object_new (GEOCLUE_TYPE_COMMON,
			     "service", service,
			     "path", path,
			     "interface", GEOCLUE_COMMON_INTERFACE_NAME,
			     NULL);
}

gboolean
geoclue_common_get_provider_info (GeoclueCommon *common,
				  char         **name,
				  char         **description,
				  GError       **error)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (common);

	if (!org_freedesktop_Geoclue_get_provider_info (provider->proxy,
							name, description,
							error)) {
		return FALSE;
	}

	return TRUE;
}

gboolean
geoclue_common_get_status (GeoclueCommon *common,
			   gboolean      *active,
			   GError       **error)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (common);

	if (!org_freedesktop_Geoclue_get_status (provider->proxy, 
						 active, error)) {
		return FALSE;
	}

	return TRUE;
}

gboolean
geoclue_common_shutdown (GeoclueCommon *common,
			 GError       **error)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (common);

	if (!org_freedesktop_Geoclue_shutdown (provider->proxy, error)) {
		return FALSE;
	}

	return TRUE;
}
