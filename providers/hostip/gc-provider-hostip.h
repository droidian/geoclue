/*
 * Geoclue
 * gc-provider-hostip.h - An Address/Position provider for hostip.info
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 */

#ifndef _GC_PROVIDER_HOSTIP
#define _GC_PROVIDER_HOSTIP

#include <glib-object.h>

G_BEGIN_DECLS


#define GC_TYPE_PROVIDER_HOSTIP (gc_provider_hostip_get_type ())

#define GC_PROVIDER_HOSTIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_PROVIDER_HOSTIP, GcProviderHostip))
#define GC_PROVIDER_HOSTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_PROVIDER_HOSTIP, GcProviderHostipClass))
#define GC_IS_PROVIDER_HOSTIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_PROVIDER_HOSTIP))
#define GC_IS_PROVIDER_HOSTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_PROVIDER_HOSTIP))

typedef struct _GcProviderHostip {
	GcWebProvider parent;
	GMainLoop *loop;
} GcProviderHostip;

typedef struct _GcProviderHostipClass {
	GcWebProviderClass parent_class;
} GcProviderHostipClass;

GType gc_provider_hostip_get_type (void);

G_END_DECLS

#endif
