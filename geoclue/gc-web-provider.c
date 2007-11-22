#include <stdarg.h>
#include <glib-object.h>

#include <libxml/nanohttp.h>
#include <libxml/xpathInternals.h>
#include <libxml/uri.h>      /* for xmlURIEscapeStr */

#include <geoclue/gc-provider.h>
#include "gc-web-provider.h"

G_DEFINE_ABSTRACT_TYPE (GcWebProvider, gc_web_provider, GC_TYPE_PROVIDER)

typedef struct _XmlNamespace {
    gchar *name;
    gchar *uri;
}XmlNamespace;

/* enum for gobject properties */
enum {
        GC_WEB_PROVIDER_URL = 1,
        GC_WEB_PROVIDER_RESPONSE,
        GC_WEB_PROVIDER_RESPONSE_LENGTH,
};

/* GFunc, use with g_list_foreach */
static void
gc_web_provider_register_ns (gpointer data, gpointer user_data)
{
	GcWebProvider *self = (GcWebProvider *)user_data;
	XmlNamespace *ns = (XmlNamespace *)data;
	xmlXPathRegisterNs (self->xpath_ctx, 
	                    (xmlChar*)ns->name, (xmlChar*)ns->uri);
}

/* GFunc, use with g_list_foreach */
static void
gc_web_provider_free_ns (gpointer data, gpointer user_data)
{
	XmlNamespace *ns = (XmlNamespace *)data;
	g_free (ns->name);
	g_free (ns->uri);
	g_free (ns);
}


/* Register namespaces listed in self->namespaces */
static void
gc_web_provider_register_namespaces (GcWebProvider *self)
{
	g_assert (self->xpath_ctx);
	g_list_foreach (self->namespaces, (GFunc)gc_web_provider_register_ns, self);
}

/* Parse data (self->response), build xpath context and register 
 * namespaces. Nothing will be done if xpath context exists already. */
static gboolean
gc_web_provider_build_xpath_context (GcWebProvider *self)
{
	xmlDocPtr doc;
	
	g_assert (self->response);
	
	/* don't rebuild if there's no need */
	if (self->xpath_ctx) {
		return TRUE;
	}
	
	/* make sure response is NULL-terminated */
	xmlChar *tmp = (xmlChar *)g_strndup((gchar *)self->response, self->response_length);
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
	gc_web_provider_register_namespaces (self);
	return TRUE;
}

/* fetch data from url, save into self->response */
static gboolean
gc_web_provider_fetch (GcWebProvider *self, gchar *url)
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
gc_web_provider_init (GcWebProvider *self)
{
	self->response = NULL;
	self->response_length = 0;
	self->xpath_ctx = NULL;
	self->namespaces = NULL;
	self->base_url = NULL;
}


static void
gc_web_provider_finalize (GObject *obj)
{
	GcWebProvider *self = (GcWebProvider *) obj;
	
	g_free (self->base_url);
	g_free (self->response);
	
	g_list_foreach (self->namespaces, (GFunc)gc_web_provider_free_ns, NULL);
	g_list_free (self->namespaces);
	
	xmlXPathFreeContext (self->xpath_ctx);
	
	((GObjectClass *) gc_web_provider_parent_class)->finalize (obj);
}

static void
gc_web_provider_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
	GcWebProvider *self = (GcWebProvider *) object;
	
	switch (property_id) {
		case GC_WEB_PROVIDER_URL:
			g_free (self->base_url);
			self->base_url = NULL;
			g_free (self->response);
			self->response = NULL;
			self->response_length = 0;
			g_list_foreach (self->namespaces, (GFunc)gc_web_provider_free_ns, NULL);
			g_list_free (self->namespaces);
			xmlXPathFreeContext (self->xpath_ctx);
			self->xpath_ctx = NULL;
			self->base_url = g_value_dup_string (value);
			g_debug ("set base_url: %s\n",self->base_url);
			break;
		case GC_WEB_PROVIDER_RESPONSE:
			g_assert_not_reached();
		case GC_WEB_PROVIDER_RESPONSE_LENGTH:
			g_assert_not_reached();
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gc_web_provider_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
	g_return_if_fail (GC_IS_WEB_PROVIDER (object));
	GcWebProvider *self = (GcWebProvider *) object;
	gpointer copy;
	
	switch (property_id) {
		case GC_WEB_PROVIDER_URL:
			g_value_set_string (value, self->base_url);
			break;
		case GC_WEB_PROVIDER_RESPONSE:
			copy = g_memdup (self->response, self->response_length);
			g_value_set_pointer (value, copy);
			break;
		case GC_WEB_PROVIDER_RESPONSE_LENGTH:
			g_value_set_int (value, self->response_length);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gc_web_provider_class_init (GcWebProviderClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec *param_spec;
	
	g_debug ("web_provider class init");
	
	gobject_class->set_property = gc_web_provider_set_property;
	gobject_class->get_property = gc_web_provider_get_property;
	gobject_class->finalize = gc_web_provider_finalize;
	
	param_spec = g_param_spec_string ("base-url",
	                                  "base-url",
	                                  "Set web service base url",
	                                  NULL,
	                                  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
	                                 GC_WEB_PROVIDER_URL,
	                                 param_spec);
	
	param_spec = g_param_spec_pointer ("response",
	                                  "response",
	                                  "Get copy of the response to the last web service query",
	                                  G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
	                                 GC_WEB_PROVIDER_RESPONSE,
	                                 param_spec);
	                                 
	param_spec = g_param_spec_int ("response-length",
	                               "response-length",
	                               "Get the response length of the last web service query",
	                               0, G_MAXINT, 0,
	                               G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
	                                 GC_WEB_PROVIDER_RESPONSE_LENGTH,
	                                 param_spec);
	
}

 
gboolean
gc_web_provider_query (GcWebProvider *self, ...)
{
	va_list list;
	gchar *key, *value,  *esc_value, *tmp, *url;
	gboolean first_pair = TRUE;
	
	g_return_val_if_fail (self->base_url, FALSE);
	
	url = g_strdup (self->base_url);
	
	/* read the arguments one key-value pair at a time,
	   add the pairs to url */
	va_start (list, self);
	key = va_arg (list, char*);
	while (key) {
		/*TODO: value should be cleaned...*/
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
	
	if (!gc_web_provider_fetch (self, url)) {
		g_free (url);
		return FALSE;
	}
	g_assert (self->response);
	g_free (url);
	
	g_debug ("response length: %d", self->response_length);
	
	return TRUE;
}


gboolean
gc_web_provider_add_namespace (GcWebProvider *self, gchar *namespace, gchar *uri)
{
	g_return_val_if_fail (self->base_url, FALSE);
	
	XmlNamespace *ns = g_new0(XmlNamespace,1);
	ns->name = g_strdup (namespace);
	ns->uri = g_strdup (uri);
	self->namespaces = g_list_prepend (self->namespaces, ns);
	return TRUE;
}


gboolean
gc_web_provider_get_double (GcWebProvider *self, gdouble *OUT_value, gchar *xpath)
{
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (OUT_value, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!gc_web_provider_build_xpath_context (self)) {
		return FALSE;
	}
	g_assert (self->xpath_ctx);
	
	gboolean retval = FALSE;
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
gc_web_provider_get_string (GcWebProvider *self, gchar **OUT_value, gchar* xpath)
{
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (OUT_value, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!gc_web_provider_build_xpath_context (self)) {
		return FALSE;
	}
	g_assert (self->xpath_ctx);
	
	gboolean retval= FALSE;
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
