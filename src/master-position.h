#ifndef MASTER_POSITION_H
#define MASTER_POSITION_H

#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

G_BEGIN_DECLS

#define GC_TYPE_MASTER_POSITION (gc_master_position_get_type ())
#define GC_MASTER_POSITION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GC_TYPE_MASTER_POSITION, GcMasterPosition))
#define GC_IS_MASTER_POSITION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GC_TYPE_MASTER_POSITION))

#define GEOCLUE_POSITION_INTERFACE_NAME "org.freedesktop.Geoclue.Position"

typedef struct _GcMasterPosition {
	GeoclueProvider provider;
} GcMasterPosition;

typedef struct _GcMasterPositionClass {
	GeoclueProviderClass provider_class;
	
	void (* position_changed) (GcMasterPosition     *position,
				   GeocluePositionFields fields,
				   int                   timestamp,
				   double                latitude,
				   double                longitude,
				   double                altitude,
				   GeoclueAccuracy      *accuracy);
} GcMasterPositionClass;

GType gc_master_position_get_type (void);

GcMasterPosition *gc_master_position_new (const char *service,
				          const char *path,
				          gboolean use_cache);

GeocluePositionFields gc_master_position_get_position (GcMasterPosition *position,
						       int              *timestamp,
						       double           *latitude,
						       double           *longitude,
						       double           *altitude,
						       GeoclueAccuracy **accuracy,
						       GError          **error);

void gc_master_position_update_cache (GcMasterPosition *position);

G_END_DECLS

#endif /* MASTER_POSITION_H */
