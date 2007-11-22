#ifndef GC_WEB_PROVIDER_H
#define GC_WEB_PROVIDER_H

#include <glib-object.h>
#include <libxml/xpath.h> /* TODO: should move prvates to .c-file and get rid of this*/

G_BEGIN_DECLS

#define GC_TYPE_WEB_PROVIDER (gc_web_provider_get_type ())

#define GC_WEB_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_WEB_PROVIDER, GcWebProvider))
#define GC_WEB_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_WEB_PROVIDER, GcWebProviderClass))
#define GC_IS_WEB_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_WEB_PROVIDER))
#define GC_IS_WEB_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_WEB_PROVIDER))
#define GC_WEB_PROVIDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GC_TYPE_WEB_PROVIDER, GcWebProviderClass))


typedef struct _GcWebProvider {
	GcProvider parent;
	
	/* private */
	gchar *base_url;
	guchar *response;
	gint response_length;
	GList *namespaces;
	xmlXPathContext *xpath_ctx;
} GcWebProvider;

typedef struct _GcWebProviderClass {
	GcProviderClass parent_class;
} GcWebProviderClass;

GType gc_web_provider_get_type (void);

gboolean gc_web_provider_set_base_url (GcWebProvider *self, gchar *url);
gboolean gc_web_provider_add_namespace (GcWebProvider *self, gchar *namespace, gchar *uri);

gboolean gc_web_provider_query (GcWebProvider *self, ...);
gboolean gc_web_provider_get_string (GcWebProvider *self, gchar **OUT_value, gchar *xpath);
gboolean gc_web_provider_get_double (GcWebProvider *self, gdouble *OUT_value, gchar *xpath);

gboolean gc_web_provider_get_response (GcWebProvider *self, guchar **response, gint *response_length);

G_END_DECLS

#endif /* GC_WEB_PROVIDER_H */
