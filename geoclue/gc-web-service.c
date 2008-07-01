/*
 * Geoclue
 * gc-web-service.c - A web service helper object for geoclue providers
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * 
 * Copyright 2007 Jussi Kukkonen (from old geoclue_web_service.c)
 * Copyright 2007, 2008 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:gc-web-service
 * @short_description: Web service helper object for Geoclue providers.
 *
 * #GcWebService is a web service abstraction for Geoclue provider 
 * implementations. It handles basic http stuff and xml parsing 
 * (although the raw data is available through 
 * gc_web_service_get_response() as well).
 * 
 * At the moment xml parsing functions only exist for double and 
 * char-array data types. Adding new functions is trivial, though.
 * <informalexample>
 * <programlisting>
 * . . .
 * 
 * #GcWebService *web_service;
 * web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
 * gc_web_service_set_base_url (web_service, "http://example.org");
 * 
 * / * Add namespaces if needed * / 
 * gc_web_service_add_namespace (web_service,
 *                               "ns_name", "http://example.org/ns");
 * 
 * . . .
 * 
 * / * Fetch document "http://api.example.org?key1=val1&key2=val2" * /
 * if (!gc_web_service_query (web_service, 
 *                            "key1", "val1"
 *                            "key2", val2"
 *                            (char *)0)) {
 * 	/ * error * /
 * 	return;
 * }
 * 
 * / * Use XPath expressions to parse the xml in fetched document * /
 * gchar *str;
 * if (gc_web_service_get_string (web_service,
 *                                &str, "//path/to/element")) {
 * 	g_debug("got string: %s", str);
 * }
 * 
 * gdouble number;
 * if (gc_web_service_get_double (web_service,
 *                                &number, "//path/to/another/element")) {
 * 	g_debug("got double: %f", number);
 * }
 * 
 * . . . 
 *
 * g_object_unref (G_OBJECT (web_service));
 * </programlisting>
 * </informalexample>
 */

#include <stdarg.h>
#include <glib-object.h>

#include <libxml/nanohttp.h>
#include <libxml/xpathInternals.h>
#include <libxml/uri.h>      /* for xmlURIEscapeStr */

#include "gc-web-service.h"
#include "geoclue-error.h"

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
gc_web_service_fetch (GcWebService *self, gchar *url, GError **error)
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
		*error = g_error_new (GEOCLUE_ERROR, 
		                      GEOCLUE_ERROR_NOT_AVAILABLE,
		                      g_strdup_printf ("xmlNanoHTTPMethod did not get a response from %s\n", url));
		return FALSE;
	}
	
	output = xmlBufferCreate ();
	while ((len = xmlNanoHTTPRead (ctxt, buf, sizeof(buf))) > 0) {
		if (xmlBufferAdd (output, buf, len) != 0) {
			xmlNanoHTTPClose(ctxt);
			xmlBufferFree (output);
			
			*error = g_error_new (GEOCLUE_ERROR, 
			                      GEOCLUE_ERROR_FAILED,
			                      g_strdup_printf ("libxml error (xmlBufferAdd failed)"));
			
			return FALSE;
		}
	}
	xmlNanoHTTPClose(ctxt);
	
	self->response_length = xmlBufferLength (output);
	self->response = g_memdup (xmlBufferContent (output), self->response_length);
	xmlBufferFree (output);
	
	return TRUE;
}

static xmlXPathObject*
gc_web_service_get_xpath_object (GcWebService *self, gchar* xpath)
{
	xmlXPathObject *obj = NULL;
	
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!gc_web_service_build_xpath_context (self)) {
		return FALSE;
	}
	g_assert (self->xpath_ctx);
	
	obj = xmlXPathEvalExpression ((xmlChar*)xpath, self->xpath_ctx);
	if (obj && 
	    (!obj->nodesetval || xmlXPathNodeSetIsEmpty (obj->nodesetval))) {
		xmlXPathFreeObject (obj);
		obj = NULL;
	}
	return obj;
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

/**
 * gc_web_service_set_base_url:
 * @self: The #GcWebService object
 * @url: base url
 * 
 * Sets base url for the web service. Must be called before calls to 
 * gc_web_service_get_* -methods.
 */
void
gc_web_service_set_base_url (GcWebService *self, gchar *url)
{
	g_assert (url);
	
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
}
 
/**
 * gc_web_service_add_namespace:
 * @self: The #GcWebService object
 * @namespace: Namespace name
 * @uri: Namespace uri 
 * 
 * Adds an xml namespace that will be used in all following calls to 
 * gc_web_service_get_*-functions. 
 *
 * Return value: %TRUE on success.
 */
gboolean
gc_web_service_add_namespace (GcWebService *self, gchar *namespace, gchar *uri)
{
	XmlNamespace *ns;
	
	g_return_val_if_fail (self->base_url, FALSE);
	
	ns = g_new0 (XmlNamespace,1);
	ns->name = g_strdup (namespace);
	ns->uri = g_strdup (uri);
	self->namespaces = g_list_prepend (self->namespaces, ns);
	return TRUE;
}

/**
 * gc_web_service_query:
 * @self: A #GcWebService object
 * @Varargs: NULL-terminated list of key-value gchar* pairs
 * 
 * Fetches data from the web. The url is constructed using the 
 * optional arguments as GET parameters (see example in the 
 * Description-section). Data should be read using 
 * gc_web_service_get_* -functions.
 *
 * Return value: %TRUE on success.
 */
gboolean
gc_web_service_query (GcWebService *self, GError **error, ...)
{
	va_list list;
	gchar *key, *value, *esc_value, *tmp, *url;
	gboolean first_pair = TRUE;
	
	g_return_val_if_fail (self->base_url, FALSE);
	
	url = g_strdup (self->base_url);
	
	/* read the arguments one key-value pair at a time,
	   add the pairs to url as "?key1=value1&key2=value2&..." */
	va_start (list, error);
	key = va_arg (list, char*);
	while (key) {
		value = va_arg (list, char*);
		esc_value = (gchar *)xmlURIEscapeStr ((xmlChar *)value, NULL);
		
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
	
	if (!gc_web_service_fetch (self, url, error)) {
		g_free (url);
		return FALSE;
	}
	g_assert (self->response);
	g_free (url);
	
	return TRUE;
}

/**
 * gc_web_service_get_double:
 * @self: A #GcWebService object
 * @value: Pointer to returned value
 * @xpath: XPath expression to find the value  
 * 
 * Extracts a @value from the data that was fetched in the last call 
 * to gc_web_service_query() using XPath expression @xpath. Returned 
 * value is the first match.
 *
 * Return value: %TRUE if a value was found.
 */
gboolean
gc_web_service_get_double (GcWebService *self, gdouble *value, gchar *xpath)
{
	xmlXPathObject *obj;
	
	obj = gc_web_service_get_xpath_object (self, xpath);
	if (!obj) {
		return FALSE;
	}
	*value = xmlXPathCastNodeSetToNumber (obj->nodesetval);
	xmlXPathFreeObject (obj);
	return TRUE;
}

/**
 * gc_web_service_get_double:
 * @self: The #GcWebService object
 * @value: returned gchar array
 * @xpath: XPath expression used to find the value  
 * 
 * Extracts a @value from the data that was fetched in the last call 
 * to gc_web_service_query() using XPath expression @xpath (returned 
 * value is the first match).
 *
 * Return value: %TRUE if a value was found.
 */
gboolean
gc_web_service_get_string (GcWebService *self, gchar **value, gchar* xpath)
{
	xmlXPathObject *obj;
	
	obj = gc_web_service_get_xpath_object (self, xpath);
	if (!obj) {
		return FALSE;
	}
	*value = g_strdup ((char*)xmlXPathCastNodeSetToString (obj->nodesetval));
	xmlXPathFreeObject (obj);
	return TRUE;
}

/**
 * gc_web_service_get_response:
 * @self: The #GcWebService object
 * @response: returned guchar array
 * @response_length: length of the returned array
 * 
 * Returns the raw data fetched with the last call to 
 * gc_web_service_query(). Data may be unterminated.
 *
 * Return value: %TRUE on success.
 */
gboolean
gc_web_service_get_response (GcWebService *self, guchar **response, gint *response_length)
{
	g_return_val_if_fail (self->response, FALSE);
	
	*response = g_memdup (self->response, self->response_length);
	*response_length = self->response_length;
	return TRUE;
}
