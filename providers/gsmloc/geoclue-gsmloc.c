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
  * Gsmloc provider is a position provider that uses GSM cell location and 
  * the webservice http://www.opencellid.org/ (a similar service
  * used to live at gsmloc.org, hence the name).
  * 
  * Gsmloc requires the telephony stack oFono to work.
  *
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

/* generated ofono bindings */
#include "ofono-marshal.h"
#include "ofono-manager-bindings.h"
#include "ofono-modem-bindings.h"
#include "ofono-network-registration-bindings.h"
#include "ofono-network-operator-bindings.h"

#define GEOCLUE_DBUS_SERVICE_GSMLOC "org.freedesktop.Geoclue.Providers.Gsmloc"
#define GEOCLUE_DBUS_PATH_GSMLOC "/org/freedesktop/Geoclue/Providers/Gsmloc"

#define OPENCELLID_URL "http://www.opencellid.org/cell/get"
#define OPENCELLID_LAT "/rsp/cell/@lat"
#define OPENCELLID_LON "/rsp/cell/@lon"
#define OPENCELLID_CID "/rsp/cell/@cellId"

#define GEOCLUE_TYPE_GSMLOC (geoclue_gsmloc_get_type ())
#define GEOCLUE_GSMLOC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GSMLOC, GeoclueGsmloc))

typedef struct _GeoclueGsmloc GeoclueGsmloc;

typedef struct _NetOp {
	GeoclueGsmloc *gsmloc;
	DBusGProxy *proxy;

	char *mcc;
	char *mnc;
} NetOp;

typedef struct _Modem {
	GeoclueGsmloc *gsmloc;
	DBusGProxy *proxy;
	DBusGProxy *netreg_proxy;
	GList *netops;

	char *lac;
	char *cid;
} Modem;

struct _GeoclueGsmloc {
	GcProvider parent;
	GMainLoop *loop;
	GcWebService *web_service;

	/* ofono abstraction */
	DBusGProxy *ofono_manager;
	GList *modems;

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
static void geoclue_gsmloc_cell_data_changed (GeoclueGsmloc *gsmloc);

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

	if (!gsmloc->ofono_manager) {
		*status = GEOCLUE_STATUS_ERROR;
	} else if (!gsmloc->modems) {
		/* no modems: this could change with usb devices etc. */
		*status = GEOCLUE_STATUS_UNAVAILABLE;
	} else { 
		/* Assume available so long as all the requirements are satisfied
		   ie: Network is available */

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
net_op_set_mnc (NetOp *op, const char *mnc)
{
	if (g_strcmp0 (op->mnc, mnc) == 0) {
		return FALSE;
	}

	g_free (op->mnc);
	op->mnc = g_strdup (mnc);
	return TRUE;
}

static gboolean
net_op_set_mcc (NetOp *op, const char *mcc)
{
	if (g_strcmp0 (op->mcc, mcc) == 0) {
		return FALSE;
	}

	g_free (op->mcc);
	op->mcc = g_strdup (mcc);
	return TRUE;
}

static void
net_op_property_changed_cb (DBusGProxy *proxy,
                            char *name,
                            GValue *value,
                            NetOp *op)
{
	if (g_strcmp0 ("MobileNetworkCode", name) == 0) {
		if (net_op_set_mnc (op, g_value_get_string (value))) {
			geoclue_gsmloc_cell_data_changed (op->gsmloc);
		}
	} else if (g_strcmp0 ("MobileCountryCode", name) == 0) {
		if (net_op_set_mcc (op, g_value_get_string (value))) {
			geoclue_gsmloc_cell_data_changed (op->gsmloc);
		}
	}
}

static void
net_op_free (NetOp *op)
{
	g_free (op->mcc);
	g_free (op->mnc);

	if (op->proxy) {
		dbus_g_proxy_disconnect_signal (op->proxy, "PropertyChanged",
		                                G_CALLBACK (net_op_property_changed_cb),
		                                op);
		g_object_unref (op->proxy);
	}

	g_slice_free (NetOp, op);
}

static gboolean
modem_set_lac (Modem *modem, const char *lac)
{
	if (g_strcmp0 (modem->lac, lac) == 0) {
		return FALSE;
	}

	g_free (modem->lac);
	modem->lac = g_strdup (lac);
	return TRUE;
}

static gboolean
modem_set_cid (Modem *modem, const char *cid)
{
	if (g_strcmp0 (modem->cid, cid) == 0) {
		return FALSE;
	}

	g_free (modem->cid);
	modem->cid = g_strdup (cid);
	return TRUE;
}

static void
get_netop_properties_cb (DBusGProxy *proxy,
                         GHashTable *props,
                         GError *error,
                         NetOp *op)
{
	GValue *mnc_val, *mcc_val;

	if (error) {
		g_warning ("oFono NetworkOperator.GetProperties failed: %s", error->message);
		g_error_free (error);
		return;
	}

	mnc_val = g_hash_table_lookup (props, "MobileNetworkCode");
	mcc_val = g_hash_table_lookup (props, "MobileCountryCode");
	if (mnc_val && mcc_val) {
		gboolean changed;
		changed = net_op_set_mcc (op, g_value_get_string (mcc_val));
		changed = net_op_set_mnc (op, g_value_get_string (mnc_val)) || changed;

		if (changed) {
			geoclue_gsmloc_cell_data_changed (op->gsmloc);
		}
	}
}

static void
modem_set_net_ops (Modem *modem, GPtrArray *ops)
{
	int i;
	GList *list;

	for (list = modem->netops; list; list = list->next) {
		net_op_free ((NetOp*)list->data);
	}
	g_list_free (modem->netops);
	modem->netops = NULL;

	if (ops->len == 0) {
		return;
	}

	for (i = 0; i < ops->len; i++) {
		const char* op_path;
		NetOp *op;

		op_path = g_ptr_array_index (ops, i);
		op = g_slice_new0 (NetOp);
		op->gsmloc = modem->gsmloc;
		op->proxy = dbus_g_proxy_new_from_proxy (modem->proxy,
												 "org.ofono.NetworkOperator",
												 op_path);
		if (!op->proxy) {
			g_warning ("failed to find the oFono NetworkOperator '%s'", op_path);
		} else {
			modem->netops = g_list_prepend (modem->netops, op);
			org_ofono_NetworkOperator_get_properties_async (op->proxy,
															(org_ofono_NetworkOperator_get_properties_reply)get_netop_properties_cb,
															op);
			dbus_g_proxy_add_signal (op->proxy,"PropertyChanged",
									 G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal (op->proxy, "PropertyChanged",
										 G_CALLBACK (net_op_property_changed_cb),
										 op, NULL);
		}
	}
}

static void
net_reg_get_properties_cb (DBusGProxy *proxy,
                           GHashTable *props,
                           GError *error,
                           Modem *modem)
{
	GValue *lac_val, *cid_val, *ops_val;

	if (error) {
		g_warning ("oFono NetworkRegistration.GetProperties failed: %s", error->message);
		g_error_free (error);
		return;
	}

	lac_val = g_hash_table_lookup (props, "LocationAreaCode");
	cid_val = g_hash_table_lookup (props, "CellId");
	ops_val = g_hash_table_lookup (props, "AvailableOperators");

	if (lac_val && cid_val) {
		gboolean changed;
		char *str;

		str = g_strdup_printf ("%u", g_value_get_uint (lac_val));
		changed = modem_set_lac (modem, str);
		g_free (str);
		str = g_strdup_printf ("%u", g_value_get_uint (cid_val));
		changed = modem_set_cid (modem, str) || changed;
		g_free (str);

		if (changed) {
			geoclue_gsmloc_cell_data_changed (modem->gsmloc);
		}
	}

	if (ops_val) {
		GPtrArray *ops;

		ops = g_value_get_boxed (ops_val);
		modem_set_net_ops (modem, ops);
	}
}

static void
netreg_property_changed_cb (DBusGProxy *proxy,
                            char *name,
                            GValue *value,
                            Modem *modem)
{
	char *str;

	if (g_strcmp0 ("LocationAreaCode", name) == 0) {
		str = g_strdup_printf ("%u", g_value_get_uint (value));
		if (modem_set_lac (modem, str)) {
			geoclue_gsmloc_cell_data_changed (modem->gsmloc);
		}
		g_free (str);
	} else if (g_strcmp0 ("CellId", name) == 0) {
		str = g_strdup_printf ("%u", g_value_get_uint (value));
		if (modem_set_cid (modem, str)) {
			geoclue_gsmloc_cell_data_changed (modem->gsmloc);
		}
		g_free (str);
	} else if (g_strcmp0 ("AvailableOperators", name) == 0) {
		modem_set_net_ops (modem, g_value_get_boxed (value));
	}
}

static void
modem_set_net_reg (Modem *modem, gboolean net_reg)
{
	GList *netops;

	g_free (modem->cid);
	modem->cid = NULL;
	g_free (modem->lac);
	modem->lac = NULL;

	if (modem->netreg_proxy) {
		dbus_g_proxy_disconnect_signal (modem->netreg_proxy, "PropertyChanged",
		                                G_CALLBACK (netreg_property_changed_cb),
		                                modem);
		g_object_unref (modem->netreg_proxy);
		modem->netreg_proxy = NULL;
	}

	for (netops = modem->netops; netops; netops = netops->next) {
		net_op_free ((NetOp*)netops->data);
	}
	g_list_free (modem->netops);
	modem->netops = NULL;

	if (net_reg) {
		modem->netreg_proxy = dbus_g_proxy_new_from_proxy (modem->proxy,
		                                                   "org.ofono.NetworkRegistration",
		                                                   dbus_g_proxy_get_path (modem->proxy));
		if (!modem->netreg_proxy) {
			g_warning ("failed to find the oFono NetworkRegistration '%s'",
			           dbus_g_proxy_get_path (modem->proxy));
		} else {
			org_ofono_NetworkRegistration_get_properties_async (modem->netreg_proxy,
																(org_ofono_NetworkRegistration_get_properties_reply)net_reg_get_properties_cb,
																modem);
			dbus_g_proxy_add_signal (modem->netreg_proxy,"PropertyChanged",
									 G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal (modem->netreg_proxy, "PropertyChanged",
										 G_CALLBACK (netreg_property_changed_cb),
										 modem, NULL);
		}
	}
}

static void
modem_set_interfaces (Modem *modem, char **ifaces)
{
	int i = 0;

	while (ifaces[i]) {
		if (g_strcmp0 ("org.ofono.NetworkRegistration", ifaces[i]) == 0) {
			if (!modem->netreg_proxy) {
				modem_set_net_reg (modem, TRUE);
			}
			return;
		}
		i++;
	}
	modem_set_net_reg (modem, FALSE);
}

static void
modem_property_changed_cb (DBusGProxy *proxy,
                           char *name,
                           GValue *value,
                           Modem *modem)
{
	if (g_strcmp0 ("Interfaces", name) == 0) {
		modem_set_interfaces (modem, g_value_get_boxed (value));
		geoclue_gsmloc_cell_data_changed (modem->gsmloc);
	}
}

static void
modem_free (Modem *modem)
{
	GList *netops;

	g_free (modem->cid);
	g_free (modem->lac);

	if (modem->netreg_proxy) {
		dbus_g_proxy_disconnect_signal (modem->netreg_proxy, "PropertyChanged",
		                                G_CALLBACK (netreg_property_changed_cb),
		                                modem);
		g_object_unref (modem->netreg_proxy);
	}

	if (modem->proxy) {
		dbus_g_proxy_disconnect_signal (modem->proxy, "PropertyChanged",
		                                G_CALLBACK (modem_property_changed_cb),
		                                modem);
		g_object_unref (modem->proxy);
	}

	for (netops = modem->netops; netops; netops = netops->next) {
		net_op_free ((NetOp*)netops->data);
	}
	g_list_free (modem->netops);

	g_slice_free (Modem, modem);
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
geoclue_gsmloc_cell_data_changed (GeoclueGsmloc *gsmloc)
{
	const char *mcc, *mnc, *lac, *cid; 
	GList *modems, *netops;

	mcc = mnc = lac = cid = NULL;

	/* find the first complete cell data we have */
	for (modems = gsmloc->modems; modems; modems = modems->next) {
		Modem *modem = (Modem*)modems->data;

		if (modem->lac && modem->cid) {
			for (netops = modem->netops; netops; netops = netops->next) {
				NetOp *netop = (NetOp*)netops->data;

				if (netop->mnc && netop->mcc) {
					mcc = netop->mcc;
					mnc = netop->mnc;
					lac = modem->lac;
					cid = modem->cid;
					break;
				}
			}
		}
		if (cid) {
			break;
		}
	}

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
modem_get_properties_cb (DBusGProxy *proxy,
                         GHashTable *props,
                         GError *error,
                         Modem *modem)
{
	GValue *val;

	if (error) {
		g_warning ("oFono Modem.GetProperties failed: %s", error->message);
		g_error_free (error);
		return;
	}

	val = g_hash_table_lookup (props, "Interfaces");
	modem_set_interfaces (modem, g_value_get_boxed (val));
}

static void
geoclue_gsmloc_set_modems (GeoclueGsmloc *gsmloc, GPtrArray *modems)
{
	int i;
	GList *mlist;

	/* empty current modem list */
	for (mlist = gsmloc->modems; mlist; mlist = mlist->next) {
		modem_free ((Modem*)mlist->data);
	}
	g_list_free (gsmloc->modems);
	gsmloc->modems = NULL;

	if (!modems || modems->len == 0) {
		return;
	}

	for (i = 0; i < modems->len; i++) {
		const char* str;
		Modem *modem;

		str = (const char*)g_ptr_array_index (modems, i);

		modem = g_slice_new0 (Modem);
		modem->gsmloc = gsmloc;
		modem->lac = NULL;
		modem->cid = NULL;
		modem->proxy = dbus_g_proxy_new_from_proxy (gsmloc->ofono_manager,
		                                            "org.ofono.Modem",
		                                            str);
		if (!modem->proxy) {
			g_warning ("failed to find the oFono Modem '%s'", str);
		} else {
			gsmloc->modems = g_list_prepend (gsmloc->modems, modem);
			org_ofono_Modem_get_properties_async (modem->proxy,
			                                      (org_ofono_Manager_get_properties_reply)modem_get_properties_cb,
			                                      modem);
			dbus_g_proxy_add_signal (modem->proxy,"PropertyChanged",
			                         G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal (modem->proxy, "PropertyChanged",
			                             G_CALLBACK (modem_property_changed_cb),
			                             modem, NULL);
		}
	}
}

static void
manager_get_properties_cb (DBusGProxy *proxy,
                           GHashTable *props,
                           GError *error,
                           GeoclueGsmloc *gsmloc)
{
	GValue *val;
	GPtrArray *modems = NULL;

	if (error) {
		g_warning ("oFono Manager.GetProperties failed: %s", error->message);
		g_error_free (error);
		return;
	}

	val = g_hash_table_lookup (props, "Modems");
	if (val) {
		modems = (GPtrArray*)g_value_get_boxed (val);
	}
	geoclue_gsmloc_set_modems (gsmloc, modems);
}

static void
manager_property_changed_cb (DBusGProxy *proxy,
                             char *name,
                             GValue *value,
                             GeoclueGsmloc *gsmloc)
{
	if (g_strcmp0 ("Modems", name) == 0) {
		GPtrArray *modems;

		modems = (GPtrArray*)g_value_get_boxed (value);
		geoclue_gsmloc_set_modems (gsmloc, modems);
	}
}

static void
geoclue_gsmloc_dispose (GObject *obj)
{
	GeoclueGsmloc *gsmloc = GEOCLUE_GSMLOC (obj);
	GList *mlist;

	if (gsmloc->ofono_manager) {
		dbus_g_proxy_disconnect_signal (gsmloc->ofono_manager, "PropertyChanged",
		                                G_CALLBACK (manager_property_changed_cb),
		                                gsmloc);
		g_object_unref (gsmloc->ofono_manager);
		gsmloc->ofono_manager = NULL;
	}

	if (gsmloc->modems) {
		for (mlist = gsmloc->modems; mlist; mlist = mlist->next) {
			modem_free ((Modem*)mlist->data);
		}
		g_list_free (gsmloc->modems);
		gsmloc->modems = NULL;
	}

	if (gsmloc->web_service) {
		g_object_unref (gsmloc->web_service);
		gsmloc->web_service = NULL;
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

	dbus_g_object_register_marshaller (ofono_marshal_VOID__STRING_BOXED,
	                                   G_TYPE_NONE,
	                                   G_TYPE_STRING,
	                                   G_TYPE_VALUE,
	                                   G_TYPE_INVALID);
}

static void
geoclue_gsmloc_init (GeoclueGsmloc *gsmloc)
{
	DBusGConnection *system_bus;

	gc_provider_set_details (GC_PROVIDER (gsmloc), 
	                         GEOCLUE_DBUS_SERVICE_GSMLOC,
	                         GEOCLUE_DBUS_PATH_GSMLOC,
	                         "Gsmloc", "GSM cell based position provider");

	gsmloc->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (gsmloc->web_service, OPENCELLID_URL);

	gsmloc->modems = NULL;
	geoclue_gsmloc_set_cell (gsmloc, NULL, NULL, NULL, NULL);

	system_bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);
	if (!system_bus) {
		g_warning ("failed to connect to DBus system bus");
		return;
	}

	gsmloc->ofono_manager = 
	    dbus_g_proxy_new_for_name (system_bus,
	                               "org.ofono",
	                               "/",
	                               "org.ofono.Manager");
	if (!gsmloc->ofono_manager) {
		g_warning ("failed to find the oFono Manager DBus object");
		return;
	}

	org_ofono_Manager_get_properties_async (gsmloc->ofono_manager,
	                                        (org_ofono_Manager_get_properties_reply)manager_get_properties_cb,
	                                        gsmloc);
	dbus_g_proxy_add_signal (gsmloc->ofono_manager,"PropertyChanged",
	                         G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (gsmloc->ofono_manager, "PropertyChanged",
	                             G_CALLBACK (manager_property_changed_cb),
	                             gsmloc, NULL);
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
