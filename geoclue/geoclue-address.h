/*
 * Geoclue
 * geoclue-address.h - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
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

typedef void (*GeoclueAddressCallback) (GeoclueAddress   *address,
					int               timestamp,
					GHashTable       *details,
					GeoclueAccuracy  *accuracy,
					GError           *error,
					gpointer          userdata);

void geoclue_address_get_address_async (GeoclueAddress         *address,
					GeoclueAddressCallback  callback,
					gpointer                userdata);

G_END_DECLS

#endif
