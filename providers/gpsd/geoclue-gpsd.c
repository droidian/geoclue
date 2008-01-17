/*
 * Geoclue
 * geoclue-gpsd.c - Geoclue backend for gpsd
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <gps.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-velocity.h>

typedef struct {
	GcProvider parent;
	
	struct gps_data_t *gpsdata;
	GMainLoop *loop;
} GeoclueGpsd;

typedef struct {
	GcProviderClass parent_class;
} GeoclueGpsdClass;

static void geoclue_gpsd_position_init (GcIfacePositionClass *iface);
static void geoclue_gpsd_velocity_init (GcIfaceVelocityClass *iface);

#define GEOCLUE_TYPE_GPSD (geoclue_gpsd_get_type ())
#define GEOCLUE_GPSD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GPSD, GeoclueGpsd))

G_DEFINE_TYPE_WITH_CODE (GeoclueGpsd, geoclue_gpsd, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                geoclue_gpsd_position_init)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_VELOCITY,
                                                geoclue_gpsd_velocity_init))


/* defining global GeoclueGpsd because gpsd does not support "user_data"
 * pointers in callbacks */
GeoclueGpsd *gpsd;



/* GcIfaceGeoclue methods */

static gboolean
get_status (GcIfaceGeoclue *gc,
            gboolean       *available,
            GError        **error)
{
	*available = TRUE;
	return TRUE;
}

static gboolean
shutdown (GcIfaceGeoclue *gc,
          GError        **error)
{
	GeoclueGpsd *self = GEOCLUE_GPSD (gc);
	
	g_main_loop_quit (self->loop);
	return TRUE;
}

static void
dispose (GObject *object)
{
	GeoclueGpsd *self = GEOCLUE_GPSD (object);
	
	if (self->gpsdata) {
		gps_close (self->gpsdata);     
	}
	
	((GObjectClass *) geoclue_gpsd_parent_class)->dispose (object);
}

static void
geoclue_gpsd_class_init (GeoclueGpsdClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	GcProviderClass *p_class = (GcProviderClass *) klass;

	o_class->dispose = dispose;

	p_class->get_status = get_status;
	p_class->shutdown = shutdown;
}


static void 
gpsd_callback (struct gps_data_t *gpsdata, char *message, size_t len, int level)
{
	if (gpsdata->set & (LATLON_SET)) {
		GeocluePositionFields fields;
		GeoclueAccuracy *accuracy;
		time_t ttime;
		
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_DETAILED,
		                                  gpsdata->hdop, gpsdata->vdop);
		
		fields = GEOCLUE_POSITION_FIELDS_LATITUDE | GEOCLUE_POSITION_FIELDS_LONGITUDE;
		fields |= (gpsdata->set & ALTITUDE_SET) ? 
		          GEOCLUE_POSITION_FIELDS_ALTITUDE : 0;
		
		/* TODO: gpsdata should have the time... */
		time (&ttime);
		
		g_debug ("gpsd callback");
		
		gc_iface_position_emit_position_changed 
			(GC_IFACE_POSITION (gpsd), fields,
			 ttime, gpsdata->fix.latitude, gpsdata->fix.longitude, 
			 gpsdata->fix.altitude, accuracy);
			 
		/* clear the flag */
		gpsdata->set &= ~LATLON_SET;
	}
	/* check status, speed, precision etc.*/
}


static void
geoclue_gpsd_init (GeoclueGpsd *self)
{
	pthread_t gps_thread;
	
	/* init gpsd (localhost, default port) */
	self->gpsdata = gps_open (NULL, DEFAULT_GPSD_PORT);
	if (self->gpsdata) {
		gps_set_callback (self->gpsdata, gpsd_callback, &gps_thread);
	} else {
		g_printerr("Cannot find gpsd!\n");
		/* TODO: What now? */
	}
	
	gc_provider_set_details (GC_PROVIDER (self),
				 "org.freedesktop.Geoclue.Providers.gpsd",
				 "/org/freedesktop/Geoclue/Providers/gpsd",
				 "gpsd", "gpsd provider");
}

static gboolean
get_position (GcIfacePosition       *gc,
              GeocluePositionFields *fields,
              int                   *timestamp,
              double                *latitude,
              double                *longitude,
              double                *altitude,
              GeoclueAccuracy      **accuracy,
              GError               **error)
{
	return TRUE;
}

static void
geoclue_gpsd_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
}

static gboolean
get_velocity (GcIfaceVelocity       *gc,
              GeoclueVelocityFields *fields,
              int                   *timestamp,
              double                *speed,
              double                *direction,
              double                *climb,
              GError               **error)
{
	return TRUE;
}

static void
geoclue_gpsd_velocity_init (GcIfaceVelocityClass *iface)
{
	iface->get_velocity = get_velocity;
}

int
main (int    argc,
      char **argv)
{
	g_type_init ();
	
	gpsd = g_object_new (GEOCLUE_TYPE_GPSD, NULL);
	
	
	gpsd->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (gpsd->loop);
	
	g_object_unref (gpsd);
	return 0;
}
