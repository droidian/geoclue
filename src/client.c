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

typedef struct _GcMasterClientPrivate {
	gboolean provider_choice_in_progress;
	
	GeoclueAccuracyLevel min_accuracy;
	int min_time;
	gboolean require_updates;
	GeoclueResourceFlags allowed_resources;
	
	GcMasterProvider *position_provider;
	GList *position_providers;
	
	GcMasterProvider *address_provider;
	GList *address_providers;
	
} GcMasterClientPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GC_TYPE_MASTER_CLIENT, GcMasterClientPrivate))

enum {
	PROVIDER_CHANGED,
	POSITION_CHANGED, /* signal id of current provider */
	ADDRESS_CHANGED, /* signal id of current provider */
	LAST_SIGNAL
};
static guint32 signals[LAST_SIGNAL] = {0, };


static gboolean gc_iface_master_client_set_requirements (GcMasterClient       *client, 
                                                         GeoclueAccuracyLevel  min_accuracy, 
                                                         int                   min_time, 
                                                         gboolean              require_updates, 
                                                         GeoclueResourceFlags  allowed_resources, 
                                                         GError              **error);
static gboolean gc_iface_master_client_position_start (GcMasterClient *client, GError **error);
static gboolean gc_iface_master_client_address_start (GcMasterClient *client, GError **error);
static gboolean gc_iface_master_client_get_provider (GcMasterClient  *client,
                                                     char            *interface,
                                                     char           **name,
                                                     char           **description,
                                                     GError         **error);

static void gc_master_client_position_init (GcIfacePositionClass *iface);
static void gc_master_client_address_init (GcIfaceAddressClass *iface);

G_DEFINE_TYPE_WITH_CODE (GcMasterClient, gc_master_client, 
			 G_TYPE_OBJECT,
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
	
	if (priv->provider_choice_in_progress) {
		g_debug ("client: %s status changed (while choosing provider)", 
		         gc_master_provider_get_name (provider));
		return;
	}
	
	g_debug ("client: provider %s status changed", gc_master_provider_get_name (provider));
	
	/*change providers if needed */
	
	if (status_change_requires_provider_change (priv->position_providers,
	                                            priv->position_provider,
	                                            provider, status) &&
	    gc_master_client_choose_position_provider (client, 
	                                               priv->position_providers)) {
		
		/* we have a new position provider, force-emit position_changed */
		gc_master_client_emit_position_changed (client);
	}
	
	if (status_change_requires_provider_change (priv->address_providers,
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
		if (priv->provider_choice_in_progress) {
			g_debug ("        ...but provider choice in progress", 
				 gc_master_provider_get_name (provider));
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
		if (priv->provider_choice_in_progress) {
			g_debug ("        ...but provider choice in progress", 
				 gc_master_provider_get_name (provider));
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
	}
	if (current_provider == NULL) {
		return (status == GEOCLUE_STATUS_AVAILABLE);
	}
	if (current_provider == changed_provider) {
		return (status != GEOCLUE_STATUS_AVAILABLE);
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

static GcMasterProvider * 
gc_master_client_get_best_position_provider (GcMasterClient *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GList *l = priv->position_providers;
	/* TODO: should choose a acquiring provider if better ones are are not available */
	
	while (l) {
		GcMasterProvider *provider = l->data;
		GcMasterStatus status;
		
		status = gc_master_provider_get_status (provider);
		g_debug ("client: trying position provider %s (status %d)", gc_master_provider_get_name (provider), status);
		
		if (priv->position_provider && provider == priv->position_provider) {
			return provider;
		}
		
		if (status == GC_MASTER_STATUS_NOT_STARTED) {
			if (!gc_master_provider_activate (provider, client, NULL)) {
				g_warning ("starting provider failed");
			} else {
				g_debug ("client: started %s (status %d)",
				         gc_master_provider_get_name (provider),
				         gc_master_provider_get_status (provider));
				/* accuracy may have changed, which means 
				   the provider list may have changed */
				l = priv->position_providers;
				continue;
			}
		}
		
		if (gc_master_provider_get_status (provider) == GC_MASTER_STATUS_AVAILABLE) {
			return provider;
		}
		
		l = l->next;
	}
	return NULL;
}

static GcMasterProvider * 
gc_master_client_get_best_address_provider (GcMasterClient *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GList *l = priv->address_providers;
	/* TODO: should choose a acquiring provider if better ones are are not available */
	
	while (l) {
		GcMasterProvider *provider = l->data;
		GcMasterStatus status;
		
		status = gc_master_provider_get_status (provider);
		g_debug ("client: trying address provider %s (status %d)", gc_master_provider_get_name (provider), status);
		
		if (priv->address_provider && provider == priv->address_provider) {
			return provider;
		}
		
		if (status == GC_MASTER_STATUS_NOT_STARTED) {
			if (!gc_master_provider_activate (provider, client, NULL)) {
				g_warning ("starting provider failed");
			} else {
				g_debug ("client: started %s (status %d)",
				         gc_master_provider_get_name (provider),
				         gc_master_provider_get_status (provider));
				/* accuracy may have changed, which means 
				   the provider list may have changed */
				l = priv->address_providers;
				continue;
			}
		}
		
		if (gc_master_provider_get_status (provider) == GC_MASTER_STATUS_AVAILABLE) {
			return provider;
		}
		
		l = l->next;
	}
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
	new_p = gc_master_client_get_best_position_provider (client);
	
	if (priv->position_provider && new_p == priv->position_provider) {
		return FALSE;
	}
	
	if (priv->position_provider) {
		if (signals[POSITION_CHANGED] > 0) {
			g_signal_handler_disconnect (priv->position_provider, signals[POSITION_CHANGED]);
			signals[POSITION_CHANGED] = 0;
		}
		gc_master_provider_deactivate (priv->position_provider, client);
	}
	
	priv->position_provider = new_p;
	
	if (priv->position_provider == NULL) {
		g_signal_emit (client, signals[PROVIDER_CHANGED], 0, 
		               GEOCLUE_POSITION_INTERFACE_NAME, 
		               NULL, NULL);
		/* empty cache ? */
		/* TODO should probably emit address chhanged ? */
		return FALSE;
	}
	
	g_signal_emit (client, signals[PROVIDER_CHANGED], 0, 
		       GEOCLUE_POSITION_INTERFACE_NAME, 
		       gc_master_provider_get_name (priv->position_provider),
		       gc_master_provider_get_description (priv->position_provider));
	signals[POSITION_CHANGED] =
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
	priv->provider_choice_in_progress = TRUE;
	new_p = gc_master_client_get_best_address_provider (client);
	priv->provider_choice_in_progress = FALSE;
	
	if (priv->address_provider && new_p == priv->address_provider) {
		/* keep using the same provider */
		return FALSE;
	}
	
	if(priv->address_provider) {
		if (signals[ADDRESS_CHANGED] > 0) {
			g_signal_handler_disconnect (priv->address_provider, 
			                             signals[ADDRESS_CHANGED]);
			signals[ADDRESS_CHANGED] = 0;
		}
		g_debug ("client: deactivating %s", gc_master_provider_get_name (priv->address_provider));
		gc_master_provider_deactivate (priv->address_provider, client);
	}
	
	priv->address_provider = new_p;
	
	if (priv->address_provider == NULL) {
		g_debug ("client: provider changed (to NULL)");
		g_signal_emit (client, signals[PROVIDER_CHANGED], 0, 
		               GEOCLUE_ADDRESS_INTERFACE_NAME, 
		               NULL, NULL);
		/* empty cache ? */
		/* TODO should probably emit address chhanged ? */
		return FALSE;
	}
	
	g_debug ("client: provider changed (to %s)", gc_master_provider_get_name (priv->address_provider));
	g_signal_emit (client, signals[PROVIDER_CHANGED], 0, 
		       GEOCLUE_ADDRESS_INTERFACE_NAME, 
		       gc_master_provider_get_name (priv->address_provider),
		       gc_master_provider_get_description (priv->address_provider));
	signals[ADDRESS_CHANGED] = 
		g_signal_connect (G_OBJECT (priv->address_provider),
				  "address-changed",
				  G_CALLBACK (address_changed),
				  client);
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
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	priv->min_accuracy = min_accuracy;
	priv->min_time = min_time;
	priv->require_updates = require_updates;
	priv->allowed_resources = allowed_resources;
	
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


static gboolean 
gc_iface_master_client_position_start (GcMasterClient *client, 
                                       GError         **error)
{
	GList *providers;
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (priv->position_providers) {
		/* TODO set error */
		return FALSE;
	}
	
	providers = gc_master_get_providers (GC_IFACE_POSITION,
	                                     priv->min_accuracy,
	                                     priv->require_updates,
	                                     priv->allowed_resources,
	                                     NULL);
	g_debug ("client: %d position providers matching requirements found, now choosing current provider", 
	         g_list_length (providers));
	
	gc_master_provider_set_position_providers (client, providers);
	gc_master_client_choose_position_provider (client, priv->position_providers);
	
	return TRUE;
}

static gboolean 
gc_iface_master_client_address_start (GcMasterClient *client,
                                      GError         **error)
{
	GList *providers;
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	if (priv->address_providers) {
		/* TODO set error */
		return FALSE;
	}
	
	providers = gc_master_get_providers (GC_IFACE_ADDRESS,
	                                     priv->min_accuracy,
	                                     priv->require_updates,
	                                     priv->allowed_resources,
	                                     NULL);
	g_debug ("client: %d address providers matching requirements found, now choosing current provider", 
	         g_list_length (providers));
	
	gc_master_provider_set_address_providers (client, providers);
	gc_master_client_choose_address_provider (client, priv->address_providers);
	
	return TRUE;
}

static gboolean 
gc_iface_master_client_get_provider (GcMasterClient  *client,
                                     char            *iface,
                                     char           **name,
                                     char           **description,
                                     GError         **error)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	GcMasterProvider *provider;
	
	if (g_ascii_strcasecmp (iface, GEOCLUE_ADDRESS_INTERFACE_NAME) == 0) {
		provider = priv->address_provider;
	} else if (g_ascii_strcasecmp (iface, GEOCLUE_POSITION_INTERFACE_NAME) == 0) {
		provider = priv->position_provider;
	} else {
		g_warning ("unknown interface in get_provider");
		return FALSE;
	}
	
	if (name) {
		if (!provider) {
			name = NULL;
		} else {
			*name = g_strdup (gc_master_provider_get_name (provider));
		}
	}
	if (description) {
		if (!provider) {
			description = NULL;
		} else {
			*description = g_strdup (gc_master_provider_get_description (provider));
		}
	}
	return TRUE;
}

static void
finalize (GObject *object)
{
	GcMasterClient *client = GC_MASTER_CLIENT (object);
	GcMasterClientPrivate *priv = GET_PRIVATE (object);
	
	/* do not free contents of the lists, Master takes care of them */
	if (priv->position_providers) {
		g_list_free (priv->position_providers);
		priv->position_providers = NULL;
	}
	if (priv->address_providers) {
		g_list_free (priv->address_providers);
		priv->address_providers = NULL;
	}
	
	
	if (priv->position_provider) {
		g_signal_handler_disconnect (priv->position_provider, signals[POSITION_CHANGED]);
		gc_master_provider_deactivate (priv->position_provider, client);
		
		priv->position_provider = NULL;
		signals[POSITION_CHANGED] = 0;
	}
	if (priv->address_provider) {
		g_signal_handler_disconnect (priv->address_provider, signals[POSITION_CHANGED]);
		gc_master_provider_deactivate (priv->address_provider, client);
		
		priv->address_provider = NULL;
		signals[ADDRESS_CHANGED] = 0;
	}
	
	((GObjectClass *) gc_master_client_parent_class)->finalize (object);
}

static void
gc_master_client_class_init (GcMasterClientClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	
	g_type_class_add_private (klass, sizeof (GcMasterClientPrivate));
	
	signals[PROVIDER_CHANGED] = g_signal_new ("provider-changed",
	                                          G_OBJECT_CLASS_TYPE (klass),
	                                          G_SIGNAL_RUN_LAST, 0,
	                                          NULL, NULL,
	                                          geoclue_marshal_VOID__STRING_STRING_STRING,
	                                          G_TYPE_NONE, 3,
	                                          G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	
	dbus_g_object_type_install_info (gc_master_client_get_type (),
					 &dbus_glib_gc_iface_master_client_object_info);
	

}

static void
gc_master_client_init (GcMasterClient *client)
{
	GcMasterClientPrivate *priv = GET_PRIVATE (client);
	
	priv->provider_choice_in_progress = FALSE;
	
	priv->position_provider = NULL;
	priv->position_providers = NULL;
	
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
