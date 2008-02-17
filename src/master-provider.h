#ifndef MASTER_PROVIDER_H
#define MASTER_PROVIDER_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GC_TYPE_MASTER_PROVIDER (gc_master_provider_get_type ())
#define GC_MASTER_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_MASTER_PROVIDER, GcMasterProvider))
#define GC_IS_MASTER_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_MASTER_PROVIDER))

typedef struct _GcMasterProvider {
	GeoclueProvider provider;
} GcMasterProvider;

typedef struct _GcMasterProviderClass {
	GeoclueProviderClass provider_class;
	
	void (* position_changed) (GcMasterProvider     *master_provider,
	                           GeocluePositionFields fields,
	                           int                   timestamp,
	                           double                latitude,
	                           double                longitude,
	                           double                altitude,
	                           GeoclueAccuracy      *accuracy);
} GcMasterProviderClass;

GType gc_master_provider_get_type (void);

GcMasterProvider *gc_master_provider_new (const char *filename,
                                          gboolean    network_status_events);

gboolean gc_master_provider_is_position_provider (GcMasterProvider *master_provider);

gboolean gc_master_provider_is_good (GcMasterProvider     *provider,
                                     GeoclueAccuracy      *accuracy,
                                     GeoclueResourceFlags  allowed_resources,
                                     gboolean              need_update);
                                     
void gc_master_provider_network_status_changed (GcMasterProvider *provider,
                                                GeoclueStatus status);


GeocluePositionFields gc_master_provider_get_position (GcMasterProvider *master_provider,
                                                       int              *timestamp,
                                                       double           *latitude,
                                                       double           *longitude,
                                                       double           *altitude,
                                                       GeoclueAccuracy **accuracy,
                                                       GError          **error);

G_END_DECLS

#endif /* MASTER_PROVIDER_H */
