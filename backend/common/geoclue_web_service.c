/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2007 Jussi Kukkonen
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <glib.h>
#include <libxml/nanohttp.h>
#include <libxml/uri.h>      /* for xmlURIEscapeStr */

#include "geoclue_web_service.h"

#ifdef HAVE_LIBCONIC

static void geoclue_web_service_conic_callback (ConIcConnection *connection,
                                                ConIcConnectionEvent *event,
                                                gpointer user_data)
{
	g_assert (GEOCLUE_IS_WEB_SERVICE (user_data));
	GeoclueWebService *self = (GeoclueWebService *) user_data;
	
	switch (con_ic_connection_event_get_status (event)) {
		case CON_IC_STATUS_CONNECTED:
			g_signal_emit (self, 
			               GEOCLUE_WEB_SERVICE_GET_CLASS (self)->connection_event_signal_id,
			               0, TRUE);
			break;
		case CON_IC_STATUS_DISCONNECTED:
			g_signal_emit (self, 
			               GEOCLUE_WEB_SERVICE_GET_CLASS (self)->connection_event_signal_id,
			               0, FALSE);
			break;
		default:
			break;
	}
}

static void geoclue_web_service_connection_events_init (GeoclueWebService *self)
{
	g_assert (GEOCLUE_IS_WEB_SERVICE (self));
	
	/* init dbus connection -- this needs to be done,
	   connection signals do not work otherwise */
	self->system_bus = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	dbus_connection_setup_with_g_main (self->system_bus, NULL);
	/* TODO: dbus error handling */
	
	/* setup the connection signal callback */
	self->net_connection = con_ic_connection_new ();
	g_object_set (self->net_connection, 
	              "automatic-connection-events", TRUE, 
	              NULL);
	g_signal_connect (self->net_connection,
	                  "connection-event",
	                  G_CALLBACK(geoclue_web_service_conic_callback),
	                  self);
	self->using_connection_events = TRUE;
	g_debug ("connection_events init");
}

static void geoclue_web_service_connection_events_deinit (GeoclueWebService *self)
{
	g_assert (GEOCLUE_IS_WEB_SERVICE (self));
	g_debug ("connection_events_deinit");
	/* should signal be disconnected too? */
	g_object_unref (self->net_connection);
	
	dbus_connection_disconnect (self->system_bus);
	dbus_connection_unref (self->system_bus);
	
	self->using_connection_events = FALSE;
}

#else
/* stubs when we do not have  any ic events available */

static void 
geoclue_web_service_connection_events_init (GeoclueWebService *self) {}

static void 
geoclue_web_service_connection_events_deinit (GeoclueWebService *self) {}

#endif /* HAVE_LIBCONIC */

/* GFunc, use with g_list_foreach */
static void
geoclue_web_service_free_ns (gpointer data, gpointer user_data)
{
	XmlNamespace *ns = (XmlNamespace *)data;
	g_free (ns->name);
	g_free (ns->uri);
	g_free (ns);
}

/* GFunc, use with g_list_foreach */
static void
geoclue_web_service_register_ns (gpointer data, gpointer user_data)
{
	g_assert (GEOCLUE_IS_WEB_SERVICE (user_data));
	GeoclueWebService *self = (GeoclueWebService *)user_data;
	XmlNamespace *ns = (XmlNamespace *)data;
	xmlXPathRegisterNs (self->xpath_ctx, 
	                    (xmlChar*)ns->name, (xmlChar*)ns->uri);
}

/* Register namespaces listed in self->namespaces */
static void
geoclue_web_service_register_namespaces (GeoclueWebService *self)
{
	g_assert (GEOCLUE_IS_WEB_SERVICE (self));
	g_assert (self->xpath_ctx);
	g_list_foreach (self->namespaces, (GFunc)geoclue_web_service_register_ns, self);
}

/* Parse data (self->response), build xpath context and register 
 * namespaces. Nothing will be done if xpath context exists already. */
static gboolean
geoclue_seb_service_build_xpath_context (GeoclueWebService *self)
{
	xmlDocPtr doc;
	
	g_assert (GEOCLUE_IS_WEB_SERVICE (self));
	g_assert (self->response);
	
	/* don't rebuild if there's no need */
	if (self->xpath_ctx) {
		return TRUE;
	}
	
	/*make sure response is NULL-terminated*/
	xmlChar *tmp = (xmlChar *)g_strndup(self->response, self->response_length);
	
	g_debug ("parsing response: %s", tmp);
	
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
	geoclue_web_service_register_namespaces (self);
	return TRUE;
}

/* fetch data from url, save into self->response */
static gboolean
geoclue_web_service_fetch (GeoclueWebService *self, gchar *url)
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
	g_debug ("fetch: %s", url);
	ctxt = xmlNanoHTTPMethod (url, "GET", NULL, NULL, NULL, 0);
	if (!ctxt) {
		g_debug ("xmlNanoHTTPMethod did not get a response.");
		return FALSE;
	}
	
	output = xmlBufferCreate ();
	while ((len = xmlNanoHTTPRead (ctxt, buf, sizeof(buf))) > 0) {
		if (xmlBufferAdd (output, buf, len) != 0) {
			g_debug ("xmlBufferAdd failed.");
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
geoclue_web_service_init (GTypeInstance *instance,
                          gpointer g_class)
{
	g_return_if_fail (GEOCLUE_IS_WEB_SERVICE (instance));
	GeoclueWebService *self = (GeoclueWebService *)instance;
	g_debug ("web_service init");
	
	self->response = NULL;
	self->response_length = 0;
	self->xpath_ctx = NULL;
	self->namespaces = NULL;
	self->base_url = NULL;
	self->using_connection_events = FALSE;
	
	geoclue_web_service_connection_events_init (self);
}


static void
geoclue_web_service_finalize (GObject *obj)
{
	g_return_if_fail (GEOCLUE_IS_WEB_SERVICE (obj));
	GeoclueWebService *self = (GeoclueWebService *)obj;
	g_debug ("web_service finalize");
	
	geoclue_web_service_connection_events_deinit (self);
	g_free (self->base_url);
	self->base_url = NULL;
	g_free (self->response);
	self->response = NULL;
	self->response_length = 0;
	
	g_list_foreach (self->namespaces, (GFunc)geoclue_web_service_free_ns, NULL);
	g_list_free (self->namespaces);
	
	xmlXPathFreeContext (self->xpath_ctx);
	
	/*chain up*/
	GObjectClass *parent_class = g_type_class_peek_parent (GEOCLUE_WEB_SERVICE_GET_CLASS (obj));
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* enum for gobject properties */
enum {
        GEOCLUE_WEB_SERVICE_URL = 1,
        GEOCLUE_WEB_SERVICE_USING_CONNECTION_EVENTS,
        GEOCLUE_WEB_SERVICE_RESPONSE,
        GEOCLUE_WEB_SERVICE_RESPONSE_LENGTH,
};

static void
geoclue_web_service_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	g_return_if_fail (GEOCLUE_IS_WEB_SERVICE (object));
	GeoclueWebService *self = (GeoclueWebService *) object;
	
	switch (property_id) {
		case GEOCLUE_WEB_SERVICE_URL:
			g_free (self->base_url);
			self->base_url = NULL;
			g_free (self->response);
			self->response = NULL;
			self->response_length = 0;
			g_list_foreach (self->namespaces, (GFunc)geoclue_web_service_free_ns, NULL);
			g_list_free (self->namespaces);
			xmlXPathFreeContext (self->xpath_ctx);
			self->xpath_ctx = NULL;
			self->base_url = g_value_dup_string (value);
			g_debug ("set base_url: %s\n",self->base_url);
			break;
		case GEOCLUE_WEB_SERVICE_USING_CONNECTION_EVENTS:
			g_assert_not_reached();
		case GEOCLUE_WEB_SERVICE_RESPONSE:
			g_assert_not_reached();
		case GEOCLUE_WEB_SERVICE_RESPONSE_LENGTH:
			g_assert_not_reached();
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
geoclue_web_service_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
	g_return_if_fail (GEOCLUE_IS_WEB_SERVICE (object));
	GeoclueWebService *self = (GeoclueWebService *) object;
	gpointer copy;
	
	switch (property_id) {
		case GEOCLUE_WEB_SERVICE_URL:
			g_value_set_string (value, self->base_url);
			break;
		case GEOCLUE_WEB_SERVICE_USING_CONNECTION_EVENTS:
			g_value_set_boolean (value, self->using_connection_events);
			break;
		case GEOCLUE_WEB_SERVICE_RESPONSE:
			copy = g_memdup (self->response, self->response_length);
			g_value_set_pointer (value, copy);
			break;
		case GEOCLUE_WEB_SERVICE_RESPONSE_LENGTH:
			g_value_set_int (value, self->response_length);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
geoclue_web_service_connection_event_default_handler (GeoclueWebService *self, gboolean connected)
{
	g_return_if_fail (GEOCLUE_IS_WEB_SERVICE (self));
	g_debug ("connection event:%s connected", connected ? "" : " not");
}

static void
geoclue_web_service_class_init (gpointer klass,
                                gpointer klass_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GeoclueWebServiceClass *web_service_class = GEOCLUE_WEB_SERVICE_CLASS (klass);
	GParamSpec *param_spec;
	GType param_types[1];
	
	g_debug ("web_service class init");
	
	gobject_class->set_property = geoclue_web_service_set_property;
	gobject_class->get_property = geoclue_web_service_get_property;
	gobject_class->finalize = geoclue_web_service_finalize;
	
	param_spec = g_param_spec_string ("base-url",
	                                  "base-url",
	                                  "Set web service base url",
	                                  NULL,
	                                  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
	                                 GEOCLUE_WEB_SERVICE_URL,
	                                 param_spec);
	
	param_spec = g_param_spec_boolean ("using-connection-events",
	                                   "using-connection-events",
	                                   "True if there is an internet connection event system in use",
	                                   FALSE,
	                                   G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
	                                 GEOCLUE_WEB_SERVICE_USING_CONNECTION_EVENTS,
	                                 param_spec);
	
	param_spec = g_param_spec_pointer ("response",
	                                  "response",
	                                  "Get copy of the response to the last web service query",
	                                  G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
	                                 GEOCLUE_WEB_SERVICE_RESPONSE,
	                                 param_spec);
	                                 
	param_spec = g_param_spec_int ("response-length",
	                               "response-length",
	                               "Get the response length of the last web service query",
	                               0, G_MAXINT, 0,
	                               G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
	                                 GEOCLUE_WEB_SERVICE_RESPONSE_LENGTH,
	                                 param_spec);
	
	param_types[0] = G_TYPE_BOOLEAN;
	web_service_class->connection_event = geoclue_web_service_connection_event_default_handler;
	web_service_class->connection_event_signal_id =
	    g_signal_newv ("connection-event",
	                   G_TYPE_FROM_CLASS (klass),
	                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
	                   NULL,
	                   NULL,
	                   NULL,
	                   g_cclosure_marshal_VOID__BOOLEAN,
	                   G_TYPE_NONE,
	                   1,
	                   param_types);
}


GType
geoclue_web_service_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GeoclueWebServiceClass),
			NULL,                            /* base_init */
			NULL,                            /* base_finalize */
			geoclue_web_service_class_init,  /* class_init */
			NULL,                            /* class_finalize */
			NULL,                            /* class_data */
			sizeof (GeoclueWebService),
			0,                               /* n_preallocs */
			geoclue_web_service_init         /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
		                               "GeoclueWebServiceType",
		                               &info, 0);
	}
	return type;
}

gboolean
geoclue_web_service_query (GeoclueWebService *self, ...)
{
	va_list list;
	gchar *key, *value,  *esc_value, *tmp, *url;
	gboolean first_pair = TRUE;
	
	g_return_val_if_fail (GEOCLUE_IS_WEB_SERVICE (self), FALSE);
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
	
	if (!geoclue_web_service_fetch (self, url)) {
		g_free (url);
		return FALSE;
	}
	g_assert (self->response);
	g_free (url);
	
	g_debug ("response length: %d", self->response_length);
	
	return TRUE;
}


gboolean
geoclue_web_service_add_namespace (GeoclueWebService *self, gchar *namespace, gchar *uri)
{
	g_return_val_if_fail (GEOCLUE_IS_WEB_SERVICE (self), FALSE);
	g_return_val_if_fail (self->base_url, FALSE);
	
	XmlNamespace *ns = g_new0(XmlNamespace,1);
	ns->name = g_strdup (namespace);
	ns->uri = g_strdup (uri);
	self->namespaces = g_list_prepend (self->namespaces, ns);
}


gboolean
geoclue_web_service_get_double (GeoclueWebService *self, gdouble *OUT_value, gchar *xpath)
{
	g_return_val_if_fail (GEOCLUE_IS_WEB_SERVICE (self), FALSE);
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (OUT_value, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!geoclue_seb_service_build_xpath_context (self)) {
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
geoclue_web_service_get_string (GeoclueWebService *self, gchar **OUT_value, gchar* xpath)
{
	g_return_val_if_fail (GEOCLUE_IS_WEB_SERVICE (self), FALSE);
	g_return_val_if_fail (self->response, FALSE);
	g_return_val_if_fail (OUT_value, FALSE);
	g_return_val_if_fail (xpath, FALSE);
	
	/* parse the doc if not parsed yet and register namespaces */
	if (!geoclue_seb_service_build_xpath_context (self)) {
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
