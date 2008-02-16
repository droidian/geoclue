/**
 *  A cached version of GeocluePosition for GcMaster.
 *  
 **/

#include "master-position.h"
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-marshal.h>


/*
#include <geoclue/geoclue-marshal.h>
*/

typedef struct _GcMasterPositionPrivate {
	GeocluePosition *position;
	
	gboolean use_cache;
	
	int timestamp; /* Last time cached data was updated */
	GeocluePositionFields fields;
	double latitude;
	double longitude;
	double altitude;
	GeoclueAccuracy *accuracy;
} GcMasterPositionPrivate;

enum {
	POSITION_CHANGED,
	LAST_SIGNAL
};
static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GC_TYPE_MASTER_POSITION, GcMasterPositionPrivate))

G_DEFINE_TYPE (GcMasterPosition, gc_master_position, G_TYPE_OBJECT)


static void
finalize (GObject *object)
{
	GcMasterPositionPrivate *priv = GET_PRIVATE (object);
	
	if (priv->accuracy) {
		geoclue_accuracy_free (priv->accuracy);
	}
	G_OBJECT_CLASS (gc_master_position_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GcMasterPositionPrivate *priv = GET_PRIVATE (object);
	
	if (priv->accuracy) {
		g_object_unref (priv->position);
	}
	
	G_OBJECT_CLASS (gc_master_position_parent_class)->dispose (object);
}



static void
gc_master_position_class_init (GcMasterPositionClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;
	
	g_type_class_add_private (klass, sizeof (GcMasterPositionPrivate));
	
	signals[POSITION_CHANGED] = g_signal_new ("position-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GcMasterPositionClass, position_changed), 
						  NULL, NULL,
						  geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE_BOXED,
						  G_TYPE_NONE, 6,
						  G_TYPE_INT, G_TYPE_INT,
						  G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE,
						  G_TYPE_POINTER);
}

static void
gc_master_position_init (GcMasterPosition *position)
{
	GcMasterPositionPrivate *priv = GET_PRIVATE (position);
	
	priv->accuracy = NULL;
}


void 
gc_master_position_update_cache (GcMasterPosition *position)
{
	GcMasterPositionPrivate *priv;
	int timestamp;
	double lat, lon, alt;
	GeocluePositionFields fields;
	GeoclueAccuracy *accuracy;
	GError *error;
	
	priv = GET_PRIVATE (position);
	
	error = NULL;
	fields = geoclue_position_get_position (priv->position,
	                                        &timestamp,
	                                        &lat, &lon, &alt,
	                                        &accuracy, 
	                                        &error);
	if (error)
	{
		g_warning ("Error updating position cache: %s", error->message);
		g_error_free (error);
		return;
		
		/* FIXME should cache the error too ? */
	}
	
	priv->timestamp = timestamp;
	/* FIXME check accuracy too */
	if (fields != priv->fields || 
	    lat != priv->latitude || 
	    lon != priv->longitude ||
	    alt != priv->altitude) {
		
		priv->fields = fields;
		priv->latitude = lat;
		priv->longitude = lon;
		priv->altitude = alt;
		
		/*FIXME: should emit a copy of accuracy? 
		  if not, should free the "previous" acccuracy ? */
		priv->accuracy = accuracy;
		
		g_signal_emit (position, signals[POSITION_CHANGED], 0, 
		               fields, timestamp, 
		               lat, lon, alt, 
		               accuracy);
		
	}
}

static void
position_changed (GeocluePosition      *position,
                  GeocluePositionFields fields,
                  int                   timestamp,
                  double                latitude,
                  double                longitude,
                  double                altitude,
                  GeoclueAccuracy      *accuracy,
                  GcMasterPosition     *master_pos)
{
	GcMasterPositionPrivate *priv = GET_PRIVATE (master_pos);
	/* is there a situation when we'd need to check against cache 
	 * if data has changed ? */
	if (priv->use_cache) {
		priv->timestamp = timestamp;
		priv->fields = fields;
		priv->latitude = latitude;
		priv->longitude = longitude;
		priv->altitude = altitude;
		
		/*FIXME free accuracy ? (see comment in update_cache) */
		priv->accuracy = accuracy;
	}
	g_signal_emit (master_pos, signals[POSITION_CHANGED], 0, 
	               fields, timestamp, 
	               latitude, longitude, altitude, 
	               accuracy);
}

GcMasterPosition *
gc_master_position_new (const char *service,
                        const char *path,
                        gboolean use_cache)
{
	GcMasterPosition *master_pos;
	GcMasterPositionPrivate *priv;
	
	master_pos = g_object_new (GC_TYPE_MASTER_POSITION, NULL);
	priv = GET_PRIVATE (master_pos);
	
	priv->position = g_object_new (GEOCLUE_TYPE_POSITION,
				       "service", service,
				       "path", path,
				       "interface", GEOCLUE_POSITION_INTERFACE_NAME,
				       NULL);
	priv->use_cache = use_cache;
	
	if (use_cache) {
		gc_master_position_update_cache (master_pos);
	}
	
	g_signal_connect (G_OBJECT (priv->position), "position-changed",
	                  G_CALLBACK (position_changed), master_pos);
	
	return master_pos;
}

GeocluePositionFields
gc_master_position_get_position (GcMasterPosition *position,
                                 int              *timestamp,
                                 double           *latitude,
                                 double           *longitude,
                                 double           *altitude,
                                 GeoclueAccuracy **accuracy,
                                 GError          **error)
{
	GcMasterPositionPrivate *priv = GET_PRIVATE (position);
	
	if (priv->use_cache) {
		if (timestamp != NULL) {
			*timestamp = priv->timestamp;
		}
		if (latitude != NULL) {
			*latitude = priv->latitude;
		}
		if (longitude != NULL) {
			*longitude = priv->longitude;
		}
		if (altitude != NULL) {
			*altitude = priv->altitude;
		}
		if (accuracy != NULL) {
			*accuracy = priv->accuracy;
		}
		return priv->fields;
	} else {
		return geoclue_position_get_position (priv->position,
		                                      timestamp,
		                                      latitude, 
		                                      longitude, 
		                                      altitude,
		                                      accuracy, 
		                                      error);
	}
}

