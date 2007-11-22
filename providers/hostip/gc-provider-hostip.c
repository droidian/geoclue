/*
 * Geoclue
 * gc-provider-hostip.c - A hostip.info-based Address/Position provider
 *
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 */

#include <config.h>

#include <time.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-web-provider.h>

#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#include "gc-provider-hostip.h"

#define GC_DBUS_SERVICE_HOSTIP "org.freedesktop.Geoclue.Providers.Hostip"
#define GC_DBUS_PATH_HOSTIP "/org/freedesktop/Geoclue/Providers/Hostip"

#define HOSTIP_URL "http://api.hostip.info/"


static void gc_provider_hostip_init (GcProviderHostip *obj);
static void gc_provider_hostip_position_init (GcIfacePositionClass  *iface);
static void gc_provider_hostip_address_init (GcIfaceAddressClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GcProviderHostip, gc_provider_hostip, GC_TYPE_WEB_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                gc_provider_hostip_position_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
                                                gc_provider_hostip_address_init))


/* Geoclue interface implementation */

static gboolean
gc_provider_hostip_get_version (GcIfaceGeoclue  *iface,
                                int             *major,
                                int             *minor,
                                int             *micro,
                                GError         **error)
{
	*major = 1;
	*minor = 0;
	*micro = 0;
	return TRUE;
}

static gboolean
gc_provider_hostip_get_status (GcIfaceGeoclue  *iface,
                               gboolean        *available,
                               GError         **error)
{
	/* TODO: if connection status info is in master, how do we do this? */
	return TRUE;
}

static gboolean
gc_provider_hostip_shutdown (GcIfaceGeoclue  *iface,
                             GError         **error)
{
	GcProviderHostip *obj = GC_PROVIDER_HOSTIP (iface);
	g_main_loop_quit (obj->loop);
	return TRUE;
}

/* Position interface implementation */

static gboolean 
gc_provider_hostip_get_position (GcIfacePosition        *iface,
                                 GeocluePositionFields  *fields,
                                 int                    *timestamp,
                                 double                 *latitude,
                                 double                 *longitude,
                                 double                 *altitude,
                                 GeoclueAccuracy       **accuracy,
                                 GError                **error)
{
	GcWebProvider *obj = GC_WEB_PROVIDER (iface);
	gchar *coord_str = NULL;
	
	if (!gc_web_provider_query (obj, NULL)) {
		/* TODO: error handling */
		return FALSE;
	}
	
	if (!gc_web_provider_get_string (obj,
	                                 &coord_str,
	                                 "//gml:featureMember/hostip:Hostip//gml:coordinates")) {
		/* No data available */
		/* TODO: error handling */
		return FALSE;
	}
	
	/* Hostip xml has to most idiotic format ever. lat and lon in one xml element? */
	if (sscanf (coord_str, "%lf,%lf", longitude , latitude) != 2) {
		/* No data available */
		/* TODO: error handling */
		g_free (coord_str);
		return FALSE;
	}
	g_free (coord_str);
	
	*fields = GEOCLUE_POSITION_FIELDS_LONGITUDE;
	*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	
	/* TODO this is going to bite us in 2038 with int timestamps */
	time ((time_t *)timestamp);

	/* TODO fix accuracy: city level */
	*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_DETAILED,
	                                 20000, 20000);
	return TRUE;
}

/* Address interface implementation */

static gboolean 
gc_provider_hostip_get_address (GcIfaceAddress   *iface,
                                int              *timestamp,
                                GHashTable      **address,
                                GeoclueAccuracy **accuracy,
                                GError          **error)
{
	
	
	return FALSE;
	
}

/* Initialization */

static void
gc_provider_hostip_class_init (GcProviderHostipClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	
	p_class->get_version = gc_provider_hostip_get_version;
	p_class->get_status = gc_provider_hostip_get_status;
	p_class->shutdown = gc_provider_hostip_shutdown;
	
}

static void
gc_provider_hostip_init (GcProviderHostip *obj)
{
	gc_provider_set_details (GC_PROVIDER (obj), 
	                         GC_DBUS_SERVICE_HOSTIP,
	                         GC_DBUS_PATH_HOSTIP);
	
	gc_web_provider_set_base_url (GC_WEB_PROVIDER (obj), HOSTIP_URL);
	gc_web_provider_add_namespace (GC_WEB_PROVIDER (obj),
	                               "gml", "http://www.opengis.net/gml");
	gc_web_provider_add_namespace (GC_WEB_PROVIDER (obj),
	                               "hostip", "http://www.hostip.info/api");
}

static void
gc_provider_hostip_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = gc_provider_hostip_get_position;
}

static void
gc_provider_hostip_address_init (GcIfaceAddressClass  *iface)
{
	iface->get_address = gc_provider_hostip_get_address;
}

int 
main()
{
	g_type_init();
	
	GcProviderHostip *o = g_object_new (GC_TYPE_PROVIDER_HOSTIP, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (o->loop);
	
	g_main_loop_unref (o->loop);
	g_object_unref (o);
	
	return 0;
}
