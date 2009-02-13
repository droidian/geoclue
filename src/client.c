/*
 * Geoclue
 * client.c - Geoclue Master Client
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 *                2008 OpenedHand Ltd
 */

/** TODO
 * 
 * 	might want to write a testing-provider with a gui for 
 * 	choosing what to emit...
 * 
 **/


#include <config.h>

#include <geoclue/geoclue-error.h>
#include <geoclue/geoclue-marshal.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-address.h>

#include "client.h"

#define GEOCLUE_POSITION_INTERFACE_NAME "org.freedesktop.Geoclue.Position"
#define GEOCLUE_ADDRESS_INTERFACE_NAME "org.freedesktop.Geoclue.Address"

enum {
	ADDRESS_PROVIDER_CHANGED,
	POSITION_PROVIDER_CHANGED,
	LAST_SIGNAL
};
static guint32 signals[LAST_SIGNAL] = {0, };


enum {
	POSITION_CHANGED, /* signal id of current provider */
	ADDRESS_CHANGED, /* signal id of current provider */
	LAST_PRIVATE_SIGNAL
};

typedef struct _GcMasterClientPrivate {
	guint32 signals[LAST_PRIVATE_SIGNAL];

	GeoclueAccuracyLevel min_accuracy;
	int min_time;
	gboolean require_updates;
	GeoclueResourceFlags allowed_resources;
	
	gboolean position_started;
	GcMasterProvider *position_provider;
	GList *position_providers;
	gboolean position_provider_choice_in_progress;
	
	gboolean address_started;
	GcMasterProvider *address_provider;
	GList *address_providers;
	gboolean address_provider_choice_in_progress;
	
} GcMasterClientPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GC_TYPE_MASTER_CLIENT, GcMasterClientPrivate))



static gboolean gc_iface_master_client_set_requirements (GcMasterClient       *client, 
                                                         GeoclueAccuracyLevel  min_accuracy, 
                                                         int                   min_time, 
                                                         gboolean              require_updates, 
                                                         GeoclueResourceFlags  allowed_resources, 
                                                         GError              **error);
static gboolean gc_iface_master_client_position_start (GcMasterClient *client, GError **error);
static gboolean gc_iface_master_client_address_start (GcMasterClient *client, GError **error);
static gboolean gc_iface_master_client_get_address_provider (GcMasterClient  *client,
                                                             char           **name,
                                                             char           **description,
                                                             char           **service,
                                                             char           **path,
                                                             GError         **error);
static gboolean gc_iface_master_client_get_position_provider (GcMasterClient  *client,
                                                              char           **name,
                                                              char           **description,
                                                              char           **service,
                                                              char           **path,
                                                              GError         **error);

static void gc_master_client_geoclue_init (GcIfaceGeoclueClass *iface);
static void gc_master_client_position_init (GcIfacePositionClass *iface);
static void gc_master_client_address_init (GcIfaceAddressClass *iface);

G_DEFINE_TYPE_WITH_CODE (GcMasterClient, gc_master_client, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE(GC_TYPE_IFACE_GEOCLUE,
						gc_master_client_geoclue_init)
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						gc_master_client_position_init)
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
						gc_master_client_address_init))

#include "gc-iface-master-client-glue.h"


static gboolean status_change_requires_provider_change (GList            *provider_list,
                                                        GcMasterProvider *current_provider,
                                                        GcMasterProvider *changed_provider,
                                                        GeoclueStatus     status);
static void gc_master_client_emit_position_changed (GcMasterClient *client);
static void gc_master_client_emit_address_changed (GcMasterClient *client);
static gboolean gc_master_client_choose_position_provider (GcMasterClient  *client, 
                                                           GList           *providers);
static gboolean gc_master_client_choose_address_provider (GcMasterClient  *client, 
                                                          GList           *providers);


static void
status_changed (GcMasterProvider *provider,
                GeoclueStatus     status,
                GcMasterClient   *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	g_debug ("client: provider %s status changed: %d", gc_master_provider_get_name (provider), status);
	
	/* change providers if needed (and if we're not choosing provider already) */
	
	if (!priv->position_provider_choice_in_progress &&
	    status_change_requires_provider_change (priv->position_providers,
	                                            priv->position_provider,
	                                            provider, status) &&
	    gc_master_client_choose_position_provider (client, 
	                                               priv->position_providers)) {
		
		/* we have a new position provider, force-emit position_changed */
		gc_master_client_emit_position_changed (client);
	}
	
	if (!priv->address_provider_choice_in_progress &&
	    status_change_requires_provider_change (priv->address_providers,
	                                            priv->address_provider,
	                                            provider, status) &&
	    gc_master_client_choose_address_provider (client, 
	                                              priv->address_providers)) {
		
		/* we have a new address provider, force-emit address_changed */
		gc_master_client_emit_address_changed (client);
	}
}

static void
accuracy_changed (GcMasterProvider     *provider,
                  GcInterfaceFlags      interface,
                  GeoclueAccuracyLevel  level,
                  GcMasterClient       *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GcInterfaceAccuracy *accuracy_data;
	
	accuracy_data = g_new0 (GcInterfaceAccuracy, 1);
	g_debug ("client: %s accuracy changed (%d)", 
	         gc_master_provider_get_name (provider), level);
	
	accuracy_data->interface = interface;
	accuracy_data->accuracy_level = priv->min_accuracy;
	switch (interface) {
		case GC_IFACE_POSITION:
			priv->position_providers = 
				g_list_sort_with_data (priv->position_providers, 
						       (GCompareDataFunc)gc_master_provider_compare,
						       accuracy_data);
			if (priv->position_provider_choice_in_progress) {
				g_debug ("        ...but provider choice in progress");
			} else if (gc_master_client_choose_position_provider (client, 
									      priv->position_providers)) {
				gc_master_client_emit_position_changed (client);
			}
			break;
			
		case GC_IFACE_ADDRESS:
			priv->address_providers = 
				g_list_sort_with_data (priv->address_providers, 
						       (GCompareDataFunc)gc_master_provider_compare,
						       accuracy_data);
			if (priv->address_provider_choice_in_progress) {
				g_debug ("        ...but provider choice in progress");
			} else if (gc_master_client_choose_address_provider (client, 
								      priv->address_providers)) {
				gc_master_client_emit_address_changed (client);
			}
			break;
			
		default:
			g_assert_not_reached ();
	}
	g_free (accuracy_data);
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

/*if changed_provider status changes, do we need to choose a new provider? */
static gboolean
status_change_requires_provider_change (GList            *provider_list,
                                        GcMasterProvider *current_provider,
                                        GcMasterProvider *changed_provider,
                                        GeoclueStatus     status)
{
	if (!provider_list) {
		return FALSE;
		
	} else if (current_provider == NULL) {
		return (status == GEOCLUE_STATUS_AVAILABLE);
		
	} else if (current_provider == changed_provider) {
		return (status != GEOCLUE_STATUS_AVAILABLE);
		
	}else if (status != GEOCLUE_STATUS_AVAILABLE) {
		return FALSE;
		
	}
	
	while (provider_list) {
		GcMasterProvider *p = provider_list->data;
		if (p == current_provider) {
			/* not interested in worse-than-current providers */
			return FALSE;
		}
		if (p == changed_provider) {
			/* changed_provider is better than current */
			return (status == GEOCLUE_STATUS_AVAILABLE);
		}
		provider_list = provider_list->next;
	}
	return FALSE;
}

static void
gc_master_client_connect_common_signals (GcMasterClient *client, GList *providers)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GList *l;
	
	/* connect to common signals if the provider is not already connected */
	l = providers;
	while (l) {
		GcMasterProvider *p = l->data;
		if (!g_list_find (priv->address_providers, p) &&
		    !g_list_find (priv->position_providers, p)) {
			g_debug ("client: connecting to '%s' accuracy-changed and status-changed", gc_master_provider_get_name (p));
			g_signal_connect (G_OBJECT (p),
					  "status-changed",
					  G_CALLBACK (status_changed),
					  client);
			g_signal_connect (G_OBJECT (p),
					  "accuracy-changed",
					  G_CALLBACK (accuracy_changed),
					  client);
		}
		l = l->next;
	}
}

static void
gc_master_client_unsubscribe_providers (GcMasterClient *client, GList *provider_list, GcInterfaceFlags iface)
{
	while (provider_list) {
		GcMasterProvider *provider = provider_list->data;
		
		gc_master_provider_unsubscribe (provider, client, iface);
		provider_list = provider_list->next;
	}
	
}

/* get_best_provider will return the best provider with status == GEOCLUE_STATUS_AVAILABLE.
 * It will also "subscribe" to that provider and all better ones, and unsubscribe from worse.*/
static GcMasterProvider *
gc_master_client_get_best_provider (GcMasterClient    *client,
                                    GList            **provider_list,
                                    GcInterfaceFlags   iface)
{
	GList *l = *provider_list;
	/* TODO: should maybe choose a acquiring provider if better ones are are not available */
	
	g_debug ("client: choosing best provider");
	
	while (l) {
		GcMasterProvider *provider = l->data;
		
		g_debug ("        ...trying provider %s", gc_master_provider_get_name (provider));
		if (gc_master_provider_subscribe (provider, client, iface)) {
			/* provider was started, so accuracy may have changed 
			   (which re-sorts provider lists), restart provider selection */
			/* TODO re-think this: restarting provider selection leads to potentially
			   never-ending looping */
			g_debug ("        ...started %s (status %d), re-starting provider selection",
			         gc_master_provider_get_name (provider),
			         gc_master_provider_get_status (provider));
			l = *provider_list;
			continue;
		}
		/* provider did not need to be started */
		
		/* TODO: currently returning even providers that are worse than priv->min_accuracy,
		 * if nothing else is available */
		if (gc_master_provider_get_status (provider) == GEOCLUE_STATUS_AVAILABLE) {
			/* unsubscribe from all providers worse than this */
			gc_master_client_unsubscribe_providers (client, l->next, iface);
			return provider;
		}
		l = l->next;
	}
	
	/* no provider found */
	gc_master_client_unsubscribe_providers (client, *provider_list, iface);
	return NULL;
}

static void
gc_master_client_emit_position_changed (GcMasterClient *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GeocluePositionFields fields;
	int timestamp;
	double latitude, longitude, altitude;
	GeoclueAccuracy *accuracy = NULL;
	GError *error = NULL;
	
	
	if (priv->position_provider == NULL) {
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0.0, 0.0);
		gc_iface_position_emit_position_changed
			(GC_IFACE_POSITION (client),
			 GEOCLUE_POSITION_FIELDS_NONE,
			 time (NULL),
			 0.0, 0.0, 0.0,
			 accuracy);
		geoclue_accuracy_free (accuracy);
		return;
	}
	
	fields = gc_master_provider_get_position
		(priv->position_provider,
		 &timestamp,
		 &latitude, &longitude, &altitude,
		 &accuracy,
		 &error);
	if (error) {
		/*TODO what now?*/
		g_warning ("client: failed to get position from %s: %s", 
		           gc_master_provider_get_name (priv->position_provider),
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
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	int timestamp;
	GHashTable *details = NULL;
	GeoclueAccuracy *accuracy = NULL;
	GError *error = NULL;
	
	if (priv->address_provider == NULL) {
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0.0, 0.0);
		details = g_hash_table_new (g_str_hash, g_str_equal);
		gc_iface_address_emit_address_changed
			(GC_IFACE_ADDRESS (client),
			 time (NULL),
			 details,
			 accuracy);
		g_hash_table_destroy (details);
		geoclue_accuracy_free (accuracy);
		return;
	}
	if (!gc_master_provider_get_address
		(priv->address_provider,
		 &timestamp,
		 &details,
		 &accuracy,
		 &error)) {
		/*TODO what now?*/
		g_warning ("client: failed to get address from %s: %s", 
		           gc_master_provider_get_name (priv->address_provider),
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
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GcMasterProvider *new_p;
	
	/* choose and start provider */
	priv->position_provider_choice_in_progress = TRUE;
	new_p = gc_master_client_get_best_provider (client, 
	                                            &priv->position_providers, 
	                                            GC_IFACE_POSITION);
	priv->position_provider_choice_in_progress = FALSE;
	
	if (priv->position_provider && new_p == priv->position_provider) {
		return FALSE;
	}
	
	if (priv->signals[POSITION_CHANGED] > 0) {
		g_signal_handler_disconnect (priv->position_provider, 
		                             priv->signals[POSITION_CHANGED]);
		priv->signals[POSITION_CHANGED] = 0;
	}
	
	priv->position_provider = new_p;
	
	if (priv->position_provider == NULL) {
		g_debug ("client: position provider changed (to NULL)");
		g_signal_emit (client, signals[POSITION_PROVIDER_CHANGED], 0, 
		               NULL, NULL, NULL, NULL);
		return TRUE;
	}
	
	g_debug ("client: position provider changed (to %s)", gc_master_provider_get_name (priv->position_provider));
	g_signal_emit (client, signals[POSITION_PROVIDER_CHANGED], 0, 
		       gc_master_provider_get_name (priv->position_provider),
		       gc_master_provider_get_description (priv->position_provider),
		       gc_master_provider_get_service (priv->position_provider),
		       gc_master_provider_get_path (priv->position_provider));
	priv->signals[POSITION_CHANGED] =
		g_signal_connect (G_OBJECT (priv->position_provider),
				  "position-changed",
				  G_CALLBACK (position_changed),
				  client);
	return TRUE;
}

/* return true if a _new_ provider was chosen */
static gboolean
gc_master_client_choose_address_provider (GcMasterClient *client, 
                                          GList *providers)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GcMasterProvider *new_p;
	
	
	/* choose and start provider */
	priv->address_provider_choice_in_progress = TRUE;
	new_p = gc_master_client_get_best_provider (client, 
	                                            &priv->address_providers, 
	                                            GC_IFACE_ADDRESS);
	priv->address_provider_choice_in_progress = FALSE;
	
	if (priv->address_provider != NULL && new_p == priv->address_provider) {
		/* keep using the same provider */
		return FALSE;
	}
	
	if (priv->address_provider && priv->signals[ADDRESS_CHANGED] > 0) {
		g_signal_handler_disconnect (priv->address_provider, 
					     priv->signals[ADDRESS_CHANGED]);
		priv->signals[ADDRESS_CHANGED] = 0;
	}
	
	priv->address_provider = new_p;
	
	if (priv->address_provider == NULL) {
		g_debug ("client: address provider changed (to NULL)");
		g_signal_emit (client, signals[ADDRESS_PROVIDER_CHANGED], 0, 
		               NULL, NULL, NULL, NULL);
		return TRUE;
	}
	
	g_debug ("client: address provider changed (to %s)", gc_master_provider_get_name (priv->address_provider));
	g_signal_emit (client, signals[ADDRESS_PROVIDER_CHANGED], 0, 
		       gc_master_provider_get_name (priv->address_provider),
		       gc_master_provider_get_description (priv->address_provider),
		       gc_master_provider_get_service (priv->address_provider),
		       gc_master_provider_get_path (priv->address_provider));
	priv->signals[ADDRESS_CHANGED] = 
		g_signal_connect (G_OBJECT (priv->address_provider),
				  "address-changed",
				  G_CALLBACK (address_changed),
				  client);
	return TRUE;
}

static void
gc_master_provider_set_position_providers (GcMasterClient *client, 
                                           GList *providers)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GcInterfaceAccuracy *accuracy_data;
	
	accuracy_data = g_new0(GcInterfaceAccuracy, 1);
	accuracy_data->interface = GC_IFACE_POSITION;
	accuracy_data->accuracy_level = priv->min_accuracy;
	
	gc_master_client_connect_common_signals (client, providers);
	priv->position_providers = 
		g_list_sort_with_data (providers,
		                       (GCompareDataFunc)gc_master_provider_compare,
		                       accuracy_data);
	
	g_free (accuracy_data);
}

static void
gc_master_provider_set_address_providers (GcMasterClient *client, 
                                           GList *providers)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GcInterfaceAccuracy *accuracy_data;
	
	accuracy_data = g_new0(GcInterfaceAccuracy, 1);
	accuracy_data->interface = GC_IFACE_ADDRESS;
	accuracy_data->accuracy_level = priv->min_accuracy;
	
	gc_master_client_connect_common_signals (client, providers);
	priv->address_providers = 
		g_list_sort_with_data (providers,
		                       (GCompareDataFunc)gc_master_provider_compare,
		                       accuracy_data);
	
	g_free (accuracy_data);
}

static void
gc_master_client_init_position_providers (GcMasterClient *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GList *providers;
	
	if (!priv->position_started) {
		return;
	}
	 
	/* TODO: free priv->position_providers */
	
	providers = gc_master_get_providers (GC_IFACE_POSITION,
	                                     priv->min_accuracy,
	                                     priv->require_updates,
	                                     priv->allowed_resources,
	                                     NULL);
	g_debug ("client: %d position providers matching requirements found, now choosing current provider", 
	         g_list_length (providers));
	
	gc_master_provider_set_position_providers (client, providers);
	gc_master_client_choose_position_provider (client, priv->position_providers);
}
static void
gc_master_client_init_address_providers (GcMasterClient *client)
{
	GList *providers;
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (!priv->address_started) {
		return;
	}
	 
	/* TODO: free priv->address_providers */
	
	providers = gc_master_get_providers (GC_IFACE_ADDRESS,
	                                     priv->min_accuracy,
	                                     priv->require_updates,
	                                     priv->allowed_resources,
	                                     NULL);
	g_debug ("client: %d address providers matching requirements found, now choosing current provider", 
	         g_list_length (providers));
	
	gc_master_provider_set_address_providers (client, providers);
	gc_master_client_choose_address_provider (client, priv->address_providers);
}

static gboolean
gc_iface_master_client_set_requirements (GcMasterClient        *client,
					 GeoclueAccuracyLevel   min_accuracy,
					 int                    min_time,
					 gboolean               require_updates,
					 GeoclueResourceFlags   allowed_resources,
					 GError               **error)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	priv->min_accuracy = min_accuracy;
	priv->min_time = min_time;
	priv->require_updates = require_updates;
	priv->allowed_resources = allowed_resources;
	
	gc_master_client_init_position_providers (client);
	gc_master_client_init_address_providers (client);
	
	return TRUE;
}


static gboolean 
gc_iface_master_client_position_start (GcMasterClient *client, 
                                       GError         **error)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (priv->position_providers) {
		if (error) {
			*error = g_error_new (GEOCLUE_ERROR,
			                      GEOCLUE_ERROR_FAILED,
			                      "Position interface already started");
		}
		return FALSE;
	}
	
	priv->position_started = TRUE;
	
	gc_master_client_init_position_providers (client); 
	
	return TRUE;
}

static gboolean 
gc_iface_master_client_address_start (GcMasterClient *client,
                                      GError         **error)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (priv->address_providers) {
		if (error) {
			*error = g_error_new (GEOCLUE_ERROR,
					      GEOCLUE_ERROR_FAILED,
					      "Address interface already started");
		}
		return FALSE;
	}
	
	priv->address_started = TRUE;
	gc_master_client_init_address_providers (client);
	return TRUE;
}

static void
get_master_provider_details (GcMasterProvider  *provider,
                             char             **name,
                             char             **description,
                             char             **service,
                             char             **path)
{
	if (name) {
		if (!provider) {
			*name = NULL;
		} else {
			*name = g_strdup (gc_master_provider_get_name (provider));
		}
	}
	if (description) {
		if (!provider) {
			*description = NULL;
		} else {
			*description = g_strdup (gc_master_provider_get_description (provider));
		}
	}
	if (service) {
		if (!provider) {
			*service = NULL;
		} else {
			*service = g_strdup (gc_master_provider_get_service (provider));
		}
	}
	if (path) {
		if (!provider) {
			*path = NULL;
		} else {
			*path = g_strdup (gc_master_provider_get_path (provider));
		}
	}
}
                             

static gboolean 
gc_iface_master_client_get_address_provider (GcMasterClient  *client,
                                             char           **name,
                                             char           **description,
                                             char           **service,
                                             char           **path,
                                             GError         **error)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	get_master_provider_details (priv->address_provider,
	                             name, description, service, path);
	return TRUE;
}

static gboolean 
gc_iface_master_client_get_position_provider (GcMasterClient  *client,
                                              char           **name,
                                              char           **description,
                                              char           **service,
                                              char           **path,
                                              GError         **error)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	get_master_provider_details (priv->position_provider,
	                             name, description, service, path);
	return TRUE;
}

static void
finalize (GObject *object)
{
	GcMasterClient *client = GC_MASTER_CLIENT (object);
	GcMasterClientPrivate *priv = GET_PRIVATE (object);
	
	/* do not free contents of the lists, Master takes care of them */
	if (priv->position_providers) {
		gc_master_client_unsubscribe_providers (client, priv->position_providers, GC_IFACE_ALL);
		g_list_free (priv->position_providers);
		priv->position_providers = NULL;
	}
	if (priv->address_providers) {
		gc_master_client_unsubscribe_providers (client, priv->address_providers, GC_IFACE_ALL);
		g_list_free (priv->address_providers);
		priv->address_providers = NULL;
	}
	
	((GObjectClass *) gc_master_client_parent_class)->finalize (object);
}

static void
gc_master_client_class_init (GcMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	
	g_type_class_add_private (klass, sizeof (GcMasterClientPrivate));
	
	signals[ADDRESS_PROVIDER_CHANGED] = 
		g_signal_new ("address-provider-changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL,
		              geoclue_marshal_VOID__STRING_STRING_STRING_STRING,
		              G_TYPE_NONE, 4,
		              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[POSITION_PROVIDER_CHANGED] = 
		g_signal_new ("position-provider-changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL,
		              geoclue_marshal_VOID__STRING_STRING_STRING_STRING,
		              G_TYPE_NONE, 4,
		              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	
	dbus_g_object_type_install_info (gc_master_client_get_type (),
					 &dbus_glib_gc_iface_master_client_object_info);
	

}

static void
gc_master_client_init (GcMasterClient *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	priv->position_provider_choice_in_progress = FALSE;
	priv->address_provider_choice_in_progress = FALSE;
	
	priv->position_started = FALSE;
	priv->position_provider = NULL;
	priv->position_providers = NULL;
	
	priv->address_started = FALSE;
	priv->address_provider = NULL;
	priv->address_providers = NULL;
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
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (priv->position_provider == NULL) {
		if (error) {
			*error = g_error_new (GEOCLUE_ERROR,
			                      GEOCLUE_ERROR_NOT_AVAILABLE,
			                      "Geoclue master client has no usable Position providers");
		}
		return FALSE;
	}
	
	*fields = gc_master_provider_get_position
		(priv->position_provider,
		 timestamp,
		 latitude, longitude, altitude,
		 accuracy,
		 error);
	return (!*error);
}

static gboolean 
get_address (GcIfaceAddress   *iface,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GcMasterClient *client = GC_MASTER_CLIENT (iface);
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (priv->address_provider == NULL) {
		if (error) {
			*error = g_error_new (GEOCLUE_ERROR,
			                      GEOCLUE_ERROR_NOT_AVAILABLE,
			                      "Geoclue master client has no usable Address providers");
		}
		return FALSE;
	}
	
	return gc_master_provider_get_address
		(priv->address_provider,
		 timestamp,
		 address,
		 accuracy,
		 error);
}

static gboolean
get_status (GcIfaceGeoclue *geoclue,
            GeoclueStatus  *status,
            GError        **error)
{
	/* not really meaningful */
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static gboolean
set_options (GcIfaceGeoclue *geoclue,
             GHashTable     *options,
             GError        **error)
{
	/* not meaningful, options come from master */
	
	/* It is not an error to not have a SetOptions implementation */
	return TRUE;
}

static gboolean 
get_provider_info (GcIfaceGeoclue  *geoclue,
		   gchar          **name,
		   gchar          **description,
		   GError         **error)
{
	if (name) {
		*name = g_strdup ("Geoclue Master");
	}
	if (description) {
		*description = g_strdup ("Meta-provider that internally uses what ever provider is the best ");
	}
	return TRUE;
}

static void
add_reference (GcIfaceGeoclue *geoclue,
               DBusGMethodInvocation *context)
{
	/* TODO implement if needed */
	dbus_g_method_return (context);
}

static void
remove_reference (GcIfaceGeoclue *geoclue,
                  DBusGMethodInvocation *context)
{
	/* TODO implement if needed */
	dbus_g_method_return (context);
}

static void
gc_master_client_geoclue_init (GcIfaceGeoclueClass *iface)
{
	iface->get_provider_info = get_provider_info;
	iface->get_status = get_status;
	iface->set_options = set_options;
	iface->add_reference = add_reference;
	iface->remove_reference = remove_reference;
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
