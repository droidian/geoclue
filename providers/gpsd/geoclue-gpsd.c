/*
 * Geoclue
 * geoclue-gpsd.c - Geoclue backend for gpsd
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <math.h>

#include <gps.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-velocity.h>

typedef struct gps_data_t gps_data;

typedef struct {
	GcProvider parent;
	
	gps_data *gpsdata;
	gps_data *last_gpsdata;
	
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
	if (self->last_gpsdata) {
		g_free (self->last_gpsdata);
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

static gboolean
equal_or_nan (double a, double b)
{
	if (isnan (a) && isnan (b)) {
		return TRUE;
	}
	return a == b;
}

static void 
gpsd_callback (struct gps_data_t *gpsdata, char *message, size_t len, int level)
{
	/* TODO: how to figure we're offline? */
	/* FIXME: move the psoitionupdate and vel update to own functions*/
	
	/* Position */
	if ((gpsdata->set & LATLON_SET) || (gpsdata->set & ALTITUDE_SET)) {
		GeocluePositionFields fields = GEOCLUE_POSITION_FIELDS_NONE;
		GeoclueAccuracy *accuracy;
		
		/* clear the flag */
		gpsdata->set &= ~(LATLON_SET | ALTITUDE_SET);
		
		if (equal_or_nan (gpsdata->fix.latitude, gpsd->last_gpsdata->fix.latitude) &&
		    equal_or_nan (gpsdata->fix.longitude, gpsd->last_gpsdata->fix.longitude) &&
		    equal_or_nan (gpsdata->fix.altitude, gpsd->last_gpsdata->fix.altitude)) {
			/* don't emit if no position change */
			return;
		}
		
		/* save values */
		gpsd->last_gpsdata->fix.latitude = gpsdata->fix.latitude;
		gpsd->last_gpsdata->fix.longitude = gpsdata->fix.longitude;
		gpsd->last_gpsdata->fix.altitude = gpsdata->fix.altitude;
		
		
		/* Could use fix.eph for accuracy, but eph is 
		 * often NaN... what then? */
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_DETAILED,
		                                 24, 60);
		
		fields |= (isnan (gpsdata->fix.latitude)) ? 
		          0 : GEOCLUE_POSITION_FIELDS_LATITUDE;
		fields |= (isnan (gpsdata->fix.longitude)) ? 
		          0 : GEOCLUE_POSITION_FIELDS_LONGITUDE;
		fields |= (isnan (gpsdata->fix.altitude)) ? 
		          0 : GEOCLUE_POSITION_FIELDS_ALTITUDE;
		
		gc_iface_position_emit_position_changed 
			(GC_IFACE_POSITION (gpsd), fields,
			 (int)(gpsdata->fix.time+0.5), 
			 gpsdata->fix.latitude, gpsdata->fix.longitude, 
			 gpsdata->fix.altitude, accuracy);
		
		geoclue_accuracy_free (accuracy);
	}
	
	
	/* Velocity */
	/* gpsd seems to send two alternating messages:
	 * one has valid climb, other has valid track/speed
	 * */
	if ((gpsdata->set & TRACK_SET) || (gpsdata->set & SPEED_SET) ||
	    (gpsdata->set & CLIMB_SET)) {
		
		GeoclueVelocityFields fields = GEOCLUE_VELOCITY_FIELDS_NONE;
		GeoclueAccuracy *accuracy;
		
		/* clear the flags */
		gpsdata->set &= ~(TRACK_SET | SPEED_SET | CLIMB_SET);
		
		if (equal_or_nan (gpsdata->fix.track, gpsd->last_gpsdata->fix.track) &&
		    equal_or_nan (gpsdata->fix.speed, gpsd->last_gpsdata->fix.speed) &&
		    equal_or_nan (gpsdata->fix.climb, gpsd->last_gpsdata->fix.climb)) {
			/* don't emit if no velocity change */
			return;
		}
		gpsd->last_gpsdata->fix.track = gpsdata->fix.track;
		gpsd->last_gpsdata->fix.speed = gpsdata->fix.speed;
		gpsd->last_gpsdata->fix.climb = gpsdata->fix.climb;
		
		
		/* Could fix.epd/fix.eps for accuracy... */
		/* FIXME: what does accuracy represent here? degrees / meters*/
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_DETAILED,
		                                 24, 60);
		                                 
		fields |= (isnan (gpsdata->fix.track)) ?
		          0 : GEOCLUE_VELOCITY_FIELDS_DIRECTION;
		fields |= (isnan (gpsdata->fix.speed)) ?
		          0 : GEOCLUE_VELOCITY_FIELDS_SPEED;
		fields |= (isnan (gpsdata->fix.climb)) ?
		          0 : GEOCLUE_VELOCITY_FIELDS_CLIMB;
		
		gc_iface_velocity_emit_velocity_changed 
			(GC_IFACE_VELOCITY (gpsd), fields,
			 (int)(gpsdata->fix.time+0.5),
			 gpsdata->fix.speed, gpsdata->fix.speed, 
			 gpsdata->fix.climb);
		 
		geoclue_accuracy_free (accuracy);
 		g_debug ("vel %f, %f, %f (%d)", gpsdata->fix.speed, gpsdata->fix.track, gpsdata->fix.climb, fields);
	}
}


static void
geoclue_gpsd_init (GeoclueGpsd *self)
{
	pthread_t gps_thread;
	
	self->last_gpsdata = g_new0 (gps_data, 1);
	
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
