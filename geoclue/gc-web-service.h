/* GeoclueWebService is meant to be used in webservice-based geoclue
 * provider implementations. Its objective is to simplify/abstract 
 * interaction with web services.
 * 
 *** CONSTRUCTION
 * 
 * 	GcWebService web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
 * 	gc_web_service_set_base_url (web_service, "http://example.org");
 * 
 *** QUERYING DATA
 *  
 * gc_web_service_query takes a NULL-terminated list of key-value
 * pairs as parameters. The pairs will be used as GET parameters. 
 * value-part will be escaped.
 * 
 * 	// wanted query url: "http://example.org?key1=value1&key2=value2"
 * 	if (!gc_web_service_query (web_service,
 * 	                           "key1", "value1",
 * 	                           "key2", "value2",
 * 	                           NULL)) {
 * 		// query error
 * 	}
 * 
 *** READING DATA
 * 
 * There are so far functions for reading char arrays and doubles from XML 
 * elements using xpath expressions (adding new functions is trivial).
 * 
 * 	gchar *str;
 * 	gdouble number;
 * 	if (gc_web_service_get_string (web_service,
 * 	                               &str, "//path/to/element")) {
 * 		g_debug("string: %s", str);
 * 	}
 * 	if (gc_web_service_get_double (web_service,
 * 	                               &number, "//path/to/another/element")) {
 * 		g_debug("double: %f", number);
 * 	}
 *
 * gc_web_service_get_response will give a copy of the full response.
 * 
 * If the xml data uses namespaces, they should be added with 
 * gc_web_service_add_namespace before calling "gc_web_service_get_*"
 * -functions.
 */


#ifndef GC_WEB_SERVICE_H
#define GC_WEB_SERVICE_H

#include <glib-object.h>
#include <libxml/xpath.h> /* TODO: should move prvates to .c-file and get rid of this*/

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

gboolean gc_web_service_set_base_url (GcWebService *self, gchar *url);
gboolean gc_web_service_add_namespace (GcWebService *self, gchar *namespace, gchar *uri);

gboolean gc_web_service_query (GcWebService *self, ...);
gboolean gc_web_service_get_string (GcWebService *self, gchar **OUT_value, gchar *xpath);
gboolean gc_web_service_get_double (GcWebService *self, gdouble *OUT_value, gchar *xpath);

gboolean gc_web_service_get_response (GcWebService *self, guchar **response, gint *response_length);

G_END_DECLS

#endif /* GC_WEB_SERVICE_H */
