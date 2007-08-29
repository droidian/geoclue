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

/* GeoclueWebService is meant to be used in webservice-based geoclue
 * provider implementations. It has two tasks:
 *   a) simplify interaction with webservices
 *   a) notify about connectivity changes
 * The only thing these two have in common is that they're usually 
 * needed in the same place.
 * 
 *** CONSTRUCTION
 * 
 * GeoclueWebservice takes the webservice base url as a construction
 * parameter (you can also set it using normal gobject property set). 
 * Example:
 * 	obj = g_object_new (GEOCLUE_TYPE_WEB_SERVICE, 
 * 	                    "base_url", "http://plazes.com/suggestions.xml",
 * 	                    NULL);
 * 
 *** CONNECTION EVENT
 * 
 * There's a "connection-event" signal for getting notified about 
 * connection events: 
 * 	g_signal_connect (GEOCLUE_WEB_SERVICE (obj),
 * 	                  "connection-event",
 * 	                  (GCallback)callback_func,
 * 	                  NULL);
 * This only works if the platform supports this: use 
 * "using-connection-events" property to check. Currently only Maemo
 * (libconic) support is included, but Gnome (NetworkManager) support
 * should be doable.
 * 
 *** QUERYING DATA
 *  
 * A typical web service query is done like this: 
 * 	if (geoclue_web_service_query (GEOCLUE_WEB_SERVICE (obj),
 * 	                               "mac_address", "00:11:95:20:df:11",
 * 	                               NULL)) {
 * 		if (geoclue_web_service_get_string (GEOCLUE_WEB_SERVICE (obj),
 * 		                                    &addr, "//plaze/address")) {
 * 			g_debug ("address: %s", addr);
 * 		}
 * 		if (geoclue_web_service_get_double (GEOCLUE_WEB_SERVICE (obj),
 * 		                                    &lat, "//plaze/latitude")) {
 * 			g_debug ("latitude: %f", lat);
 * 		}
 * 	}
 * 
 * So, geoclue_web_service_query takes a NULL-terminated list of 
 * key-value pairs that will be used as GET params. The value part 
 * of the pair will be escaped. The example case query url will be:
 * "http://plazes.com/suggestions.xml?mac_address=00%3A11%3A95%3A20%3Adf%3A11"
 * 
 * The actual response data is available (see "response"-property), 
 * but if the data is xml it's a lot easier to use 
 * "geoclue_web_service_get_*" -methods to get specific data using 
 * simple xpath expressions.
 * 
 * If the xml data uses namespaces, they should be added with 
 * geoclue_web_service_add_namespace before calling 
 * geoclue_web_service_query.
 */

#ifndef GEOCLUE_WEB_SERVICE_H
#define GEOCLUE_WEB_SERVICE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <libxml/xpath.h>

#ifdef HAVE_LIBCONIC
#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#include <conicconnection.h>
#include <conicconnectionevent.h>
#endif

G_BEGIN_DECLS

typedef struct _XmlNamespace XmlNamespace;
struct _XmlNamespace {
	gchar *name;
	gchar *uri;
};

#define GEOCLUE_TYPE_WEB_SERVICE            (geoclue_web_service_get_type ())
#define GEOCLUE_WEB_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_WEB_SERVICE, GeoclueWebService))
#define GEOCLUE_WEB_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_TYPE_WEB_SERVICE, GeoclueWebServiceClass))
#define GEOCLUE_IS_WEB_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_WEB_SERVICE))
#define GEOCLUE_IS_WEB_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_TYPE_WEB_SERVICE))
#define GEOCLUE_WEB_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCLUE_TYPE_WEB_SERVICE, GeoclueWebServiceClass))

typedef struct _GeoclueWebService GeoclueWebService;
typedef struct _GeoclueWebServiceClass GeoclueWebServiceClass;

struct _GeoclueWebService {
	GObject parent;
	
	/* private */
	gchar* base_url;
	guchar *response;
	gint response_length;
	GList *namespaces;
	gboolean using_connection_events;
	xmlXPathContext *xpath_ctx;
	
#ifdef HAVE_LIBCONIC
	ConIcConnection* net_connection;
	DBusConnection* system_bus;
#endif
};

struct _GeoclueWebServiceClass {
	GObjectClass parent;
	
	guint connection_event_signal_id;
	/* default handler for connection event */
	void (*connection_event) (GeoclueWebService *self,
	                          gboolean connected);
};

GType geoclue_web_service_get_type (void);


/* Public methods */
gboolean geoclue_web_service_query (GeoclueWebService *self, ...);
gboolean geoclue_web_service_add_namespace (GeoclueWebService *self, gchar *namespace, gchar *uri);

gboolean geoclue_web_service_get_string (GeoclueWebService *self, gchar **OUT_value, gchar *xpath);
gboolean geoclue_web_service_get_double (GeoclueWebService *self, gdouble *OUT_value, gchar *xpath);

G_END_DECLS

#endif /* GEOCLUE_WEB_SERVICE_H */
