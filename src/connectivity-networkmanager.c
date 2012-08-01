/*
 * Geoclue
 * geoclue-networkmanager.c
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
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

#ifdef HAVE_NETWORK_MANAGER


#include <dbus/dbus-glib.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <NetworkManager.h> /*for DBus strings */

#include <nm-client.h>
#include <nm-device-wifi.h>
#include <nm-setting-ip4-config.h>

#if !defined(NM_CHECK_VERSION)
#define NM_CHECK_VERSION(x,y,z) 0
#endif

#include "connectivity-networkmanager.h"

static void geoclue_networkmanager_connectivity_init (GeoclueConnectivityInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueNetworkManager, geoclue_networkmanager, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEOCLUE_TYPE_CONNECTIVITY,
                                                geoclue_networkmanager_connectivity_init))


/* GeoclueConnectivity iface method */
static int
get_status (GeoclueConnectivity *iface)
{
	GeoclueNetworkManager *nm = GEOCLUE_NETWORKMANAGER (iface);

	return nm->status;
}

static char *
get_ap_mac (GeoclueConnectivity *iface)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (iface);

	return g_strdup (self->cache_ap_mac);
}

static int
strength_to_dbm (int strength)
{
	/* Hackish linear strength to dBm conversion.
	 * 0% is -90 dBm
	 * 100% is -20 dBm */
	return (strength * 0.7) - 90;
}

static GHashTable *
get_aps (GeoclueConnectivity *iface)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (iface);
	const GPtrArray *devices;
	GHashTable *ht;
	guint i;

	devices = nm_client_get_devices (self->client);
	if (devices == NULL)
		return NULL;

	ht = g_hash_table_new_full (g_str_hash, g_str_equal,
				    (GDestroyNotify) g_free, NULL);

	for (i = 0; i < devices->len; i++) {
		NMDevice *device = g_ptr_array_index (devices, i);
		if (NM_IS_DEVICE_WIFI (device)) {
			const GPtrArray *aps;
			guint j;

			aps = nm_device_wifi_get_access_points (NM_DEVICE_WIFI (device));
			if (aps == NULL || aps->len == 0)
				continue;
			for (j = 0; j < aps->len; j++) {
				NMAccessPoint *ap = NM_ACCESS_POINT (g_ptr_array_index (aps, j));
				char *ap_mac;
				int strength;

				ap_mac = g_strdup (nm_access_point_get_hw_address (ap));
				strength = nm_access_point_get_strength (ap);
				g_hash_table_insert (ht, ap_mac, GINT_TO_POINTER (strength_to_dbm (strength)));
			}
		}
	}
	if (g_hash_table_size (ht) == 0) {
		g_hash_table_destroy (ht);
		return NULL;
	}

	return ht;
}

static char *
mac_strup (char *mac)
{
	guint i;
	for (i = 0; mac[i] != '\0' ; i++) {
		if (g_ascii_isalpha (mac[i]))
			mac[i] = g_ascii_toupper (mac[i]);
	}
	return mac;
}

static char *
get_mac_for_gateway (const char *gateway)
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

	return mac_strup (mac);
}

static gchar *
ip4_address_as_string (guint32 ip)
{
	struct in_addr tmp_addr;
	char buf[INET_ADDRSTRLEN+1];

	memset (&buf, '\0', sizeof (buf));
	tmp_addr.s_addr = ip;

	if (inet_ntop (AF_INET, &tmp_addr, buf, INET_ADDRSTRLEN))
		return g_strdup (buf);

	return NULL;
}

static char *
get_router_mac (GeoclueConnectivity *iface)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (iface);
	const GPtrArray *devices;
	char *gateway, *mac;
	guint i;

	devices = nm_client_get_devices (self->client);
	if (devices == NULL)
		return NULL;

	gateway = NULL;

	for (i = 0; i < devices->len; i++) {
		NMDevice *device = g_ptr_array_index (devices, i);
		NMIP4Config *cfg4;
		GSList *iter;

		if (nm_device_get_state (device) != NM_DEVICE_STATE_ACTIVATED)
			continue;

		cfg4 = nm_device_get_ip4_config (device);
		if (cfg4 == NULL)
			continue;

		for (iter = (GSList *) nm_ip4_config_get_addresses (cfg4); iter; iter = g_slist_next (iter)) {
			NMIP4Address *addr = (NMIP4Address *) iter->data;

			gateway = ip4_address_as_string (nm_ip4_address_get_gateway (addr));
			if (gateway != NULL)
				break;
		}
	}
	if (gateway == NULL)
		return NULL;

	mac = get_mac_for_gateway (gateway);
	g_free (gateway);

	return mac;
}

static void
get_best_ap (GeoclueNetworkManager *self, NMDevice *device)
{
	const GPtrArray *aps;
	guint i;

	aps = nm_device_wifi_get_access_points (NM_DEVICE_WIFI (device));
	if (aps == NULL || aps->len == 0)
		return;
	for (i = 0; i < aps->len; i++) {
		NMAccessPoint *ap = NM_ACCESS_POINT (g_ptr_array_index (aps, i));
		int strength;

		strength = nm_access_point_get_strength (ap);
		if (strength > self->ap_strength) {
			g_free (self->cache_ap_mac);
			self->cache_ap_mac = g_strdup (nm_access_point_get_hw_address (ap));
			self->ap_strength = strength;
		}
	}
}

static void
cache_ap_mac (GeoclueNetworkManager *self)
{
	const GPtrArray *devices;
	guint i;

	devices = nm_client_get_devices (self->client);

	g_free (self->cache_ap_mac);
	self->cache_ap_mac = NULL;
	self->ap_strength = 0;

	for (i = 0; devices != NULL && i < devices->len; i++) {
		NMDevice *device = g_ptr_array_index (devices, i);
		if (NM_IS_DEVICE_WIFI (device)) {
			get_best_ap (self, device);
		}
	}
}

static void
dispose (GObject *object)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (object);
	
	g_free (self->cache_ap_mac);
	self->cache_ap_mac = NULL;
	g_object_unref (self->client);
	self->client = NULL;
	((GObjectClass *) geoclue_networkmanager_parent_class)->dispose (object);
}

static void
geoclue_networkmanager_class_init (GeoclueNetworkManagerClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->dispose = dispose;
}

static GeoclueNetworkStatus 
nmstate_to_geocluenetworkstatus (NMState status)
{
	switch (status) {
		case NM_STATE_UNKNOWN:
			return GEOCLUE_CONNECTIVITY_UNKNOWN;
		case NM_STATE_ASLEEP:
		case NM_STATE_DISCONNECTED:
#if NM_CHECK_VERSION(0,8,992)
		case NM_STATE_DISCONNECTING:
#endif
			return GEOCLUE_CONNECTIVITY_OFFLINE;
		case NM_STATE_CONNECTING:
			return GEOCLUE_CONNECTIVITY_ACQUIRING;
#if NM_CHECK_VERSION(0,8,992)
		case NM_STATE_CONNECTED_LOCAL:
		case NM_STATE_CONNECTED_SITE:
		case NM_STATE_CONNECTED_GLOBAL:
#else
		case NM_STATE_CONNECTED:
#endif
			return GEOCLUE_CONNECTIVITY_ONLINE;
		default:
			g_warning ("Unknown NMStatus: %d", status);
			return GEOCLUE_CONNECTIVITY_UNKNOWN;
	}
}

static void
update_status (GeoclueNetworkManager *self, gboolean do_signal)
{
	GeoclueNetworkStatus old_status;
	NMState state;

	old_status = self->status;

	if (nm_client_get_manager_running (self->client)) {
		state = nm_client_get_state (self->client);
		self->status = nmstate_to_geocluenetworkstatus (state);
		cache_ap_mac (self);
	} else {
		self->status = GEOCLUE_CONNECTIVITY_OFFLINE;
	}

	if ((self->status != old_status) && do_signal) {
		geoclue_connectivity_emit_status_changed (GEOCLUE_CONNECTIVITY (self),
		                                          self->status);
	}
}

static void
nm_update_status_cb (GObject *obj, GParamSpec *spec, gpointer userdata)
{
	update_status (GEOCLUE_NETWORKMANAGER (userdata), TRUE);
}

static void
geoclue_networkmanager_init (GeoclueNetworkManager *self)
{
	self->status = GEOCLUE_CONNECTIVITY_UNKNOWN;
	self->client = nm_client_new ();
	if (self->client == NULL) {
		g_warning ("%s was unable to create a connection to NetworkManager",
			   G_OBJECT_TYPE_NAME (self));
		return;
	}

	g_signal_connect (G_OBJECT (self->client), "notify::running",
	                  G_CALLBACK (nm_update_status_cb), self);
	g_signal_connect (G_OBJECT (self->client), "notify::state",
	                  G_CALLBACK (nm_update_status_cb), self);

	/* get initial status */
	update_status (self, FALSE);
}

static void
geoclue_networkmanager_connectivity_init (GeoclueConnectivityInterface *iface)
{
	iface->get_status = get_status;
	iface->get_ap_mac = get_ap_mac;
	iface->get_router_mac = get_router_mac;
	iface->get_aps    = get_aps;
}

#endif /* HAVE_NETWORK_MANAGER */
