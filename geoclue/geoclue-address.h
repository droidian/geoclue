/*
 * Geoclue
 * geoclue-address.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GEOCLUE_ADDRESS_H
#define _GEOCLUE_ADDRESS_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-address-details.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_ADDRESS (geoclue_address_get_type ())
#define GEOCLUE_ADDRESS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_ADDRESS, GeoclueAddress))
#define GEOCLUE_IS_ADDRESS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_ADDRESS))

#define GEOCLUE_ADDRESS_INTERFACE_NAME "org.freedesktop.Geoclue.Address"

typedef struct _GeoclueAddress {
	GeoclueProvider provider;
} GeoclueAddress;

typedef struct _GeoclueAddressClass {
	GeoclueProviderClass provider_class;

	void (* address_changed) (GeoclueAddress  *address,
				  int              timestamp,
				  GHashTable      *details,
				  GeoclueAccuracy *accuracy);
} GeoclueAddressClass;

GType geoclue_address_get_type (void);

GeoclueAddress *geoclue_address_new (const char *service,
				     const char *path);

gboolean geoclue_address_get_address (GeoclueAddress   *address,
				      int              *timestamp,
				      GHashTable      **details,
				      GeoclueAccuracy **accuracy,
				      GError          **error);
G_END_DECLS

#endif
