/*
 * Geoclue
 * gc-web-service.h
 *
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */
#ifndef GC_WEB_SERVICE_H
#define GC_WEB_SERVICE_H

#include <glib-object.h>
#include <libxml/xpath.h> /* TODO: could move privates to .c-file and get rid of this*/

G_BEGIN_DECLS

#define GC_TYPE_WEB_SERVICE (gc_web_service_get_type ())

#define GC_WEB_SERVICE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_WEB_SERVICE, GcWebService))
#define GC_WEB_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GC_TYPE_WEB_SERVICE, GcWebServiceClass))
#define GC_IS_WEB_SERVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_WEB_SERVICE))
#define GC_IS_WEB_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GC_TYPE_WEB_SERVICE))
#define GC_WEB_SERVICE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GC_TYPE_WEB_SERVICE, GcWebServiceClass))


typedef struct _GcWebService {
	GObject parent;
	
	/* private */
	gchar *base_url;
	guchar *response;
	gint response_length;
	GList *namespaces;
	xmlXPathContext *xpath_ctx;
} GcWebService;

typedef struct _GcWebServiceClass {
	GObjectClass parent_class;
} GcWebServiceClass;

GType gc_web_service_get_type (void);

void gc_web_service_set_base_url (GcWebService *self, gchar *url);
gboolean gc_web_service_add_namespace (GcWebService *self, gchar *namespace, gchar *uri);

gboolean gc_web_service_query (GcWebService *self, GError **error, ...);
gboolean gc_web_service_get_string (GcWebService *self, gchar **value, gchar *xpath);
gboolean gc_web_service_get_double (GcWebService *self, gdouble *value, gchar *xpath);

gboolean gc_web_service_get_response (GcWebService *self, guchar **response, gint *response_length);

G_END_DECLS

#endif /* GC_WEB_SERVICE_H */
