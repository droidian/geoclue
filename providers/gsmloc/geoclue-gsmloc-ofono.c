/*
 * Geoclue
 * geoclue-gsmloc-ofono.c - oFono abstraction for gsmloc provider
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
  
#include <config.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>

#include "geoclue-gsmloc-ofono.h"

/* generated ofono bindings */
#include "ofono-marshal.h"
#include "ofono-manager-bindings.h"
#include "ofono-modem-bindings.h"
#include "ofono-network-registration-bindings.h"
#include "ofono-network-operator-bindings.h"

G_DEFINE_TYPE (GeoclueGsmlocOfono, geoclue_gsmloc_ofono, G_TYPE_OBJECT)
#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_GSMLOC_OFONO, GeoclueGsmlocOfonoPrivate))

typedef struct _GeoclueGsmlocOfonoPrivate {
	DBusGProxy *ofono_manager;
	GList *modems;
	gboolean available;
} GeoclueGsmlocOfonoPrivate;

enum {
	NETWORK_DATA_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0};

enum {
	PROP_0,
	PROP_AVAILABLE,
};

static void emit_network_data_changed (GeoclueGsmlocOfono *ofono);


typedef struct _NetOp {
	GeoclueGsmlocOfono *ofono;
	DBusGProxy *proxy;

	char *mcc;
	char *mnc;
} NetOp;

typedef struct _Modem {
	GeoclueGsmlocOfono *ofono;
	DBusGProxy *proxy;
	DBusGProxy *netreg_proxy;
	GList *netops;

	char *lac;
	char *cid;
} Modem;

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
			emit_network_data_changed (op->ofono);
		}
	} else if (g_strcmp0 ("MobileCountryCode", name) == 0) {
		if (net_op_set_mcc (op, g_value_get_string (value))) {
			emit_network_data_changed (op->ofono);
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
			emit_network_data_changed (op->ofono);
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
		op->ofono = modem->ofono;
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
			emit_network_data_changed (modem->ofono);
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
			emit_network_data_changed (modem->ofono);
		}
		g_free (str);
	} else if (g_strcmp0 ("CellId", name) == 0) {
		str = g_strdup_printf ("%u", g_value_get_uint (value));
		if (modem_set_cid (modem, str)) {
			emit_network_data_changed (modem->ofono);
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
		emit_network_data_changed (modem->ofono);
	}
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


static void 
emit_network_data_changed (GeoclueGsmlocOfono *ofono)
{
	GeoclueGsmlocOfonoPrivate *priv = GET_PRIVATE (ofono);
	const char *mcc, *mnc, *lac, *cid; 
	GList *modems, *netops;

	mcc = mnc = lac = cid = NULL;

	/* find the first complete cell data we have */
	for (modems = priv->modems; modems; modems = modems->next) {
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

	g_signal_emit (ofono, signals[NETWORK_DATA_CHANGED], 0,
	               mcc, mnc, lac, cid);
}


static void
geoclue_gsmloc_ofono_set_modems (GeoclueGsmlocOfono *ofono, GPtrArray *modems)
{
	GeoclueGsmlocOfonoPrivate *priv = GET_PRIVATE (ofono);
	int i;
	GList *mlist;

	/* empty current modem list */
	for (mlist = priv->modems; mlist; mlist = mlist->next) {
		modem_free ((Modem*)mlist->data);
	}
	g_list_free (priv->modems);
	priv->modems = NULL;

	if (!modems || modems->len == 0) {
		return;
	}

	for (i = 0; i < modems->len; i++) {
		const char* str;
		Modem *modem;

		str = (const char*)g_ptr_array_index (modems, i);

		modem = g_slice_new0 (Modem);
		modem->ofono = ofono;
		modem->lac = NULL;
		modem->cid = NULL;
		modem->proxy = dbus_g_proxy_new_from_proxy (priv->ofono_manager,
		                                            "org.ofono.Modem",
		                                            str);
		if (!modem->proxy) {
			g_warning ("failed to find the oFono Modem '%s'", str);
		} else {
			priv->modems = g_list_prepend (priv->modems, modem);
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
geoclue_gsmloc_ofono_get_property (GObject *obj, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
	GeoclueGsmlocOfono *ofono = GEOCLUE_GSMLOC_OFONO (obj);
	GeoclueGsmlocOfonoPrivate *priv = GET_PRIVATE (ofono);

	switch (property_id) {
	case PROP_AVAILABLE:
		g_value_set_boolean (value, priv->available);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}
}

static void
manager_get_properties_cb (DBusGProxy *proxy,
                           GHashTable *props,
                           GError *error,
                           GeoclueGsmlocOfono *ofono)
{
	GeoclueGsmlocOfonoPrivate *priv = GET_PRIVATE (ofono);
	GValue *val;
	GPtrArray *modems = NULL;

	if (error) {
		if(error->code != DBUS_GERROR_SERVICE_UNKNOWN) {
			g_warning ("oFono Manager.GetProperties failed: %s", error->message);
		}
		g_error_free (error);
		return;
	}

	val = g_hash_table_lookup (props, "Modems");
	if (val) {
		modems = (GPtrArray*)g_value_get_boxed (val);
	}
	geoclue_gsmloc_ofono_set_modems (ofono, modems);

	priv->available = TRUE;
}

static void
manager_property_changed_cb (DBusGProxy *proxy,
                             char *name,
                             GValue *value,
                             GeoclueGsmlocOfono *ofono)
{
	if (g_strcmp0 ("Modems", name) == 0) {
		GPtrArray *modems;

		modems = (GPtrArray*)g_value_get_boxed (value);
		geoclue_gsmloc_ofono_set_modems (ofono, modems);
	}
}

static void
geoclue_gsmloc_ofono_dispose (GObject *obj)
{
	GeoclueGsmlocOfono *ofono = GEOCLUE_GSMLOC_OFONO (obj);
	GeoclueGsmlocOfonoPrivate *priv = GET_PRIVATE (ofono);
	GList *mlist;

	if (priv->ofono_manager) {
		dbus_g_proxy_disconnect_signal (priv->ofono_manager, "PropertyChanged",
		                                G_CALLBACK (manager_property_changed_cb),
		                                ofono);
		g_object_unref (priv->ofono_manager);
		priv->ofono_manager = NULL;
	}

	if (priv->modems) {
		for (mlist = priv->modems; mlist; mlist = mlist->next) {
			modem_free ((Modem*)mlist->data);
		}
		g_list_free (priv->modems);
		priv->modems = NULL;
	}

	((GObjectClass *) geoclue_gsmloc_ofono_parent_class)->dispose (obj);
}

static void
geoclue_gsmloc_ofono_class_init (GeoclueGsmlocOfonoClass *klass)
{
	GObjectClass *o_class = (GObjectClass *)klass;
	GParamSpec *pspec;

	g_type_class_add_private (klass, sizeof (GeoclueGsmlocOfonoPrivate));

	o_class->dispose = geoclue_gsmloc_ofono_dispose;
	o_class->get_property = geoclue_gsmloc_ofono_get_property;

	dbus_g_object_register_marshaller (ofono_marshal_VOID__STRING_BOXED,
	                                   G_TYPE_NONE,
	                                   G_TYPE_STRING,
	                                   G_TYPE_VALUE,
	                                   G_TYPE_INVALID);

	signals[NETWORK_DATA_CHANGED] = g_signal_new (
			"network-data-changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_LAST, 0,
			NULL, NULL,
			ofono_marshal_VOID__STRING_STRING_STRING_STRING,
			G_TYPE_NONE, 4,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	pspec = g_param_spec_boolean ("available",
	                              "Available",
	                              "Is oFono available",
	                              FALSE,
	                              G_PARAM_READABLE);
	g_object_class_install_property (o_class, PROP_AVAILABLE, pspec);
}

static void
geoclue_gsmloc_ofono_init (GeoclueGsmlocOfono *ofono)
{
	GeoclueGsmlocOfonoPrivate *priv = GET_PRIVATE (ofono);
	DBusGConnection *system_bus;

	priv->modems = NULL;
	priv->available = FALSE;

	system_bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);
	if (!system_bus) {
		g_warning ("failed to connect to DBus system bus");
		return;
	}

	priv->ofono_manager = 
	    dbus_g_proxy_new_for_name (system_bus,
	                               "org.ofono",
	                               "/",
	                               "org.ofono.Manager");

	org_ofono_Manager_get_properties_async (priv->ofono_manager,
	                                        (org_ofono_Manager_get_properties_reply)manager_get_properties_cb,
	                                        ofono);
	dbus_g_proxy_add_signal (priv->ofono_manager,"PropertyChanged",
	                         G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->ofono_manager, "PropertyChanged",
	                             G_CALLBACK (manager_property_changed_cb),
	                             ofono, NULL);
}

GeoclueGsmlocOfono*
geoclue_gsmloc_ofono_new (void)
{
	return (g_object_new (GEOCLUE_TYPE_GSMLOC_OFONO, NULL));
}
