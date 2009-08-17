/* 
 * Geoclue
 * geoclue-provider.h - Client object for accessing Geoclue Providers
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *           2008 OpenedHand Ltd
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

#ifndef _GEOCLUE_PROVIDER_H
#define _GEOCLUE_PROVIDER_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <geoclue/geoclue-types.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_PROVIDER (geoclue_provider_get_type ())
#define GEOCLUE_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_PROVIDER, GeoclueProvider))
#define GEOCLUE_IS_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_PROVIDER))

typedef struct _GeoclueProvider {
	GObject object;
	
	DBusGProxy *proxy;
} GeoclueProvider;

typedef struct _GeoclueProviderClass {
	GObjectClass object_class;
	
	void (*status_changed) (GeoclueProvider *provider,
	                        GeoclueStatus    status);
} GeoclueProviderClass;

GType geoclue_provider_get_type (void);

gboolean geoclue_provider_get_status (GeoclueProvider  *provider,
                                      GeoclueStatus    *status,
                                      GError          **error);
typedef void (*GeoclueProviderStatusCallback) (GeoclueProvider *provider,
                                               GeoclueStatus    status,
                                               GError          *error,
                                               gpointer         userdata);
void geoclue_provider_get_status_async (GeoclueProvider               *provider,
                                        GeoclueProviderStatusCallback  callback,
                                        gpointer                       userdata);

gboolean geoclue_provider_get_provider_info (GeoclueProvider  *provider,
                                             char            **name,
                                             char            **description,
                                             GError          **error);
typedef void (*GeoclueProviderInfoCallback) (GeoclueProvider *provider,
                                             char            *name,
                                             char            *description,
                                             GError          *error,
                                             gpointer         userdata);
void geoclue_provider_get_provider_info_async (GeoclueProvider             *provider,
                                               GeoclueProviderInfoCallback  callback,
                                               gpointer                     userdata);

gboolean geoclue_provider_set_options (GeoclueProvider  *provider,
                                       GHashTable       *options,
                                       GError          **error);
typedef void (*GeoclueProviderOptionsCallback) (GeoclueProvider *provider,
                                                GError          *error,
                                                gpointer         userdata);
void geoclue_provider_set_options_async (GeoclueProvider                *provider,
                                         GHashTable                     *options,
                                         GeoclueProviderOptionsCallback  callback,
                                         gpointer                        userdata);

G_END_DECLS

#endif
