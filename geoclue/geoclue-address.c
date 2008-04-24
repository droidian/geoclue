/*
 * Geoclue
 * geoclue-address.c - Client API for accessing GcIfaceAddress
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-address
 * @short_description: Geoclue address client API
 *
 * #GeoclueAddress contains Address-related methods and signals. 
 * It is part of the Geoclue public C client API which uses D-Bus 
 * to communicate with the actual provider.
 * 
 * After a #GeoclueAddress is created with geoclue_address_new() or 
 * geoclue_master_client_create_address(), the 
 * geoclue_address_get_address() and geoclue_address_get_address_async() methods 
 * and the address-changed signal can be used to obtain the current address. 
 * 
 * Address #GHashTable keys are defined in 
 * <ulink url="geoclue-types.html">geoclue-types.h</ulink>. See also 
 * convenience functions in 
 * <ulink url="geoclue-address-details.html">geoclue-address-details.h</ulink>.
 */

#include <geoclue/geoclue-address.h>
#include <geoclue/geoclue-marshal.h>

#include "gc-iface-address-bindings.h"

typedef struct _GeoclueAddressPrivate {
	int dummy;
} GeoclueAddressPrivate;

enum {
	ADDRESS_CHANGED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_ADDRESS, GeoclueAddressPrivate))

G_DEFINE_TYPE (GeoclueAddress, geoclue_address, GEOCLUE_TYPE_PROVIDER);

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (geoclue_address_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	G_OBJECT_CLASS (geoclue_address_parent_class)->dispose (object);
}

static void
address_changed (DBusGProxy      *proxy,
		 int              timestamp,
		 GHashTable      *details,
		 GeoclueAccuracy *accuracy,
		 GeoclueAddress *address)
{
	g_signal_emit (address, signals[ADDRESS_CHANGED], 0, 
		       timestamp, details, accuracy);
}

static GObject *
constructor (GType                  type,
	     guint                  n_props,
	     GObjectConstructParam *props)
{
	GObject *object;
	GeoclueProvider *provider;

	object = G_OBJECT_CLASS (geoclue_address_parent_class)->constructor
		(type, n_props, props);
	provider = GEOCLUE_PROVIDER (object);
	
	dbus_g_proxy_add_signal (provider->proxy, "AddressChanged",
				 G_TYPE_INT, 
				 DBUS_TYPE_G_STRING_STRING_HASHTABLE,
				 GEOCLUE_ACCURACY_TYPE,
				 G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (provider->proxy, "AddressChanged",
				     G_CALLBACK (address_changed),
				     object, NULL);

	return object;
}

static void
geoclue_address_class_init (GeoclueAddressClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;

	g_type_class_add_private (klass, sizeof (GeoclueAddressPrivate));
	
	/**
	 * GeoclueAddress::address-changed:
	 * @address: the #GeoclueAddress object emitting the signal
	 * @timestamp: Time of address measurement (Unix timestamp)
	 * @details: Address details as #GHashTable.
	 * @accuracy: Accuracy of measurement as #GeoclueAccuracy
	 * 
	 * The address-changed signal is emitted each time the address changes. 
	 * See <ulink url="geoclue-types.html">geoclue-types.h</ulink> for the possible 
	 * GEOCLUE_ADDRESS_KEY_* keys used in @details.
	 * 
	 * Note that not all providers support signals.
	 */
	signals[ADDRESS_CHANGED] = g_signal_new ("address-changed",
						 G_TYPE_FROM_CLASS (klass),
						 G_SIGNAL_RUN_FIRST |
						 G_SIGNAL_NO_RECURSE,
						 G_STRUCT_OFFSET (GeoclueAddressClass, address_changed), 
						 NULL, NULL,
						 geoclue_marshal_VOID__INT_BOXED_BOXED,
						 G_TYPE_NONE, 3,
						 G_TYPE_INT, 
						 G_TYPE_POINTER,
						 G_TYPE_POINTER);
}

static void
geoclue_address_init (GeoclueAddress *address)
{
}

/**
 * geoclue_address_new:
 * @service: D-Bus service name
 * @path: D-Bus path name
 *
 * Creates a #GeoclueAddress with given D-Bus service name and path.
 * 
 * Return value: Pointer to a new #GeoclueAddress
 */
GeoclueAddress *
geoclue_address_new (const char *service,
		     const char *path)
{
	return g_object_new (GEOCLUE_TYPE_ADDRESS,
			     "service", service,
			     "path", path,
			     "interface", GEOCLUE_ADDRESS_INTERFACE_NAME,
			     NULL);
}

/**
 * geoclue_address_get_address:
 * @address: A #GeoclueAddress object
 * @timestamp: Pointer to returned time of address measurement (Unix timestamp) or %NULL
 * @details: Pointer to returned #GHashTable with address details or %NULL
 * @accuracy: Pointer to returned #GeoclueAccuracy or NULL
 * @error: Pointer to returned #Gerror or %NULL
 * 
 * Obtains the current address. @timestamp will contain the time of 
 * the actual address measurement. @accuracy is the estimated of the
 * accuracy of the current address information (see #GeoclueAccuracy 
 * for more details). @details is a hashtable with the address data, 
 * see <ulink url="geoclue-types.html">geoclue-types.h</ulink> for the 
 * hashtable keys.
 * 
 * If the caller is not interested in some values, the pointers can be 
 * left %NULL.
 * 
 * Return value: %TRUE if there is no @error
 */
gboolean
geoclue_address_get_address (GeoclueAddress   *address,
			     int              *timestamp,
			     GHashTable      **details,
			     GeoclueAccuracy **accuracy,
			     GError          **error)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (address);

	if (!org_freedesktop_Geoclue_Address_get_address (provider->proxy,
							  timestamp, details,
							  accuracy, error)) {
		return FALSE;
	}

	return TRUE;
}


typedef struct _GeoclueAddressAsyncData {
	GeoclueAddress *address;
	GCallback callback;
	gpointer userdata;
} GeoclueAddressAsyncData;

static void
get_address_async_callback (DBusGProxy              *proxy, 
			    int                      timestamp,
			    GHashTable              *details,
			    GeoclueAccuracy         *accuracy,
			    GError                  *error,
			    GeoclueAddressAsyncData *data)
{
	(*(GeoclueAddressCallback)data->callback) (data->address,
	                                           timestamp,
	                                           details,
	                                           accuracy,
	                                           error,
	                                           data->userdata);
	g_free (data);
}

/**
 * GeoclueAddressCallback:
 * @address: the #GeoclueAddress object emitting the signal
 * @timestamp: Time of address measurement (Unix timestamp)
 * @details: Address details as #GHashTable.
 * @accuracy: Accuracy of measurement as #GeoclueAccuracy
 * @error: Error as #Gerror (may be %NULL)
 * @userdata: User data pointer set in geoclue_position_get_position_async()
 * 
 * Callback function for geoclue_address_get_address_async().
 */

/**
 * geoclue_address_get_address_async:
 * @address: A #GeoclueAddress object
 * @callback: A #GeoclueAddressCallback function that should be called when return values are available
 * @userdata: pointer for user specified data
 * 
 * Function returns (essentially) immediately and calls @callback when current address
 * is available or when D-Bus timeouts.
 */
void 
geoclue_address_get_address_async (GeoclueAddress         *address,
				   GeoclueAddressCallback  callback,
				   gpointer                userdata)
{
	GeoclueProvider *provider = GEOCLUE_PROVIDER (address);
	GeoclueAddressAsyncData *data;
	
	data = g_new (GeoclueAddressAsyncData, 1);
	data->address = address;
	data->callback = G_CALLBACK (callback);
	data->userdata = userdata;
	
	org_freedesktop_Geoclue_Address_get_address_async
			(provider->proxy,
			 (org_freedesktop_Geoclue_Address_get_address_reply) get_address_async_callback,
			 data);
}
