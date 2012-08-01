/*
 * Geoclue
 * geoclue-skyhook.c - A skyhook.com-based Address/Position provider
 * 
 * Author: Bastien Nocera <hadess@hadess.net>
 * Copyright 2009 Bastien Nocera
 */

#include <config.h>

#include <time.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>
#include <libsoup/soup.h>
#include <libxml/xpathInternals.h>

#include <geoclue/gc-web-service.h>
#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-position.h>

#include "connectivity.h"

#define GEOCLUE_DBUS_SERVICE_SKYHOOK "org.freedesktop.Geoclue.Providers.Skyhook"
#define GEOCLUE_DBUS_PATH_SKYHOOK "/org/freedesktop/Geoclue/Providers/Skyhook"
#define SKYHOOK_URL "https://api.skyhookwireless.com/wps2/location"
#define SKYHOOK_LAT_XPATH "//prefix:latitude"
#define SKYHOOK_LON_XPATH "//prefix:longitude"
#define USER_AGENT "Geoclue "VERSION

#define GEOCLUE_TYPE_SKYHOOK (geoclue_skyhook_get_type ())
#define GEOCLUE_SKYHOOK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_SKYHOOK, GeoclueSkyhook))

#define QUERY_START "<?xml version=\'1.0\'?><LocationRQ xmlns=\'http://skyhookwireless.com/wps/2005\' version=\'2.6\' street-address-lookup=\'full\'><authentication version=\'2.0\'><simple><username>beta</username><realm>js.loki.com</realm></simple></authentication>"
#define QUERY_AP "<access-point><mac>%s</mac><signal-strength>%d</signal-strength></access-point>"
#define QUERY_END "</LocationRQ>"

typedef struct _GeoclueSkyhook {
	GcProvider parent;
	GMainLoop *loop;
	SoupSession *session;
	GeoclueConnectivity *conn;
} GeoclueSkyhook;

typedef struct _GeoclueSkyhookClass {
	GcProviderClass parent_class;
} GeoclueSkyhookClass;


static void geoclue_skyhook_init (GeoclueSkyhook *skyhook);
static void geoclue_skyhook_position_init (GcIfacePositionClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueSkyhook, geoclue_skyhook, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                geoclue_skyhook_position_init))


/* Geoclue interface implementation */
static gboolean
geoclue_skyhook_get_status (GcIfaceGeoclue *iface,
			   GeoclueStatus  *status,
			   GError        **error)
{
	/* Assume available so long as all the requirements are satisfied
	   ie: Network is available */
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static void
_shutdown (GcProvider *provider)
{
	GeoclueSkyhook *skyhook = GEOCLUE_SKYHOOK (provider);
	g_main_loop_quit (skyhook->loop);
}

static void
add_ap (gpointer key,
	gpointer value,
	gpointer data)
{
	char *mac;
	char **split;

	/* Remove the ":" */
	split = g_strsplit (key, ":", -1);
	mac = g_strjoinv ("", split);
	g_strfreev (split);

	g_string_append_printf (data, QUERY_AP, mac, GPOINTER_TO_INT (value));
	g_free (mac);
}

static char *
create_post_query (GeoclueConnectivity *conn)
{
	GHashTable *ht;
	GString *str;

	ht = geoclue_connectivity_get_aps (conn);
	if (ht == NULL)
		return NULL;
	str = g_string_new (QUERY_START);
	g_hash_table_foreach (ht, add_ap, str);
	g_string_append (str, QUERY_END);
	g_hash_table_destroy (ht);

	return g_string_free (str, FALSE);
}

static gboolean
get_double (xmlXPathContext *xpath_ctx, char *xpath, gdouble *value)
{
	xmlXPathObject *obj = NULL;

	obj = xmlXPathEvalExpression (BAD_CAST (xpath), xpath_ctx);
	if (obj &&
	    (!obj->nodesetval || xmlXPathNodeSetIsEmpty (obj->nodesetval))) {
		xmlXPathFreeObject (obj);
		return FALSE;
	}
	*value = xmlXPathCastNodeSetToNumber (obj->nodesetval);
	xmlXPathFreeObject (obj);
	return TRUE;
}

static gboolean
parse_response (const char *body, gdouble *latitude, gdouble *longitude)
{
	xmlDocPtr doc;
	xmlXPathContext *xpath_ctx;
	gboolean ret = TRUE;

	doc = xmlParseDoc (BAD_CAST (body));
	if (!doc)
		return FALSE;

	xpath_ctx = xmlXPathNewContext(doc);
	if (!xpath_ctx) {
		xmlFreeDoc (doc);
		return FALSE;
	}
	xmlXPathRegisterNs (xpath_ctx, BAD_CAST ("prefix"), BAD_CAST("http://skyhookwireless.com/wps/2005"));
	if (get_double (xpath_ctx, SKYHOOK_LAT_XPATH, latitude) == FALSE) {
		ret = FALSE;
	} else if (get_double (xpath_ctx, SKYHOOK_LON_XPATH, longitude) == FALSE) {
		ret = FALSE;
	}
	xmlXPathFreeContext (xpath_ctx);
	xmlFreeDoc (doc);

	return ret;
}

/* Position interface implementation */

static gboolean 
geoclue_skyhook_get_position (GcIfacePosition        *iface,
                             GeocluePositionFields  *fields,
                             int                    *timestamp,
                             double                 *latitude,
                             double                 *longitude,
                             double                 *altitude,
                             GeoclueAccuracy       **accuracy,
                             GError                **error)
{
	GeoclueSkyhook *skyhook;
	char *query;
	SoupMessage *msg;
	
	skyhook = (GEOCLUE_SKYHOOK (iface));
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (timestamp)
		*timestamp = time (NULL);
	
	query = create_post_query (skyhook->conn);
	if (query == NULL) {
		g_set_error (error, GEOCLUE_ERROR, 
			     GEOCLUE_ERROR_NOT_AVAILABLE,
			     "Router mac address query failed");
		/* TODO: set status == error ? */
		return FALSE;
	}

	msg = soup_message_new ("POST", SKYHOOK_URL);
	soup_message_headers_append (msg->request_headers, "User-Agent", USER_AGENT);
	soup_message_set_request (msg,
				  "text/xml",
				  SOUP_MEMORY_TAKE,
				  query,
				  strlen (query));
	soup_session_send_message (skyhook->session, msg);
	if (msg->response_body == NULL || msg->response_body->data == NULL) {
		g_object_unref (msg);
		g_set_error (error, GEOCLUE_ERROR,
			     GEOCLUE_ERROR_NOT_AVAILABLE,
			     "Failed to query web service");
		return FALSE;
	}

	if (strstr (msg->response_body->data, "<error>") != NULL) {
		g_object_unref (msg);
		g_set_error (error, GEOCLUE_ERROR,
			     GEOCLUE_ERROR_NOT_AVAILABLE,
			     "Web service returned an error");
		return FALSE;
	}

	if (parse_response (msg->response_body->data, latitude, longitude) == FALSE) {
		g_object_unref (msg);
		g_set_error (error, GEOCLUE_ERROR,
			     GEOCLUE_ERROR_NOT_AVAILABLE,
			     "Couldn't parse response from web service");
		return FALSE;
	}

	if (latitude)
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	if (longitude)
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;

	if (accuracy) {
		if (*fields == GEOCLUE_POSITION_FIELDS_NONE) {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
							  0, 0);
		} else {
			/* Educated guess. Skyhook are typically hand pointed on 
			 * a map, or geocoded from address, so should be fairly 
			 * accurate */
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_STREET,
							  0, 0);
		}
	}

	return TRUE;
}

static void
geoclue_skyhook_finalize (GObject *obj)
{
	GeoclueSkyhook *skyhook = GEOCLUE_SKYHOOK (obj);

	if (skyhook->conn != NULL) {
		g_object_unref (skyhook->conn);
		skyhook->conn = NULL;
	}
	g_object_unref (skyhook->session);
	
	((GObjectClass *) geoclue_skyhook_parent_class)->finalize (obj);
}


/* Initialization */

static void
geoclue_skyhook_class_init (GeoclueSkyhookClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->shutdown = _shutdown;
	p_class->get_status = geoclue_skyhook_get_status;
	
	o_class->finalize = geoclue_skyhook_finalize;
}

static void
geoclue_skyhook_init (GeoclueSkyhook *skyhook)
{
	gc_provider_set_details (GC_PROVIDER (skyhook), 
	                         GEOCLUE_DBUS_SERVICE_SKYHOOK,
	                         GEOCLUE_DBUS_PATH_SKYHOOK,
	                         "Skyhook", "Skyhook.com based provider, uses gateway mac address to locate");
	skyhook->session = soup_session_sync_new ();
	skyhook->conn = geoclue_connectivity_new ();
}

static void
geoclue_skyhook_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = geoclue_skyhook_get_position;
}

int 
main()
{
	g_type_init();
#if !GLIB_CHECK_VERSION(2,31,0)
	g_thread_init (NULL);
#endif

	GeoclueSkyhook *o = g_object_new (GEOCLUE_TYPE_SKYHOOK, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);

	g_main_loop_run (o->loop);
	
	g_main_loop_unref (o->loop);
	g_object_unref (o);
	
	return 0;
}
