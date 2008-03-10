/*
 * Geoclue
 * geoclue-networkmanager.c
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */
#include <config.h>

#ifdef HAVE_NETWORK_MANAGER


#include <dbus/dbus-glib.h>
#include <NetworkManager.h> /*for DBus strings */
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


static void
finalize (GObject *object)
{
	/* free everything */
	
	((GObjectClass *) geoclue_networkmanager_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (object);
	
	dbus_g_connection_unref (self->connection);
	((GObjectClass *) geoclue_networkmanager_parent_class)->dispose (object);
}

static void
geoclue_networkmanager_class_init (GeoclueNetworkManagerClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
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
			return GEOCLUE_CONNECTIVITY_OFFLINE;
		case NM_STATE_CONNECTING:
			return GEOCLUE_CONNECTIVITY_ACQUIRING;
		case NM_STATE_CONNECTED:
			return GEOCLUE_CONNECTIVITY_ONLINE;
		default:
			g_warning ("Unknown NMStatus: %d", status);
			return GEOCLUE_CONNECTIVITY_UNKNOWN;
	}
}

static void
geoclue_networkmanager_state_changed (DBusGProxy *proxy, 
                                      NMState status, 
                                      gpointer userdata)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (userdata);
	GeoclueNetworkStatus gc_status;
	
	gc_status = nmstate_to_geocluenetworkstatus (status);
	
	if (gc_status != self->status) {
		self->status = gc_status;
		geoclue_connectivity_emit_status_changed (GEOCLUE_CONNECTIVITY (self),
		                                          self->status);
	}
}


#define NM_DBUS_SIGNAL_STATE_CHANGE "StateChange"

static void
geoclue_networkmanager_init (GeoclueNetworkManager *self)
{
	GError *error = NULL;
	DBusGProxy *proxy;
	NMState state;
	
	self->status = GEOCLUE_CONNECTIVITY_UNKNOWN;
	
	self->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (self->connection == NULL) {
		g_warning ("%s was unable to create a connection to D-Bus: %s",
			   G_OBJECT_TYPE_NAME (self), error->message);
		g_error_free (error);
		return;
	}
	
	proxy = dbus_g_proxy_new_for_name (self->connection, 
	                                   NM_DBUS_SERVICE,
	                                   NM_DBUS_PATH, 
	                                   NM_DBUS_INTERFACE);
	dbus_g_proxy_add_signal (proxy, NM_DBUS_SIGNAL_STATE_CHANGE, 
	                         G_TYPE_UINT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (proxy, NM_DBUS_SIGNAL_STATE_CHANGE, 
	                             G_CALLBACK (geoclue_networkmanager_state_changed), 
	                             self, NULL);
	
	if (dbus_g_proxy_call (proxy, "state", &error, 
	                       G_TYPE_INVALID, 
	                       G_TYPE_UINT, &state, G_TYPE_INVALID)){
		self->status = nmstate_to_geocluenetworkstatus (state);
	} else {
		g_warning ("Could not get connectivity state from NetworkManager: %s", error->message);
		g_error_free (error);
	}
}


static void
geoclue_networkmanager_connectivity_init (GeoclueConnectivityInterface *iface)
{
	iface->get_status = get_status;
}

#endif /* HAVE_NETWORK_MANAGER */
