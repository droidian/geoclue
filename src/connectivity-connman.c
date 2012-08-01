/*
 * Geoclue
 * connectivity-connman.c
 *
 * Author: Javier Fernandez <jfernandez@igalia.com>
 * Copyright (C) 2010 Igalia S.L
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

#ifdef HAVE_CONNMAN

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "connectivity-connman.h"

#define CONNMAN_SERVICE           "org.moblin.connman"
#define CONNMAN_MANAGER_PATH      "/"
#define CONNMAN_MANAGER_INTERFACE CONNMAN_SERVICE ".Manager"
#define CONNMAN_TECHNOLOGY_INTERFACE CONNMAN_SERVICE ".Technology"
#define CONNMAN_DEVICE_INTERFACE CONNMAN_SERVICE ".Device"
#define CONNMAN_NETWORK_INTERFACE CONNMAN_SERVICE ".Network"
#define CONNMAN_SERVICE_INTERFACE CONNMAN_SERVICE ".Service"

typedef void (*ConnmanFunc) (GeoclueConnman *self,
			     const gchar *path,
			     gpointer out);

static void geoclue_connman_connectivity_init (GeoclueConnectivityInterface *iface);

static int _strength_to_dbm (int strength);
static char *_get_mac_for_gateway (const char *gateway);
static char *_mac_strup (char *mac);
static gchar *_get_gateway (GeoclueConnman *self, const gchar *service);
static void _get_best_ap (GeoclueConnman *self, const gchar *network);
static void _get_aps_info (GeoclueConnman *self, const gchar *network, GHashTable **out);
static const GPtrArray *_get_technologies (GeoclueConnman *self, GHashTable **props);
static const GPtrArray *_get_services (GeoclueConnman *self, GHashTable **props);
static const GPtrArray *_get_devices (GeoclueConnman *self, GHashTable **props, const gchar *technology);
static const GPtrArray *_get_networks (GeoclueConnman *self, GHashTable **props, const gchar *device, const gchar *type_filter);
static void _explore_available_aps (GeoclueConnman *self, ConnmanFunc func, gpointer out);


G_DEFINE_TYPE_WITH_CODE (GeoclueConnman, geoclue_connman, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEOCLUE_TYPE_CONNECTIVITY,
                                                geoclue_connman_connectivity_init))


/* GeoclueConnectivity iface methods */
static int
_get_status (GeoclueConnectivity *iface)
{
	GeoclueConnman *connman = GEOCLUE_CONNMAN (iface);

	return connman->status;
}

static char *
_get_ap_mac (GeoclueConnectivity *iface)
{
	GeoclueConnman *connman = GEOCLUE_CONNMAN (iface);

	return g_strdup (connman->cache_ap_mac);
}

static GHashTable *
_get_aps (GeoclueConnectivity *iface)
{
	GeoclueConnman *self = GEOCLUE_CONNMAN (iface);
	GHashTable *ht = NULL;

	/* Explore the active networks to collect the APs. */
	_explore_available_aps (self, (ConnmanFunc) _get_aps_info, &ht);

	return ht;
}

static char *
_get_router_mac (GeoclueConnectivity *iface)
{
	GeoclueConnman *self = GEOCLUE_CONNMAN (iface);
	GHashTable *props = NULL;
	const GPtrArray *servs = NULL;
	const gchar *serv = NULL;
	gchar *gateway = NULL;
	char *mac = NULL;
	guint i;

	/* Get available services and iterate over them */
	/* to get the MAC of the connected router. */
	servs = _get_services (self, &props);
	if (servs != NULL) {
		for (i = 0; gateway == NULL && servs->len; i++) {
			serv = g_ptr_array_index (servs, i);
			gateway = _get_gateway (self, serv);
		}
	}

	/* Check the result. */
	if (gateway != NULL) {
		mac = _get_mac_for_gateway (gateway);
		g_free (gateway);
	}

	/* Free */
	g_hash_table_destroy (props);

	return mac;
}

/* internal private GObject methods */
static void
dispose (GObject *object)
{
	GeoclueConnman *self = GEOCLUE_CONNMAN (object);

	g_object_unref (self->client);
	self->client = NULL;

	dbus_g_connection_unref (self->conn);
	self->conn = NULL;

	g_free (self->cache_ap_mac);
	self->cache_ap_mac = NULL;

	((GObjectClass *) geoclue_connman_parent_class)->dispose (object);
}

static void
geoclue_connman_class_init (GeoclueConnmanClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->dispose = dispose;
}

static int
_strength_to_dbm (int strength)
{
	/* Hackish linear strength to dBm conversion.
	 * 0% is -90 dBm
	 * 100% is -20 dBm */
	return (strength * 0.7) - 90;
}

static char *
_get_mac_for_gateway (const char *gateway)
{
	char *cmd, *out, *mac, **split;

	cmd = g_strdup_printf ("ip neigh show %s", gateway);

	if (g_spawn_command_line_sync (cmd, &out, NULL, NULL, NULL) == FALSE) {
		g_free (out);
		g_free (cmd);
		return NULL;
	}
	g_free (cmd);

	/* 192.168.1.1 dev eth0 lladdr 00:00:00:00:00:00 STALE */
	split = g_strsplit (out, " ", -1);
	g_free (out);

	if (split == NULL)
		return NULL;
	if (g_strv_length (split) != 6) {
		g_strfreev (split);
		return NULL;
	}
	mac = g_strdup (split[4]);
	g_strfreev (split);

	return _mac_strup (mac);
}

static char *
_mac_strup (char *mac)
{
	guint i;

	g_assert (mac != NULL);

	for (i = 0; mac[i] != '\0' ; i++) {
		if (g_ascii_isalpha (mac[i]))
			mac[i] = g_ascii_toupper (mac[i]);
	}
	return mac;
}

static GeoclueNetworkStatus
connmanstatus_to_geocluenetworkstatus (const gchar *status)
{
	if (g_strcmp0 (status, "online")) {
		return GEOCLUE_CONNECTIVITY_OFFLINE;
	} else {
		return GEOCLUE_CONNECTIVITY_ONLINE;
	}
}

static gchar *
_get_gateway (GeoclueConnman *self,
	      const gchar *service)
{
	DBusGProxy *proxy = NULL;
	GHashTable *props = NULL;
	gchar *gateway = NULL;
	const GHashTable *ht = NULL;
	const GValue *value = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;

	/* Create proxy. */
	proxy = dbus_g_proxy_new_for_name (self->conn,
					   CONNMAN_SERVICE,
					   service,
					   CONNMAN_SERVICE_INTERFACE);
	if (proxy == NULL) {
		g_warning ("%s was unable to create connection to Service iface.",
			   G_OBJECT_TYPE_NAME (self));
		return NULL;
	}

	/* Get Service properties to get the gateway address. */
	if (dbus_g_proxy_call (proxy, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       &props,
			       G_TYPE_INVALID)) {

		/* Get the mac of the connected router. */
		value = g_hash_table_lookup (props, "IPv4");
		if (value != NULL) {
			ht = g_value_get_boxed (value);
			value = g_hash_table_lookup ((GHashTable *) ht, "Gateway");
			if (value != NULL) {
				gateway = g_value_dup_string (value);
			}
		}
	} else {
		msg = "Error getting Service properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	/* Free */
	g_hash_table_destroy (props);
	g_object_unref (proxy);

	return gateway;
}

static void
_get_aps_info (GeoclueConnman *self,
	       const gchar *network,
	       GHashTable **out)
{
	DBusGProxy *proxy = NULL;
	GHashTable *props = NULL;
	gchar *ap_mac = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;
	int strength;

	g_assert (out != NULL);

	/* Create proxy. */
	proxy = dbus_g_proxy_new_for_name (self->conn,
					   CONNMAN_SERVICE,
					   network,
					   CONNMAN_NETWORK_INTERFACE);
	if (proxy == NULL) {
		g_warning ("%s was unable to create connection to Network iface.",
			   G_OBJECT_TYPE_NAME (self));
		return;
	}

	/* Collect available APs into a HasTable. */
	if (dbus_g_proxy_call (proxy, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       &props,
			       G_TYPE_INVALID)) {

		/* Store the AP information. */
		ap_mac = g_value_dup_string (g_hash_table_lookup (props, "Address"));
		strength = g_value_get_uchar (g_hash_table_lookup (props, "Strength"));
		if (ap_mac != NULL) {
			/* Create storage for the first case. */
			if (*out == NULL) {
				*out = g_hash_table_new_full (g_str_hash, g_str_equal,
							      (GDestroyNotify) g_free, NULL);
			}
			g_hash_table_insert (*out, ap_mac,
					     GINT_TO_POINTER (_strength_to_dbm (strength)));
		}
	} else {
		msg = "Error getting Network properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	/* Free */
	g_hash_table_destroy (props);
	g_object_unref (proxy);
}

static void
_get_best_ap (GeoclueConnman *self,
	      const gchar *network)
{
	DBusGProxy *proxy = NULL;
	GHashTable *props = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;
	int strength;

	/* Create proxy. */
	proxy = dbus_g_proxy_new_for_name (self->conn,
					   CONNMAN_SERVICE,
					   network,
					   CONNMAN_NETWORK_INTERFACE);

	if (proxy == NULL) {
		g_warning ("%s was unable to create connection to Network iface.",
			   G_OBJECT_TYPE_NAME (self));
		return;
	}

	/* Evaluate Network properties and update best AP. */
	if (dbus_g_proxy_call (proxy, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       &props,
			       G_TYPE_INVALID)) {

		strength = g_value_get_uchar (g_hash_table_lookup (props, "Strength"));
		if (strength > self->ap_strength) {
			g_free (self->cache_ap_mac);
			self->cache_ap_mac = g_value_dup_string (g_hash_table_lookup (props, "Address"));
			self->ap_strength = strength;
		}
	} else {
		msg = "Error getting Network properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	/* Free */
	g_hash_table_destroy (props);
	g_object_unref (proxy);
}

static const GPtrArray *
_get_networks (GeoclueConnman *self,
	       GHashTable **props,
	       const gchar *device,
	       const gchar *type_filter)
{
	DBusGProxy *proxy = NULL;
	const GPtrArray *nets = NULL;
	const GValue *value = NULL;
	const gchar *type = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;

	/* Create proxy. */
	proxy = dbus_g_proxy_new_for_name (self->conn,
					   CONNMAN_SERVICE,
					   device,
					   CONNMAN_DEVICE_INTERFACE);

	if (proxy == NULL) {
		g_warning ("%s was unable to create connection to Device iface.",
			   G_OBJECT_TYPE_NAME (self));
		return NULL;
	}

	/* Get available Networks for a specific Device. */
	if (dbus_g_proxy_call (proxy, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       props,
			       G_TYPE_INVALID)) {

		/* Check only specific networks. */
		if (type_filter != NULL) {
			type = g_value_get_string (g_hash_table_lookup (*props, "Type"));
			if (g_strcmp0 (type, type_filter)) {
                            goto frees;
                        }
                }
                value = g_hash_table_lookup (*props, "Networks");
                nets = g_value_get_boxed (value);
	} else {
		msg = "Error getting Device properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	/* Free */
 frees:
	g_object_unref (proxy);

	return nets;
}

static const GPtrArray *
_get_devices (GeoclueConnman *self,
	      GHashTable **props,
	      const gchar *technology)
{
	DBusGProxy *proxy = NULL;
	const GPtrArray *devs = NULL;
	const GValue *value = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;

	/* Create proxy. */
	proxy = dbus_g_proxy_new_for_name (self->conn,
					   CONNMAN_SERVICE,
					   technology,
					   CONNMAN_TECHNOLOGY_INTERFACE);

	if (proxy == NULL) {
		g_warning ("%s was unable to create connection to Technology iface.",
			   G_OBJECT_TYPE_NAME (self));
		return NULL;
	}

	/* Get available Devices for a specific Technology. */
	if (dbus_g_proxy_call (proxy, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       props,
			       G_TYPE_INVALID)) {

		value = g_hash_table_lookup (*props, "Devices");
		devs = g_value_get_boxed (value);
	} else {
		msg = "Error getting Technologies properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	/* Free */
	g_object_unref (proxy);

	return devs;
}

static const GPtrArray *
_get_technologies (GeoclueConnman *self,
		   GHashTable **props)
{
	const GPtrArray *techs = NULL;
	const GValue *value = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;

	/* Get available technologies (Wifi, Eth, ...). */
	if (dbus_g_proxy_call (self->client, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       props,
			       G_TYPE_INVALID)) {

		value = g_hash_table_lookup (*props, "Technologies");
		techs = g_value_get_boxed (value);
	} else {
		msg = "Error getting Manager properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	return techs;
}

static const GPtrArray *
_get_services (GeoclueConnman *self,
	       GHashTable **props)
{
	const GPtrArray *servs = NULL;
	const GValue *value = NULL;
	const gchar *msg = NULL;
	GError *error = NULL;

	/* Get available services. */
	if (dbus_g_proxy_call (self->client, "GetProperties",
			       &error,
			       G_TYPE_INVALID,
			       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       props,
			       G_TYPE_INVALID)) {

		value = g_hash_table_lookup (*props, "Services");
		servs = g_value_get_boxed (value);
	} else {
		msg = "Error getting Manager properties: %s";
		if (error != NULL) {
			g_warning (msg, error->message);
			g_error_free (error);
		} else {
			g_warning (msg, "Unknown error");
		}
	}

	return servs;
}

static void
_explore_available_aps (GeoclueConnman *self,
			ConnmanFunc func,
			gpointer out)
{
	GHashTable *mgr_props = NULL;
	GHashTable *tech_props = NULL;
	GHashTable *dev_props = NULL;
	const GPtrArray *techs = NULL;
	const GPtrArray *devs = NULL;
	const GPtrArray *nets = NULL;
	const gchar *tech = NULL;
	const gchar *dev = NULL;
	const gchar *net = NULL;
	guint i, j, k;

	techs = _get_technologies (self, &mgr_props);
	if (techs  == NULL) {
		goto frees;
	}
	for (i = 0; i < techs->len; i++) {
		tech = g_ptr_array_index (techs, i);
		devs = _get_devices (self, &tech_props, tech);
		if (devs == NULL) {
			continue;
		}
		for (j = 0; j < devs->len; j++) {
			dev = g_ptr_array_index (devs, j);
			nets = _get_networks (self, &dev_props, dev, "wifi");
			if (nets == NULL) {
				continue;
			}
			for (k = 0; k < nets->len; k++) {
				net = g_ptr_array_index (nets, k);

				/* Perform specific actions. */
				func (self, net, out);
			}
		}
	}

	/* Free */
 frees:
	if (mgr_props != NULL) {
		g_hash_table_destroy (mgr_props);
	}
	if (tech_props != NULL) {
		g_hash_table_destroy (tech_props);
	}
	if (dev_props != NULL) {
		g_hash_table_destroy (dev_props);
	}
}

static void
_cache_ap_mac (GeoclueConnman *self)
{
	/* Cleanup the cache. */
	g_free (self->cache_ap_mac);
	self->cache_ap_mac = NULL;
	self->ap_strength = 0;

	/* Explore the active networks to get the best AP. */
	_explore_available_aps (self, (ConnmanFunc) _get_best_ap, NULL);
}



static void
_geoclue_connman_state_changed (DBusGProxy *proxy,
				const gchar *status,
				gpointer userdata)
{
	GeoclueConnman *self = GEOCLUE_CONNMAN (userdata);
	GeoclueNetworkStatus gc_status;

	gc_status = connmanstatus_to_geocluenetworkstatus (status);

	if (gc_status != self->status) {
		/* Update status. */
		self->status = gc_status;

		/* Update AP cache. */
		_cache_ap_mac (self);

		/* Notification. */
		geoclue_connectivity_emit_status_changed (GEOCLUE_CONNECTIVITY (self),
		                                          self->status);
	}
}


static void
_method_call_notify_cb (DBusGProxy *proxy,
                        DBusGProxyCall *call,
                        gpointer user_data)
{
	GeoclueConnman *self = GEOCLUE_CONNMAN (user_data);
	const gchar *msg = NULL;
	GError *error = NULL;
	gchar *state = NULL;

	/* Collect output data. */
	if (dbus_g_proxy_end_call (proxy,
				   call,
				   &error,
				   G_TYPE_STRING,
				   &state,
				   G_TYPE_INVALID)) {

		/* Set current status. */
		_geoclue_connman_state_changed (proxy,
						(const gchar *) state,
						self);
	} else {
		msg = "%s was unable to get the current network status: %s.";
		if (error != NULL) {
			g_warning (msg, G_OBJECT_TYPE_NAME (self),
				   error->message);
			g_error_free (error);
		} else {
			g_warning (msg, G_OBJECT_TYPE_NAME (self),
				   "Unknown error");
		}
	}

	/* Free */
	g_free (state);
}

static void
geoclue_connman_init (GeoclueConnman *self)
{
	GError *error = NULL;

	/* Get DBus connection to the System bus. */
	self->conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (self->conn == NULL) {
		g_warning ("%s was unable to create a connection to D-Bus: %s",
			   G_OBJECT_TYPE_NAME (self), error->message);
		g_error_free (error);
		return;
	}

	/* Create proxy. */
	self->client = dbus_g_proxy_new_for_name (self->conn,
						  CONNMAN_SERVICE,
						  CONNMAN_MANAGER_PATH,
						  CONNMAN_MANAGER_INTERFACE);

	if (self->client == NULL) {
		g_warning ("%s was unable to create connection to Connman Manager.",
			   G_OBJECT_TYPE_NAME (self));
		return;
	}

	/* Get current state (async). */
	dbus_g_proxy_begin_call (self->client, "GetState",
				 _method_call_notify_cb,
				 self,
				 NULL,
				 G_TYPE_INVALID,
				 G_TYPE_STRING);

	/* Be aware of State changes. */
	dbus_g_proxy_add_signal (self->client, "StateChanged",
	                         G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (self->client,
				     "StateChanged",
				     G_CALLBACK (_geoclue_connman_state_changed),
				     self,
				     NULL);
}


static void
geoclue_connman_connectivity_init (GeoclueConnectivityInterface *iface)
{
	iface->get_status = _get_status;
	iface->get_ap_mac = _get_ap_mac;
	iface->get_router_mac = _get_router_mac;
	iface->get_aps    = _get_aps;
}

#endif /* HAVE_CONNMAN*/
