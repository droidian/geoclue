#ifndef MASTER_PROVIDER_H
#define MASTER_PROVIDER_H


#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>
#include "connectivity.h"

G_BEGIN_DECLS

#define GC_TYPE_MASTER_PROVIDER (gc_master_provider_get_type ())
#define GC_MASTER_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_MASTER_PROVIDER, GcMasterProvider))
#define GC_IS_MASTER_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_MASTER_PROVIDER))

typedef enum {
	GC_IFACE_NONE = 0, 
	GC_IFACE_GEOCLUE = 1 << 0, 
	GC_IFACE_POSITION = 1 << 1,
	GC_IFACE_ADDRESS = 1 << 2,
} GcInterfaceFlags;


typedef struct _GcMasterProvider {
	GeoclueProvider provider;
} GcMasterProvider;

typedef struct _GcMasterProviderClass {
	GeoclueProviderClass provider_class;
	
	void (* status_changed) (GcMasterProvider *master_provider,
	                         GeoclueStatus     status);
	void (* position_changed) (GcMasterProvider     *master_provider,
	                           GeocluePositionFields fields,
	                           int                   timestamp,
	                           double                latitude,
	                           double                longitude,
	                           double                altitude,
	                           GeoclueAccuracy      *accuracy);
	void (* address_changed) (GcMasterProvider *master_provider,
	                          int               timestamp,
	                          GHashTable       *details,
	                          GeoclueAccuracy  *accuracy);
} GcMasterProviderClass;

GType gc_master_provider_get_type (void);

GcMasterProvider *gc_master_provider_new (const char *filename,
                                          GeoclueConnectivity *connectivity);

gint gc_master_provider_compare_by_accuracy (GcMasterProvider *a, GcMasterProvider *b);

gboolean gc_master_provider_is_good (GcMasterProvider     *provider,
                                     GcInterfaceFlags      iface_types,
                                     GeoclueAccuracyLevel  min_accuracy,
                                     gboolean              need_update,
                                     GeoclueResourceFlags  allowed_resources);

void gc_master_provider_network_status_changed (GcMasterProvider *provider,
                                                GeoclueNetworkStatus status);

GeoclueNetworkStatus gc_master_provider_get_status (GcMasterProvider *provider);

GeocluePositionFields gc_master_provider_get_position (GcMasterProvider *master_provider,
                                                       int              *timestamp,
                                                       double           *latitude,
                                                       double           *longitude,
                                                       double           *altitude,
                                                       GeoclueAccuracy **accuracy,
                                                       GError          **error);

gboolean gc_master_provider_get_address (GcMasterProvider  *master_provider,
                                         int               *timestamp,
                                         GHashTable       **details,
                                         GeoclueAccuracy  **accuracy,
                                         GError           **error);

G_END_DECLS

#endif /* MASTER_PROVIDER_H */
