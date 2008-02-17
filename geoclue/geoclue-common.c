/*
 * Geoclue
 * geoclue-common.c - Client API for accessing GcIfaceGeoclue
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-common
 * @short_description: Geoclue common client API
 *
 * #GeoclueCommon contains the methods and signals common to all Geoclue 
 * providers. It is part of the public client API which uses the D-Bus 
 * API to communicate with the actual provider.
 * 
 * After a #GeoclueCommon is created with geoclue_common_new(), it can 
 * be used to obtain information about the provider and to shut the 
 * provider down.
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
	GeoclueProvider *provider = GEOCLUE_PROVIDER (object);
	GError *error = NULL;

	if (!org_freedesktop_Geoclue_shutdown (provider->proxy, &error)) {
		g_warning ("Error shutting down: %s", error->message);
		g_error_free (error);
	}

	G_OBJECT_CLASS (geoclue_common_parent_class)->dispose (object);
}

static void
status_changed (DBusGProxy    *proxy,
		GeoclueStatus status,
		GeoclueCommon *common)
{
	g_signal_emit (common, signals[STATUS_CHANGED], 0, status);
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
				 G_TYPE_INT, G_TYPE_INVALID);
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
						g_cclosure_marshal_VOID__INT,
						G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
geoclue_common_init (GeoclueCommon *common)
{
}

/**
 * geoclue_common_new:
 * @service: D-Bus service name
 * @path: D-Bus path name
 *
 * Creates a new #GeoclueCommon with given D-Bus service name and path.
 * 
 * Return value: Pointer to a new #GeoclueCommon
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

/**
 * geoclue_common_get_provider_info:
 * @common: A #GeoclueCommon object
 * @name: Pointer for returned provider name
 * @description: Pointer for returned provider description
 * @error:  Pointer for returned #GError
 * 
 * Obtains name and a short description of the provider.
 * 
 * Return value: #TRUE if D-Bus call succeeded
 */
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

/**
 * geoclue_common_get_status:
 * @common: A #GeoclueCommon object
 * @status: Pointer for returned status as GeoclueStatus
 * @error:  Pointer for returned #GError
 * 
 * Obtains the current status of the provider.
 * 
 * Return value: #TRUE if D-Bus call succeeded
 */
gboolean
geoclue_common_get_status (GeoclueCommon *common,
			   GeoclueStatus *status,
			   GError       **error)
{
	gint i = 0;
	GeoclueProvider *provider = GEOCLUE_PROVIDER (common);

	if (!org_freedesktop_Geoclue_get_status (provider->proxy, 
						 &i, error)) {
		return FALSE;
	}

	if (status != NULL) {
		*status = i;
	}

	return TRUE;
}

/**
 * geoclue_common_set_options:
 * @common: A #GeoclueCommon object
 * @options: A GHashTable containing the options
 * @error: Pointer for returned #GError
 *
 * Sets the options on the provider.
 *
 * Return value: #TRUE if D-Bus call succeeded
 */
gboolean
geoclue_common_set_options (GeoclueCommon *common,
                            GHashTable    *options,
                            GError       **error)
{
        GeoclueProvider *provider = GEOCLUE_PROVIDER (common);
        
        if (options == NULL) {
                return TRUE;
        }
        
        if (!org_freedesktop_Geoclue_set_options (provider->proxy, options,
                                                  error)) {
                return FALSE;
        }

        return TRUE;
}
