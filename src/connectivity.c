/*
 * Geoclue
 * geoclue-connectivity.c
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
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

#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "connectivity.h"

#ifdef HAVE_NETWORK_MANAGER
#include "connectivity-networkmanager.h"
#else
#ifdef HAVE_CONIC
#include "connectivity-conic.h"
#else
#ifdef HAVE_CONNMAN
#include "connectivity-connman.h"
#endif
#endif
#endif

#define DEFAULT_DBM -50

enum {
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
geoclue_connectivity_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;
	
	if (initialized) {
		return;
	}
	
	initialized = TRUE;
	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
	                          G_OBJECT_CLASS_TYPE (klass),
	                          G_SIGNAL_RUN_LAST,
	                          G_STRUCT_OFFSET (GeoclueConnectivityInterface, 
	                                           status_changed),
	                          NULL, NULL,
	                          g_cclosure_marshal_VOID__INT,
	                          G_TYPE_NONE, 1, G_TYPE_INT);
}

GType
geoclue_connectivity_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		const GTypeInfo info = {
			sizeof (GeoclueConnectivityInterface),
			geoclue_connectivity_base_init,
			NULL,
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE,
		                               "GeoclueConnectivity", 
		                               &info, 0);
	}
	
	return type;
}

GeoclueConnectivity *
geoclue_connectivity_new (void)
{
	GeoclueConnectivity *connectivity = NULL;

#ifdef HAVE_NETWORK_MANAGER
	connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_NETWORKMANAGER, NULL));
#else
#ifdef HAVE_CONIC
	connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_CONIC, NULL));
#else
#ifdef HAVE_CONNMAN
	connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_CONNMAN, NULL));
#endif
#endif
#endif
	return connectivity;
}

GeoclueNetworkStatus
geoclue_connectivity_get_status (GeoclueConnectivity *self)
{
	return GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_status (self);
}

/* Parse /proc/net/route to get default gateway address and then parse
 * /proc/net/arp to find matching mac address.
 *
 * There are some problems with this. First, it's IPv4 only.
 * Second, there must be a way to do this with ioctl, but that seemed really
 * complicated... even /usr/sbin/arp parses /proc/net/arp
 *
 * returns:
 *   1 : on success
 *   0 : no success, no errors
 *  <0 : error
 */
static int
get_router_mac_fallback (char **mac)
{
	char *content;
	char **lines, **entry;
	GError *error = NULL;
	char *route_gateway = NULL;

	g_assert (*mac == NULL);

	if (!g_file_get_contents ("/proc/net/route", &content, NULL, &error)) {
		g_warning ("Failed to read /proc/net/route: %s", error->message);
		g_error_free (error);
		return -1;
	}

	lines = g_strsplit (content, "\n", 0);
	g_free (content);
	entry = lines + 1;

	while (*entry && strlen (*entry) > 0) {
		char dest[9];
		char gateway[9];
		if (sscanf (*entry,
			    "%*s %8[0-9A-Fa-f] %8[0-9A-Fa-f] %*s",
			    dest, gateway) != 2) {
			g_warning ("Failed to parse /proc/net/route entry '%s'", *entry);
		} else if (strcmp (dest, "00000000") == 0) {
			route_gateway = g_strdup (gateway);
			break;
		}
		entry++;
	}
	g_strfreev (lines);

	if (!route_gateway) {
		g_warning ("Failed to find default route in /proc/net/route");
		return -1;
	}

	if (!g_file_get_contents ("/proc/net/arp", &content, NULL, &error)) {
		g_warning ("Failed to read /proc/net/arp: %s", error->message);
		g_error_free (error);
		return -1;
	}

	lines = g_strsplit (content, "\n", 0);
	g_free (content);
	entry = lines+1;
	while (*entry && strlen (*entry) > 0) {
		char hwa[100];
		char *arp_gateway;
		int ip[4];

		if (sscanf(*entry,
			   "%d.%d.%d.%d 0x%*x 0x%*x %100s %*s %*s\n",
			   &ip[0], &ip[1], &ip[2], &ip[3], hwa) != 5) {
			g_warning ("Failed to parse /proc/net/arp entry '%s'", *entry);
		} else {
			arp_gateway = g_strdup_printf ("%02X%02X%02X%02X", ip[3], ip[2], ip[1], ip[0]);
			if (strcmp (arp_gateway, route_gateway) == 0) {
				g_free (arp_gateway);
				*mac = g_strdup (hwa);
				break;
			}
			g_free (arp_gateway);

		}
		entry++;
	}
	g_free (route_gateway);
	g_strfreev (lines);

	return *mac ? 1 : 0;
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

char *
geoclue_connectivity_get_router_mac (GeoclueConnectivity *self)
{
	if (self == NULL ||
	    GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_router_mac == NULL) {
		char *mac = NULL;
		guint i;
		int ret_val;

		for (i = 0; i < 5; i++) {
			ret_val = get_router_mac_fallback (&mac);
			if (ret_val < 0)
				return NULL;
			else if (ret_val == 1)
				break;
			g_usleep (G_USEC_PER_SEC / 10);
		}
		return mac_strup (mac);
	}

	return GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_router_mac (self);
}

char *
geoclue_connectivity_get_ap_mac (GeoclueConnectivity *self)
{
	if (self != NULL &&
	    GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_ap_mac != NULL)
		return GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_ap_mac (self);

	/* Hack when not using NetworkManager */
	return geoclue_connectivity_get_router_mac (self);
}

GHashTable *
geoclue_connectivity_get_aps (GeoclueConnectivity *self)
{
	char *ap;
	GHashTable *ht;

	if (self != NULL &&
	    GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_aps != NULL)
		return GEOCLUE_CONNECTIVITY_GET_INTERFACE (self)->get_aps (self);

	/* Fallback if the backend does not support get_aps */
	ap = geoclue_connectivity_get_ap_mac (self);
	if (ap == NULL)
		return NULL;
	ht = g_hash_table_new_full (g_str_hash, g_str_equal,
				    (GDestroyNotify) g_free, NULL);
	g_hash_table_insert (ht, ap, GINT_TO_POINTER (DEFAULT_DBM));
	return NULL;
}

void
geoclue_connectivity_emit_status_changed (GeoclueConnectivity *self,
                                          GeoclueNetworkStatus status)
{
	g_signal_emit (self, signals[STATUS_CHANGED], 0, status);
}
