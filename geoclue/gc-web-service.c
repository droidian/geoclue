#include <stdarg.h>
#include <glib-object.h>

#include <libxml/nanohttp.h>
#include <libxml/xpathInternals.h>
#include <libxml/uri.h>      /* for xmlURIEscapeStr */

#include "gc-web-service.h"

G_DEFINE_TYPE (GcWebService, gc_web_service, G_TYPE_OBJECT)

typedef struct _XmlNamespace {
	gchar *name;
	gchar *uri;
}XmlNamespace;

/* GFunc, use with g_list_foreach */
static void
gc_web_service_register_ns (gpointer data, gpointer user_data)
{
	GcWebService *self = (GcWebService *)user_data;
	XmlNamespace *ns = (XmlNamespace *)data;
	
	xmlXPathRegisterNs (self->xpath_ctx, 
	                    (xmlChar*)ns->name, (xmlChar*)ns->uri);
}

/* GFunc, use with g_list_foreach */
static void
gc_web_service_free_ns (gpointer data, gpointer user_data)
{
	XmlNamespace *ns = (XmlNamespace *)data;
	
	g_free (ns->name);
	g_free (ns->uri);
	g_free (ns);
}


/* Register namespaces listed in self->namespaces */
static void
gc_web_service_register_namespaces (GcWebService *self)
{
	g_assert (self->xpath_ctx);
	g_list_foreach (self->namespaces, (GFunc)gc_web_service_register_ns, self);
}

/* Parse data (self->response), build xpath context and register 
 * namespaces. Nothing will be done if xpath context exists already. */
static gboolean
gc_web_service_build_xpath_context (GcWebService *self)
{
	xmlDocPtr doc;
	xmlChar *tmp;
	
	g_assert (self->response);
	
	/* don't rebuild if there's no need */
	if (self->xpath_ctx) {
		return TRUE;
	}
	
	/* make sure response is NULL-terminated */
	tmp = xmlStrndup(self->response, self->response_length);
	doc = xmlParseDoc (tmp);
	if (!doc) {
		/* TODO: error handling */
		g_free (tmp);
		return FALSE;
	}
	g_free (tmp);
	
	self->xpath_ctx = xmlXPathNewContext(doc);
	if (!self->xpath_ctx) {
		/* TODO: error handling */
		return FALSE;
	}
	gc_web_service_register_namespaces (self);
	return TRUE;
}

/* fetch data from url, save into self->response */
static gboolean
gc_web_service_fetch (GcWebService *self, gchar *url)
{
	void* ctxt = NULL;
	gint len;
	xmlChar buf[1024];
	xmlBuffer *output;
	
	g_free (self->response);
	self->response = NULL;
	xmlXPathFreeContext (self->xpath_ctx);
	self->xpath_ctx = NULL;
	g_assert (url);
	
	xmlNanoHTTPInit();
	ctxt = xmlNanoHTTPMethod (url, "GET", NULL, NULL, NULL, 0);
	if (!ctxt) {
		g_printerr ("xmlNanoHTTPMethod did not get a response\n");
		return FALSE;
	}
	
	output = xmlBufferCreate ();
	while ((len = xmlNanoHTTPRead (ctxt, buf, sizeof(buf))) > 0) {
		if (xmlBufferAdd (output, buf, len) != 0) {
			g_printerr ("xmlBufferAdd failed\n");
			break;
		}
	}
	xmlNanoHTTPClose(ctxt);
	
	self->response_length = xmlBufferLength (output);
	self->response = g_memdup (xmlBufferContent (output), self->response_length);
	xmlBufferFree (output);
	
	return (self->response_length > 0);
}

static void
gc_web_service_init (GcWebService *self)
{
	self->response = NULL;
	self->response_length = 0;
	self->xpath_ctx = NULL;
	self->namespaces = NULL;
	self->base_url = NULL;
}


static void
gc_web_service_finalize (GObject *obj)
{
	GcWebService *self = (GcWebService *) obj;
	
	g_free (self->base_url);
	g_free (self->response);
	
	g_list_foreach (self->namespaces, (GFunc)gc_web_service_free_ns, NULL);
	g_list_free (self->namespaces);
	
	xmlXPathFreeContext (self->xpath_ctx);
	
	((GObjectClass *) gc_web_service_parent_class)->finalize (obj);
}

static void
gc_web_service_class_init (GcWebServiceClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	o_class->finalize = gc_web_service_finalize;
}


gboolean
gc_web_service_set_base_url (GcWebService *self, gchar *url)
{
	g_return_val_if_fail (url, FALSE);
	
	g_free (self->base_url);
	g_free (self->response);
	self->response = NULL;
	self->response_length = 0;
	
	g_list_foreach (self->namespaces, (GFunc)gc_web_service_free_ns, NULL);
	g_list_free (self->namespaces);
	self->namespaces = NULL;
	
	xmlXPathFreeContext (self->xpath_ctx);
	self->xpath_ctx = NULL;
	
	self->base_url = g_strdup (url);
	
	return TRUE;
}
 
gboolean
gc_web_service_query (GcWebService *self, ...)
{
	va_list list;
	gchar *key, *value, *esc_value, *tmp, *url;
	gboolean first_pair = TRUE;
	
	g_return_val_if_fail (self->base_url, FALSE);
	
	url = g_strdup (self->base_url);
	
	/* read the arguments one key-value pair at a time,
	   add the pairs to url as "?key1=value1&key2=value2&..." */
	va_start (list, self);
	key = va_arg (list, char*);
	while (key) {
		value = va_arg (list, char*);
		esc_value = (gchar *)xmlURIEscapeStr ((xmlChar *)value, NULL);
		g_return_val_if_fail (esc_value, FALSE);
		if (first_pair) {
			tmp = g_strdup_printf ("%s?%s=%s",  url, key, esc_value);
			first_pair = FALSE;
		} else {
			tmp = g_strdup_printf ("%s&%s=%s",  url, key, esc_value);
		}
		g_free (esc_value);
		g_free (url);
		url = tmp;
		key = va_arg (list, char*);
	}
	va_end (list);
	
	if (!gc_web_service_fetch (self, url)) {
		g_free (url);
		return FALSE;
	}
	g_assert (self->response);
	g_free (url);
	
	return TRUE;
}


gboolean
gc_web_service_add_namespace (GcWebService *self, gchar *namespace, gchar *uri)
{
	XmlNamespace *ns;
	
	g_return_val_if_fail (self->base_url, FALSE);
	
	ns = g_new0(XmlNamespace,1);
	ns->name = g_strdup (namespace);
	ns->uri = g_strdup (uri);
	self->namespaces = g_list_prepend (self->namespaces, ns);
	return TRUE;
}


gboolean
gc_web_service_get_double (GcWebService *self, gdouble *OUT_value, gchar *xpath)
{
	gboolean retval = FALSE;
	
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (OUT_value, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!gc_web_service_build_xpath_context (self)) {
		return FALSE;
	}
	g_assert (self->xpath_ctx);
	
	xmlXPathObject *xpathObj = xmlXPathEvalExpression ((xmlChar*)xpath, self->xpath_ctx);
	if (xpathObj) {
		if (xpathObj->nodesetval && !xmlXPathNodeSetIsEmpty (xpathObj->nodesetval)) {
			*OUT_value = xmlXPathCastNodeSetToNumber(xpathObj->nodesetval);
			retval = TRUE;
		}
		xmlXPathFreeObject (xpathObj);
	}
	return retval;
}

gboolean
gc_web_service_get_string (GcWebService *self, gchar **OUT_value, gchar* xpath)
{
	gboolean retval= FALSE;
	
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (OUT_value, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!gc_web_service_build_xpath_context (self)) {
		return FALSE;
	}
	g_assert (self->xpath_ctx);
	
	xmlXPathObject *xpathObj = xmlXPathEvalExpression ((xmlChar*)xpath, self->xpath_ctx);
	if (xpathObj) {
		if (xpathObj->nodesetval && !xmlXPathNodeSetIsEmpty (xpathObj->nodesetval)) {
			*OUT_value = g_strdup ((char*)xmlXPathCastNodeSetToString (xpathObj->nodesetval));
			retval = TRUE;
		}
		xmlXPathFreeObject (xpathObj);
	}
	return retval;
}

gboolean
gc_web_service_get_response (GcWebService *self, guchar **response, gint *response_length)
{
	g_return_val_if_fail (self->response, FALSE);
	
	*response = g_memdup (self->response, self->response_length);
	*response_length = self->response_length;
	return TRUE;
}
