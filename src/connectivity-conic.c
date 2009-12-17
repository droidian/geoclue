/*
 * Geoclue
 * geoclue-conic.c
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

#ifdef HAVE_CONIC


#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <conicconnectionevent.h>
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
	
	g_debug ("conic change");
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
	DBusConnection *system_bus = NULL;
	
	self->status = GEOCLUE_CONNECTIVITY_UNKNOWN;
	
	/* Need to run dbus_connection_setup_with_g_main(),
	 * otherwise conic signals will not fire... */
	system_bus = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	if (!system_bus) {
		g_warning ("D-Bus system bus not available, connection signals not connected.");
		return;
	}
	dbus_connection_setup_with_g_main (system_bus, NULL);
	
	self->conic = con_ic_connection_new();
	if (self->conic == NULL) {
		g_warning ("Creating new ConicConnection failed");
		return;
	}
	
	g_signal_connect (G_OBJECT (self->conic), 
	                  "connection-event", 
	                  G_CALLBACK (geoclue_conic_state_changed), 
	                  self);
	
	/* this should result in a connection-event signal with current 
	 * connection status. Weird API.*/
	g_object_set (G_OBJECT (self->conic), 
	              "automatic-connection-events", 
	              TRUE, NULL);
}


static void
geoclue_conic_connectivity_init (GeoclueConnectivityInterface *iface)
{
	iface->get_status = get_status;
}

#endif /* HAVE_CONIC*/
