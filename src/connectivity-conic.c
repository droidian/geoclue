/*
 * Geoclue
 * geoclue-conic.c
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <dbus/dbus-glib.h>
#include "connectivity-conic.h"

static void geoclue_conic_connectivity_init (GeoclueConnectivityInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueConic, geoclue_conic, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEOCLUE_TYPE_CONNECTIVITY,
                                                geoclue_conic_connectivity_init))


/* GeoclueConnectivity iface method */
static int
get_status (GeoclueConnectivity *iface)
{
	GeoclueConic *conic = GEOCLUE_CONIC (iface);
	
	return conic->status;
}


static void
finalize (GObject *object)
{
	/* free everything */
	
	((GObjectClass *) geoclue_conic_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GeoclueConic *self = GEOCLUE_CONIC (object);
	
	g_object_unref (self->conic);
	((GObjectClass *) geoclue_conic_parent_class)->dispose (object);
}

static void
geoclue_conic_class_init (GeoclueConicClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;
}


static GeoclueNetworkStatus 
conicstatus_to_geocluenetworkstatus (ConIcConnectionStatus status)
{
	switch (status) {
		case CON_IC_STATUS_CONNECTED:
			return GEOCLUE_CONNECTIVITY_ONLINE;
		case CON_IC_STATUS_DISCONNECTED:
		case CON_IC_STATUS_DISCONNECTING:
			return GEOCLUE_CONNECTIVITY_OFFLINE;
		default:
			g_warning ("Uknown ConIcConnectionStatus: %d", status);
			return GEOCLUE_CONNECTIVITY_UNKNOWN;
		break;
	}
}

static void
geoclue_conic_state_changed (ConIcConnection *connection,
                             ConIcConnectionEvent *event,
                             gpointer userdata)
{
	GeoclueConic *self = GEOCLUE_CONIC (userdata);
	ConIcConnectionStatus status = con_ic_connection_event_get_status (event);
	GeoclueNetworkStatus gc_status;
	
	gc_status = conicstatus_to_geocluenetworkstatus (status);
	if (gc_status != self->status) {
		self->status = gc_status;
		geoclue_connectivity_emit_status_changed (GEOCLUE_CONNECTIVITY (self),
		                                          self->status);
	}
}

static void
geoclue_conic_init (GeoclueConic *self)
{
	self->status = GEOCLUE_CONNECTIVITY_UNKNOWN;
	
	self->conic = con_ic_connection_new();
	if (self->conic == NULL) {
		warning ("Creating new ConicConnection failed");
		return;
	}
	
	g_signal_connect (G_OBJECT (self->conic), 
	                  "connection-event", 
	                  G_CALLBACK (geoclue_conic_state_changed), 
	                  self);
	g_object_set (G_OBJECT (self->conic), 
	              "automatic-connection-events", 
	              TRUE, NULL);
	
	
	/* this shouldn't actually connect, just check the connection.
	 * There really doesn't seem to be a method that returns a 
	 * ConIcConnectionStatus */
	if (con_ic_connection_connect (self->conic, 
	                               CON_IC_CONNECT_FLAG_AUTOMATICALLY_TRIGGERED) {
		self->status = GEOCLUE_CONNECTIVITY_ONLINE;
	} else {
		self->status = GEOCLUE_CONNECTIVITY_OFFLINE;
	} 
}


static void
geoclue_conic_connectivity_init (GeoclueConnectivityInterface *iface)
{
	iface->get_status = get_status;
}
