/*
 * Geoclue
 * geoclue-hostip.h - An Address/Position provider for hostip.info
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
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
