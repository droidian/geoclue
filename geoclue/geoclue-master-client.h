/*
 * Geoclue
 * geoclue-master-client.c - Client API for accessing the Geoclue Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GEOCLUE_MASTER_CLIENT_H
#define _GEOCLUE_MASTER_CLIENT_H

#include <glib-object.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-address.h>

G_BEGIN_DECLS

#define GEOCLUE_MASTER_CLIENT_DBUS_INTERFACE "org.freedesktop.Geoclue.MasterClient"

#define GEOCLUE_TYPE_MASTER_CLIENT (geoclue_master_client_get_type ())
#define GEOCLUE_MASTER_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_MASTER_CLIENT, GeoclueMasterClient))
#define GEOCLUE_IS_MASTER_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_MASTER_CLIENT))

typedef struct _GeoclueMasterClient {
	GObject parent;
} GeoclueMasterClient;

typedef struct _GeoclueMasterClientClass {
	GObjectClass parent_class;
	void (* address_provider_changed) (GeoclueMasterClient  *client,
	                                   char                 *name,
	                                   char                 *description,
	                                   char                 *service,
	                                   char                 *path);
	void (* position_provider_changed) (GeoclueMasterClient  *client,
	                                    char                 *name,
	                                    char                 *description,
	                                    char                 *service,
	                                    char                 *path);
} GeoclueMasterClientClass;

GType geoclue_master_client_get_type (void);

gboolean geoclue_master_client_set_requirements (GeoclueMasterClient   *client,
						 GeoclueAccuracyLevel   min_accuracy,
						 int                    min_time,
						 gboolean               require_updates,
						 GeoclueResourceFlags   allowed_resources,
						 GError               **error);
typedef void (*GeoclueSetRequirementsCallback) (GeoclueMasterClient  *client,
						GError               *error,
						gpointer              userdata);
void geoclue_master_client_set_requirements_async (GeoclueMasterClient           *client,
						   GeoclueAccuracyLevel           min_accuracy,
						   int                            min_time,
						   gboolean                       require_updates,
						   GeoclueResourceFlags           allowed_resources,
						   GeoclueSetRequirementsCallback callback,
						   gpointer                       userdata);

GeoclueAddress *geoclue_master_client_create_address (GeoclueMasterClient *client, GError **error);
typedef void (*CreateAddressCallback) (GeoclueMasterClient *client,
				       GeoclueAddress      *address,
				       GError              *error,
				       gpointer             userdata);
void geoclue_master_client_create_address_async (GeoclueMasterClient   *client,
						 CreateAddressCallback  callback,
						 gpointer               userdata);


GeocluePosition *geoclue_master_client_create_position (GeoclueMasterClient *client, GError **error);
typedef void (*CreatePositionCallback) (GeoclueMasterClient *client,
					GeocluePosition     *position,
					GError              *error,
					gpointer             userdata);
void geoclue_master_client_create_position_async (GeoclueMasterClient   *client,
						  CreatePositionCallback  callback,
						  gpointer               userdata);

gboolean geoclue_master_client_get_address_provider (GeoclueMasterClient  *client,
                                                     char                **name,
                                                     char                **description,
                                                     char                **service,
                                                     char                **path,
                                                     GError              **error);
typedef void (*GeoclueGetProviderCallback) (GeoclueMasterClient  *client,
                                            char                 *name,
                                            char                 *description,
                                            char                 *service,
                                            char                 *path,
                                            GError               *error,
                                            gpointer              userdata);
void geoclue_master_client_get_address_provider_async (GeoclueMasterClient        *client,
                                                       GeoclueGetProviderCallback  callback,
                                                       gpointer                    userdata);

gboolean geoclue_master_client_get_position_provider (GeoclueMasterClient  *client,
                                                     char                **name,
                                                     char                **description,
                                                     char                **service,
                                                     char                **path,
                                                     GError              **error);
void geoclue_master_client_get_position_provider_async (GeoclueMasterClient        *client,
                                                       GeoclueGetProviderCallback  callback,
                                                       gpointer userdata);

G_END_DECLS

#endif
