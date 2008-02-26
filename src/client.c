/*
 * Geoclue
 * client.c - Geoclue Master Client
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

/** TODO
 * 	if suddenly no providers available, should emit something ? 
 * 
 * 	might want to write a testing-provider with a gui for 
 * 	choosing what to emit...
 * 
 **/


#include <config.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#include "client.h"


enum {
	POSITION_CHANGED,
	ADDRESS_CHANGED,
	LAST_SIGNAL
};
/* used to save signal ids for current provider */
static guint32 signals[LAST_SIGNAL] = {0, };

static void gc_master_client_emit_position_changed (GcMasterClient *client);
static void gc_master_client_emit_address_changed (GcMasterClient *client);
static gboolean gc_master_client_choose_position_provider (GcMasterClient *client, 
                                                           GList *providers);
static gboolean gc_master_client_choose_address_provider (GcMasterClient *client, 
                                                          GList *providers);

static gboolean gc_iface_master_client_set_requirements (GcMasterClient *client, 
                                                         GeoclueAccuracyLevel min_accuracy, 
                                                         int min_time, 
                                                         gboolean require_updates, 
                                                         GeoclueResourceFlags allowed_resources, 
                                                         GError **error);

static void gc_master_client_position_init (GcIfacePositionClass *iface);
static void gc_master_client_address_init (GcIfaceAddressClass *iface);

G_DEFINE_TYPE_WITH_CODE (GcMasterClient, gc_master_client, 
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						gc_master_client_position_init)
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
						gc_master_client_address_init))


#include "gc-iface-master-client-glue.h"

static void
finalize (GObject *object)
{
	((GObjectClass *) gc_master_client_parent_class)->finalize (object);
}

/*if changed_provider status changes, do we need to choose a new provider? */
static gboolean
status_change_requires_provider_change (GList            *provider_list,
                                        GcMasterProvider *current_provider,
                                        GcMasterProvider *changed_provider,
                                        GeoclueStatus     status)
{
	if (current_provider == changed_provider &&
	    status != GEOCLUE_STATUS_AVAILABLE) {
		return TRUE;
	}
	while (provider_list) {
		GcMasterProvider *p = provider_list->data;
		if (p == current_provider) {
			/* not interested in worse-than-current providers */
			return FALSE;
		}
		if (p == changed_provider &&
		    status == GEOCLUE_STATUS_AVAILABLE) {
			/* better-than-current provider */
			return TRUE;
		}
		provider_list = provider_list->next;
	}
	g_assert_not_reached();
}

static void
status_changed (GcMasterProvider *provider,
                GeoclueStatus     status,
                GcMasterClient   *client)
{
	g_debug ("client: provider %s status changed", gc_master_provider_get_name (provider));
	
	/*change providers if needed */
	
	if (status_change_requires_provider_change (client->position_providers,
	                                            client->position_provider,
	                                            provider, status) &&
	    gc_master_client_choose_position_provider (client, 
	                                               client->position_providers)) {
		
		/* we have a new position provider, force-emit position_changed */
		gc_master_client_emit_position_changed (client);
	}
	
	if (status_change_requires_provider_change (client->address_providers,
	                                            client->address_provider,
	                                            provider, status) &&
	    gc_master_client_choose_address_provider (client, 
	                                              client->address_providers)) {
		
		/* we have a new address provider, force-emit position_changed */
		gc_master_client_emit_address_changed (client);
	}
}

static void
accuracy_changed (GcMasterProvider     *provider,
                  GeoclueAccuracyLevel  level,
                  GcMasterClient       *client)
{
	g_debug ("client: accuracy changed, re-choosing current providers");
	client->position_providers = 
		g_list_sort (client->position_providers, 
		             (GCompareFunc)gc_master_provider_compare);
	if (gc_master_client_choose_position_provider (client, 
	                                               client->address_providers)) {
		gc_master_client_emit_position_changed (client);
	}
	
	client->address_providers = 
		g_list_sort (client->address_providers, 
		             (GCompareFunc)gc_master_provider_compare);
	if (gc_master_client_choose_address_provider (client, 
	                                              client->address_providers)) {
		gc_master_client_emit_address_changed (client);
	}
}

static void
position_changed (GcMasterProvider     *provider,
                  GeocluePositionFields fields,
                  int                   timestamp,
                  double                latitude,
                  double                longitude,
                  double                altitude,
                  GeoclueAccuracy      *accuracy,
                  GcMasterClient       *client)
{
	gc_iface_position_emit_position_changed
		(GC_IFACE_POSITION (client),
		 fields,
		 timestamp,
		 latitude, longitude, altitude,
		 accuracy);
}

static void
address_changed (GcMasterProvider     *provider,
                 int                   timestamp,
                 GHashTable           *details,
                 GeoclueAccuracy      *accuracy,
                 GcMasterClient       *client)
{
	gc_iface_address_emit_address_changed
		(GC_IFACE_ADDRESS (client),
		 timestamp,
		 details,
		 accuracy);
}

static void
gc_master_client_connect_common_signals (GcMasterClient *client, GList *providers)
{
	while (providers) {
		GcMasterProvider *provider = providers->data;
		
		g_signal_connect (G_OBJECT (provider),
				  "status-changed",
				  G_CALLBACK (status_changed),
				  client);
		g_signal_connect (G_OBJECT (provider),
				  "accuracy-changed",
				  G_CALLBACK (accuracy_changed),
				  client);
		providers = providers->next;
	}
}

static GcMasterProvider * 
gc_master_client_get_best_provider (GcMasterClient *client,
                                    GList *providers)
{
	while (providers) {
		GcMasterProvider *provider = providers->data;
		
		/* pick the most accurate provider (==first) that is available */
		if (gc_master_provider_get_status (provider) == GEOCLUE_STATUS_AVAILABLE) {
			return provider;
		}
		
		providers = providers->next;
	}
	return NULL;
}

static void
gc_master_client_emit_position_changed (GcMasterClient *client)
{
	GeocluePositionFields fields;
	int timestamp;
	double latitude, longitude, altitude;
	GeoclueAccuracy *accuracy = NULL;
	GError *error = NULL;
	
	
	if (client->position_provider == NULL) {
		return;
	}
	
	fields = gc_master_provider_get_position
		(client->position_provider,
		 &timestamp,
		 &latitude, &longitude, &altitude,
		 &accuracy,
		 &error);
	if (error) {
		/*TODO what now?*/
		g_warning ("client: failed to get position from %s: %s", 
		           gc_master_provider_get_name (client->position_provider),
		           error->message);
		g_error_free (error);
		return;
	}
	gc_iface_position_emit_position_changed
		(GC_IFACE_POSITION (client),
		 fields,
		 timestamp,
		 latitude, longitude, altitude,
		 accuracy);
}

static void 
gc_master_client_emit_address_changed (GcMasterClient *client)
{
	int timestamp;
	GHashTable *details = NULL;
	GeoclueAccuracy *accuracy = NULL;
	GError *error = NULL;
	
	
	if (client->address_provider == NULL) {
		return;
	}
	
	if (!gc_master_provider_get_address
		(client->address_provider,
		 &timestamp,
		 &details,
		 &accuracy,
		 &error)) {
		/*TODO what now?*/
		g_warning ("client: failed to get address from %s: %s", 
		           gc_master_provider_get_name (client->address_provider),
		           error->message);
		g_error_free (error);
		return;
	}
	gc_iface_address_emit_address_changed
		(GC_IFACE_ADDRESS (client),
		 timestamp,
		 details,
		 accuracy);
}

/* return true if a _new_ provider was chosen */
static gboolean
gc_master_client_choose_position_provider (GcMasterClient *client, 
                                           GList *providers)
{
	GcMasterProvider *new_p;
	
	new_p = gc_master_client_get_best_provider (client, providers);
	
	if (new_p == NULL) {
		g_debug ("client: no position providers found matching req's");
		/*TODO: cache?*/
		return FALSE;
	}else if (new_p == client->position_provider) {
		g_debug ("client: continuing with position provider %s", gc_master_provider_get_name (new_p));
		return FALSE;
	}
	
	if (client->position_provider) {
		g_signal_handler_disconnect (client->position_provider, 
		                             signals[POSITION_CHANGED]);
	}
	
	client->position_provider = new_p;
	signals[POSITION_CHANGED] =
		g_signal_connect (G_OBJECT (client->position_provider),
		                  "position-changed",
		                  G_CALLBACK (position_changed),
		                  client);
	g_debug ("client: now using position provider: %s", gc_master_provider_get_name (new_p));
	
	return TRUE;
}

/* return true if a _new_ provider was chosen */
static gboolean
gc_master_client_choose_address_provider (GcMasterClient *client, 
                                          GList *providers)
{
	GcMasterProvider *new_p;
	
	new_p = gc_master_client_get_best_provider (client, providers);
	
	if (new_p == NULL) {
		g_debug ("client: no address providers found matching req's");
		/*TODO: cache?*/
		return FALSE;
	}else if (new_p == client->address_provider) {
		g_debug ("client: continuing with address provider %s", gc_master_provider_get_name (new_p));
		return FALSE;
	}
	
	if(client->address_provider) {
		g_signal_handler_disconnect (client->address_provider, 
		                             signals[ADDRESS_CHANGED]);
	}
	
	client->address_provider = new_p;
	signals[ADDRESS_CHANGED] = 
		g_signal_connect (G_OBJECT (client->address_provider),
		                  "address-changed",
		                  G_CALLBACK (address_changed),
		                  client);
	g_debug ("client: now using address provider: %s", gc_master_provider_get_name (new_p));
	return TRUE;
}

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient        *client,
					 GeoclueAccuracyLevel   min_accuracy,
					 int                    min_time,
					 gboolean               require_updates,
					 GeoclueResourceFlags   allowed_resources,
					 GError               **error)
{
	GList *all_providers, *l;
	
	client->min_accuracy = min_accuracy;
	client->min_time = min_time;
	client->require_updates = require_updates;
	client->allowed_resources = allowed_resources;
	
	client->position_providers = 
		gc_master_get_providers (GC_IFACE_POSITION,
		                         min_accuracy,
		                         require_updates,
		                         allowed_resources,
		                         NULL);
	client->position_providers = 
		g_list_sort (client->position_providers,
		             (GCompareFunc)gc_master_provider_compare);
	g_debug ("client: %d position providers matching requirements found", 
	         g_list_length (client->position_providers));
	
	client->address_providers = 
		gc_master_get_providers (GC_IFACE_ADDRESS,
		                         min_accuracy,
		                         require_updates,
		                         allowed_resources,
		                         NULL);
	client->address_providers = 
		g_list_sort (client->address_providers,
		             (GCompareFunc)gc_master_provider_compare);
	g_debug ("client: %d address providers matching requirements found", 
	         g_list_length (client->address_providers));
	
	
	/* connect status and accuracy signals */
	all_providers = g_list_copy (client->position_providers);
	l = client->address_providers;
	while (l) {
		if (!g_list_find (all_providers, l->data)) {
			all_providers = g_list_prepend (all_providers, l->data);
		}
		l = l->next;
	}
	gc_master_client_connect_common_signals (client, all_providers);
	g_list_free (all_providers);
	
	/*TODO: do something with return values? */
	gc_master_client_choose_position_provider (client, client->position_providers);
	gc_master_client_choose_address_provider (client, client->address_providers);
	
	return TRUE;
}

static void
gc_master_client_class_init (GcMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;

	dbus_g_object_type_install_info (gc_master_client_get_type (),
					 &dbus_glib_gc_iface_master_client_object_info);
}

static void
gc_master_client_init (GcMasterClient *client)
{
}

static gboolean
get_position (GcIfacePosition       *iface,
	      GeocluePositionFields *fields,
	      int                   *timestamp,
	      double                *latitude,
	      double                *longitude,
	      double                *altitude,
	      GeoclueAccuracy      **accuracy,
	      GError               **error)
{
	GcMasterClient *client = GC_MASTER_CLIENT (iface);
	/*TODO: should maybe set sensible defaults and get providers, 
	 * if set_requirements has not been called?? */
	
	if (client->position_provider == NULL) {
		/* TODO: set error*/
		g_warning ("get_position called, but no provider available");
		return FALSE;
	}
	
	*fields = gc_master_provider_get_position
		(client->position_provider,
		 timestamp,
		 latitude, longitude, altitude,
		 accuracy,
		 error);
	return TRUE;
}

static gboolean 
get_address (GcIfaceAddress   *iface,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GcMasterClient *client = GC_MASTER_CLIENT (iface);
	
	if (client->address_provider == NULL) {
		/* TODO: set error*/
		g_warning ("get_position called, but no provider available");
		return FALSE;
	}
	
	return gc_master_provider_get_address
		(client->address_provider,
		 timestamp,
		 address,
		 accuracy,
		 error);
}

static void
gc_master_client_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}

static void
gc_master_client_address_init (GcIfaceAddressClass *iface)
{
	iface->get_address = get_address;
}
