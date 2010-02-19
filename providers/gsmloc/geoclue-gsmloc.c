/*
 * Geoclue
 * geoclue-gsmloc.c - A GSM cell based Position provider
 * 
 * Author: Jussi Kukkonen <jku@linux.intel.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 *           2010 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

 /** 
  * Gsmloc provider is a position and address provider that uses GSM cell 
  * location and the webservice http://www.opencellid.org/ (a similar service
  * used to live at gsmloc.org, hence the name). The web service does not
  * provide any address data: that is done with a 
  * "mobile country code -> ISO country code" lookup table: as a result address
  * will only ever have country code and country fields.
  * 
  * Gsmloc requires the telephony stack oFono to work -- more IMSI data
  * sources could be added fairly easily. 
  **/
  
#include <config.h>

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>

#include <geoclue/gc-web-service.h>
#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-position.h>

/* ofono implementation */
#include "geoclue-gsmloc-ofono.h"

#define GEOCLUE_DBUS_SERVICE_GSMLOC "org.freedesktop.Geoclue.Providers.Gsmloc"
#define GEOCLUE_DBUS_PATH_GSMLOC "/org/freedesktop/Geoclue/Providers/Gsmloc"

#define OPENCELLID_URL "http://www.opencellid.org/cell/get"
#define OPENCELLID_LAT "/rsp/cell/@lat"
#define OPENCELLID_LON "/rsp/cell/@lon"
#define OPENCELLID_CID "/rsp/cell/@cellId"

#define GEOCLUE_TYPE_GSMLOC (geoclue_gsmloc_get_type ())
#define GEOCLUE_GSMLOC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GSMLOC, GeoclueGsmloc))

typedef struct _GeoclueGsmloc GeoclueGsmloc;

struct _GeoclueGsmloc {
	GcProvider parent;
	GMainLoop *loop;
	GcWebService *web_service;

	GeoclueGsmlocOfono *ofono;

	/* current data */
	char *mcc;
	char *mnc;
	char *lac;
	char *cid;
	GeocluePositionFields last_position_fields;
	GeoclueAccuracyLevel last_accuracy_level;
	double last_lat;
	double last_lon;
};

typedef struct _GeoclueGsmlocClass {
	GcProviderClass parent_class;
} GeoclueGsmlocClass;


static void geoclue_gsmloc_init (GeoclueGsmloc *gsmloc);
static void geoclue_gsmloc_position_init (GcIfacePositionClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueGsmloc, geoclue_gsmloc, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                geoclue_gsmloc_position_init))


/* Geoclue interface implementation */
static gboolean
geoclue_gsmloc_get_status (GcIfaceGeoclue *iface,
			   GeoclueStatus  *status,
			   GError        **error)
{
	GeoclueGsmloc *gsmloc = GEOCLUE_GSMLOC (iface);
	gboolean ofono_available;

	g_object_get (gsmloc->ofono, "available", &ofono_available, NULL);

	if (!ofono_available) {
		*status = GEOCLUE_STATUS_ERROR;
	} else if (!gsmloc->mcc || !gsmloc->mnc ||
	           !gsmloc->lac || !gsmloc->cid) {
		*status = GEOCLUE_STATUS_UNAVAILABLE;
	} else { 
		*status = GEOCLUE_STATUS_AVAILABLE;
	}
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueGsmloc *gsmloc = GEOCLUE_GSMLOC (provider);
	g_main_loop_quit (gsmloc->loop);
}

static gboolean
geoclue_gsmloc_query_opencellid (GeoclueGsmloc *gsmloc)
{
	double lat, lon;
	GeocluePositionFields fields = GEOCLUE_POSITION_FIELDS_NONE;
	GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;

	if (gsmloc->mcc && gsmloc->mnc &&
	    gsmloc->lac && gsmloc->cid) {

		if (gc_web_service_query (gsmloc->web_service, NULL,
		                          "mcc", gsmloc->mcc,
		                          "mnc", gsmloc->mnc,
		                          "lac", gsmloc->lac,
		                          "cellid", gsmloc->cid,
		                          (char *)0)) {

			if (gc_web_service_get_double (gsmloc->web_service, 
			                               &lat, OPENCELLID_LAT)) {
				fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
			}
			if (gc_web_service_get_double (gsmloc->web_service, 
			                               &lon, OPENCELLID_LON)) {
				fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
			}

			if (fields != GEOCLUE_POSITION_FIELDS_NONE) {
				char *retval_cid;
				/* if cellid is not present, location is for the local area code.
				 * the accuracy might be an overstatement -- I have no idea how 
				 * big LACs typically are */
				level = GEOCLUE_ACCURACY_LEVEL_LOCALITY;
				if (gc_web_service_get_string (gsmloc->web_service, 
				                               &retval_cid, OPENCELLID_CID)) {
					if (retval_cid && strlen (retval_cid) != 0) {
						level = GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
					}
					g_free (retval_cid);
				}
			}
		}
	}

	if (fields != gsmloc->last_position_fields ||
	    (fields != GEOCLUE_POSITION_FIELDS_NONE &&
	     (lat != gsmloc->last_lat ||
	      lon != gsmloc->last_lon ||
	      level != gsmloc->last_accuracy_level))) {
		GeoclueAccuracy *acc;

		/* position changed */

		gsmloc->last_position_fields = fields;
		gsmloc->last_accuracy_level = level;
		gsmloc->last_lat = lat;
		gsmloc->last_lon = lon;

		acc = geoclue_accuracy_new (gsmloc->last_accuracy_level, 0.0, 0.0);
		gc_iface_position_emit_position_changed (GC_IFACE_POSITION (gsmloc),
		                                         fields,
		                                         time (NULL),
		                                         lat, lon, 0.0,
		                                         acc);
		geoclue_accuracy_free (acc);
		return TRUE;
	}

	return FALSE;
}


static gboolean
geoclue_gsmloc_set_cell (GeoclueGsmloc *gsmloc,
                         const char *mcc, const char *mnc, 
                         const char *lac, const char *cid)
{
	g_free (gsmloc->mcc);
	g_free (gsmloc->mnc);
	g_free (gsmloc->lac);
	g_free (gsmloc->cid);

	gsmloc->mcc = g_strdup (mcc);
	gsmloc->mnc = g_strdup (mnc);
	gsmloc->lac = g_strdup (lac);
	gsmloc->cid = g_strdup (cid);

	return geoclue_gsmloc_query_opencellid (gsmloc);
}

static void
network_data_changed_cb (gpointer connection_manager,
                         const char *mcc, const char *mnc, 
                         const char *lac, const char *cid,
                         GeoclueGsmloc *gsmloc)
{
	if (g_strcmp0 (mcc, gsmloc->mcc) != 0 ||
	    g_strcmp0 (mnc, gsmloc->mnc) != 0 ||
	    g_strcmp0 (lac, gsmloc->lac) != 0 ||
	    g_strcmp0 (cid, gsmloc->cid) != 0) {

		/* new cell data, do a opencellid lookup */
		geoclue_gsmloc_set_cell (gsmloc, mcc, mnc, lac, cid);
	}
}

/* Position interface implementation */

static gboolean 
geoclue_gsmloc_get_position (GcIfacePosition        *iface,
                             GeocluePositionFields  *fields,
                             int                    *timestamp,
                             double                 *latitude,
                             double                 *longitude,
                             double                 *altitude,
                             GeoclueAccuracy       **accuracy,
                             GError                **error)
{
	GeoclueGsmloc *gsmloc;

	gsmloc = (GEOCLUE_GSMLOC (iface));

	if (gsmloc->last_position_fields == GEOCLUE_POSITION_FIELDS_NONE) {
		/* re-query in case there was a network problem */
		geoclue_gsmloc_query_opencellid (gsmloc);
	}

	if (timestamp) {
		*timestamp = time (NULL);
	}

	if (fields) {
		*fields = gsmloc->last_position_fields;
	}
	if (latitude) {
		*latitude = gsmloc->last_lat;
	}
	if (longitude) {
		*longitude = gsmloc->last_lon;
	}
	if (accuracy) {
		*accuracy = geoclue_accuracy_new (gsmloc->last_accuracy_level, 0, 0);
	}

	return TRUE;
}

static void
geoclue_gsmloc_dispose (GObject *obj)
{
	GeoclueGsmloc *gsmloc = GEOCLUE_GSMLOC (obj);

	if (gsmloc->ofono) {
		g_signal_handlers_disconnect_by_func (gsmloc->ofono,
		                                      network_data_changed_cb,
		                                      gsmloc);
		g_object_unref (gsmloc->ofono);
		gsmloc->ofono = NULL;
	}

	((GObjectClass *) geoclue_gsmloc_parent_class)->dispose (obj);
}


/* Initialization */

static void
geoclue_gsmloc_class_init (GeoclueGsmlocClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;

	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_gsmloc_get_status;

	o_class->dispose = geoclue_gsmloc_dispose;
}

static void
geoclue_gsmloc_init (GeoclueGsmloc *gsmloc)
{
	gsmloc->address = geoclue_address_details_new ();

	gc_provider_set_details (GC_PROVIDER (gsmloc), 
	                         GEOCLUE_DBUS_SERVICE_GSMLOC,
	                         GEOCLUE_DBUS_PATH_GSMLOC,
	                         "Gsmloc", "GSM cell based position provider");

	gsmloc->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (gsmloc->web_service, OPENCELLID_URL);

	geoclue_gsmloc_set_cell (gsmloc, NULL, NULL, NULL, NULL);

	/* init ofono*/
	gsmloc->ofono = geoclue_gsmloc_ofono_new ();
	g_signal_connect (gsmloc->ofono, "network-data-changed",
	                  G_CALLBACK (network_data_changed_cb), gsmloc);
}

static void
geoclue_gsmloc_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = geoclue_gsmloc_get_position;
}

int 
main()
{
	g_type_init();

	GeoclueGsmloc *o = g_object_new (GEOCLUE_TYPE_GSMLOC, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);

	g_main_loop_run (o->loop);

	g_main_loop_unref (o->loop);
	g_object_unref (o);

	return 0;
}
