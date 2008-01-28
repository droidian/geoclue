/*
 * Geoclue
 * geoclue-networkmanager.c
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/* #include <config.h> */ 

#include <dbus/dbus-glib.h>
#include "connectivity.h"

static int NMStateToConnectivityStatus[] = {
	GEOCLUE_CONNECTIVITY_UNKNOWN,  /* NM_STATE_UNKNOWN */
	GEOCLUE_CONNECTIVITY_OFFLINE,   /*  NM_STATE_ASLEEP */
	GEOCLUE_CONNECTIVITY_ACQUIRING, /* NM_STATE_CONNECTING */
	GEOCLUE_CONNECTIVITY_ONLINE,    /* NM_STATE_CONNECTED */
	GEOCLUE_CONNECTIVITY_OFFLINE   /* NM_STATE_DISCONNECTED */
};


typedef struct {
	GObject parent;
	
	/* private */
	GeoclueConnectionStatus status;
	DBusGConnection *connection;
} GeoclueNetworkManager;

typedef struct {
	GObjectClass parent_class;
} GeoclueNetworkManagerClass;

static void geoclue_networkmanager_connectivity_init (GeoclueConnectivityInterface *iface);

#define GEOCLUE_TYPE_NETWORKMANAGER (geoclue_networkmanager_get_type ())
#define GEOCLUE_NETWORKMANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_NETWORKMANAGER, GeoclueNetworkManager))

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

static void
geoclue_networkmanager_state_changed (DBusGProxy *proxy, 
                                      int status, 
                                      gpointer userdata)
{
	GeoclueNetworkManager *self = GEOCLUE_NETWORKMANAGER (userdata);
	
	self->status = NMStateToConnectivityStatus[status];
	geoclue_connectivity_emit_status_changed (GEOCLUE_CONNECTIVITY (self),
	                                          self->status);
}

static void
geoclue_networkmanager_init (GeoclueNetworkManager *self)
{
	GError *error = NULL;
	DBusGProxy *proxy;
	int state;
	
	self->status = GEOCLUE_CONNECTIVITY_UNKNOWN;
	
	self->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (self->connection == NULL) {
		g_warning ("%s was unable to create a connection to D-Bus: %s",
			   G_OBJECT_TYPE_NAME (self), error->message);
		g_error_free (error);
		return;
	}
	
	proxy = dbus_g_proxy_new_for_name (self->connection, 
	                                   "org.freedesktop.NetworkManager",
	                                   "/org/freedesktop/NetworkManager", 
	                                   "org.freedesktop.NetworkManager");
	dbus_g_proxy_add_signal (proxy, "StateChange", 
	                         G_TYPE_UINT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (proxy, "StateChange", 
	                             G_CALLBACK (geoclue_networkmanager_state_changed), 
	                             self, NULL);
	
	if (dbus_g_proxy_call (proxy, "state", &error, 
	                       G_TYPE_INVALID, 
	                       G_TYPE_UINT, &state, G_TYPE_INVALID)){
		self->status = NMStateToConnectivityStatus[state];
	} else {
	                       	g_debug ("!");
		g_warning ("Could not get connectivity state from NetworkManager: %s", error->message);
		g_error_free (error);
	}
}


static void
geoclue_networkmanager_connectivity_init (GeoclueConnectivityInterface *iface)
{
	iface->get_status = get_status;
}
