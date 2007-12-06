/*
 * Geoclue
 * geoclue-common.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GEOCLUE_COMMON_H
#define _GEOCLUE_COMMON_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_COMMON (geoclue_common_get_type ())
#define GEOCLUE_COMMON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_COMMON, GeoclueCommon))
#define GEOCLUE_IS_COMMON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_COMMON))

#define GEOCLUE_COMMON_INTERFACE_NAME "org.freedesktop.Geoclue"

typedef struct _GeoclueCommon {
	GeoclueProvider provider;
} GeoclueCommon;

typedef struct _GeoclueCommonClass {
	GeoclueProviderClass provider_class;

	void (* status_changed) (GeoclueCommon *common,
				 gboolean       active);
} GeoclueCommonClass;

GType geoclue_common_get_type (void);

GeoclueCommon *geoclue_common_new (const char *service,
				       const char *path);

gboolean geoclue_common_get_provider_info (GeoclueCommon *common,
					   char         **name,
					   char         **description,
					   GError       **error);
gboolean geoclue_common_get_status (GeoclueCommon *common,
				    gboolean      *active,
				    GError       **error);
gboolean geoclue_common_shutdown (GeoclueCommon *common,
				  GError       **error);

G_END_DECLS

#endif
