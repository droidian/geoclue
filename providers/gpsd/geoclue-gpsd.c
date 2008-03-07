/*
 * Geoclue
 * geoclue-gpsd.c - Geoclue backend for gpsd
 *
 * Authors: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/* TODO:
 * 
 * 	call to gps_set_callback blocks for a long time if 
 * 	BT device is not present.
 * 
 **/

#include <config.h>

#include <math.h>
#include <gps.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-velocity.h>

typedef struct gps_data_t gps_data;
typedef struct gps_fix_t gps_fix;

/* only listing used tags */
typedef enum {
	NMEA_NONE,
	NMEA_GSA,
	NMEA_GGA,
	NMEA_GSV,
	NMEA_RMC
} NmeaTag;


typedef struct {
	GcProvider parent;
	
	pthread_t *gps_thread;
	gps_data *gpsdata;
	
	gps_fix *last_fix;
	
	GeoclueStatus last_status;
	GeocluePositionFields last_pos_fields;
	GeoclueAccuracy *last_accuracy;
	GeoclueVelocityFields last_velo_fields;
	
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

static void geoclue_gpsd_stop_gpsd (GeoclueGpsd *self);
static gboolean geoclue_gpsd_start_gpsd (GeoclueGpsd *self, char *host, char *port);


/* defining global GeoclueGpsd because gpsd does not support "user_data"
 * pointers in callbacks */
GeoclueGpsd *gpsd;



/* Geoclue interface */
static gboolean
get_status (GcIfaceGeoclue *gc,
            GeoclueStatus  *status,
            GError        **error)
{
	GeoclueGpsd *gpsd = GEOCLUE_GPSD (gc);
	
	*status = gpsd->last_status;
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueGpsd *gpsd = GEOCLUE_GPSD (provider);
	
	g_main_loop_quit (gpsd->loop);
}

static gboolean
set_options (GcIfaceGeoclue *gc,
             GHashTable     *options,
             GError        **error)
{
	GeoclueGpsd *gpsd = GEOCLUE_GPSD (gc);
	char *host;
	char *port;
	
	host = g_hash_table_lookup (options, 
	                                   "org.freedesktop.Geoclue.GPSHost");
	port = g_hash_table_lookup (options, 
	                            "org.freedesktop.Geoclue.GPSPort");
	if (!port) {
		port = DEFAULT_GPSD_PORT;
	}
	
	geoclue_gpsd_stop_gpsd (gpsd);
	return geoclue_gpsd_start_gpsd (gpsd, host, port);
}

static void
finalize (GObject *object)
{
	GeoclueGpsd *gpsd = GEOCLUE_GPSD (object);
	
	geoclue_gpsd_stop_gpsd (gpsd);
	g_free (gpsd->last_fix);
	geoclue_accuracy_free (gpsd->last_accuracy);
	
	((GObjectClass *) geoclue_gpsd_parent_class)->dispose (object);
}

static void
geoclue_gpsd_class_init (GeoclueGpsdClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	GcProviderClass *p_class = (GcProviderClass *) klass;
	
	o_class->finalize = finalize;
	
	p_class->get_status = get_status;
	p_class->set_options = set_options;
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
geoclue_gpsd_update_position (GeoclueGpsd *gpsd, NmeaTag nmea_tag)
{
	gps_fix *fix = &gpsd->gpsdata->fix;
	gps_fix *last_fix = gpsd->last_fix;
	
	last_fix->time = fix->time;
	
	/* If a flag is not set, bail out.*/
	if (!((gpsd->gpsdata->set & LATLON_SET) || (gpsd->gpsdata->set & ALTITUDE_SET))) {
		return;
	}
	gpsd->gpsdata->set &= ~(LATLON_SET | ALTITUDE_SET);
	
	if (equal_or_nan (fix->latitude, last_fix->latitude) &&
	    equal_or_nan (fix->longitude, last_fix->longitude) &&
	    equal_or_nan (fix->altitude, last_fix->altitude)) {
		/* position has not changed */
		return;
	}
	
	/* save values */
	last_fix->latitude = fix->latitude;
	last_fix->longitude = fix->longitude;
	last_fix->altitude = fix->altitude;
	
	/* Could use fix.eph for accuracy, but eph is 
	 * often NaN... what then? 
	 * Could also use fix mode (2d/3d) to decide vertical accuracy, 
	 * but gpsd updates that so erratically that I couldn't
	 * be arsed so far */
	geoclue_accuracy_set_details (gpsd->last_accuracy,
	                              GEOCLUE_ACCURACY_LEVEL_DETAILED,
	                              24, 60);
	
	gpsd->last_pos_fields = GEOCLUE_POSITION_FIELDS_NONE;
	gpsd->last_pos_fields |= (isnan (fix->latitude)) ? 
	                         0 : GEOCLUE_POSITION_FIELDS_LATITUDE;
	gpsd->last_pos_fields |= (isnan (fix->longitude)) ? 
	                         0 : GEOCLUE_POSITION_FIELDS_LONGITUDE;
	gpsd->last_pos_fields |= (isnan (fix->altitude)) ? 
	                         0 : GEOCLUE_POSITION_FIELDS_ALTITUDE;
	
	gc_iface_position_emit_position_changed 
		(GC_IFACE_POSITION (gpsd), gpsd->last_pos_fields,
		 (int)(last_fix->time+0.5), 
		 last_fix->latitude, last_fix->longitude, last_fix->altitude, 
		 gpsd->last_accuracy);
	
}

static void
geoclue_gpsd_update_velocity (GeoclueGpsd *gpsd, NmeaTag nmea_tag)
{
	gps_fix *fix = &gpsd->gpsdata->fix;
	gps_fix *last_fix = gpsd->last_fix;
	gboolean changed = FALSE;
	
	/* at least with my devices, gpsd updates
	 *  - climb on GGA, GSA and GSV messages (speed and track are set to NaN).
	 *  - speed and track on RMC message (climb is set to NaN).
	 * 
	 * couldn't think of an smart way to handle this, I don't think there is one
	 */
	
	if (((gpsd->gpsdata->set & TRACK_SET) || (gpsd->gpsdata->set & SPEED_SET)) &&
	    nmea_tag == NMEA_RMC) {
		
		gpsd->gpsdata->set &= ~(TRACK_SET | SPEED_SET);
		
		last_fix->time = fix->time;
		
		if (!equal_or_nan (fix->track, last_fix->track) ||
		    !equal_or_nan (fix->speed, last_fix->speed)){
			
			/* velocity has changed */
			changed = TRUE;
			last_fix->track = fix->track;
			last_fix->speed = fix->speed;
		}
	} else if ((gpsd->gpsdata->set & CLIMB_SET) &&
	           (nmea_tag == NMEA_GGA || 
	            nmea_tag == NMEA_GSA || 
	            nmea_tag == NMEA_GSV)) {
		
		gpsd->gpsdata->set &= ~(CLIMB_SET);
		
		last_fix->time = fix->time;
		
		if (!equal_or_nan (fix->climb, last_fix->climb)){
			
			/* velocity has changed */
			changed = TRUE;
			last_fix->climb = fix->climb;
		}
	}
	
	if (changed) {
		gpsd->last_velo_fields = GEOCLUE_VELOCITY_FIELDS_NONE;
		gpsd->last_velo_fields |= (isnan (last_fix->track)) ?
			0 : GEOCLUE_VELOCITY_FIELDS_DIRECTION;
		gpsd->last_velo_fields |= (isnan (last_fix->speed)) ?
			0 : GEOCLUE_VELOCITY_FIELDS_SPEED;
		gpsd->last_velo_fields |= (isnan (last_fix->climb)) ?
			0 : GEOCLUE_VELOCITY_FIELDS_CLIMB;
		
		gc_iface_velocity_emit_velocity_changed 
			(GC_IFACE_VELOCITY (gpsd), gpsd->last_velo_fields,
			 (int)(last_fix->time+0.5),
			 last_fix->speed, last_fix->track, last_fix->climb);
	}
}

static void
geoclue_gpsd_update_status (GeoclueGpsd *gpsd, NmeaTag nmea_tag)
{
	gboolean changed = FALSE;
	gboolean online = (gpsd->gpsdata->online > 0);
	
	/* gpsdata->online is supposedly always up-to-date */
	if (online && (gpsd->last_status == GEOCLUE_STATUS_UNAVAILABLE)) {
		gpsd->last_status = GEOCLUE_STATUS_ACQUIRING;
		changed = TRUE;
	} else if (!online && (gpsd->last_status != GEOCLUE_STATUS_UNAVAILABLE)) {
		gpsd->last_status = GEOCLUE_STATUS_UNAVAILABLE;
		changed = TRUE;
	}
	
	if ((gpsd->last_status != GEOCLUE_STATUS_UNAVAILABLE) && 
	    (gpsd->gpsdata->set & STATUS_SET)) {
		gboolean fix = (gpsd->gpsdata->status > 0);
		
		gpsd->gpsdata->set &= ~(STATUS_SET);
		
		if (fix && gpsd->last_status != GEOCLUE_STATUS_AVAILABLE) {
			gpsd->last_status = GEOCLUE_STATUS_AVAILABLE;
			changed = TRUE;
		} else if (!fix && gpsd->last_status == GEOCLUE_STATUS_AVAILABLE) {
			gpsd->last_status = GEOCLUE_STATUS_UNAVAILABLE;
			changed = TRUE;
		}
	}
	if (changed) {
		/* make position and velocity invalid if no fix */
		if (gpsd->last_status != GEOCLUE_STATUS_AVAILABLE) {
			gpsd->last_pos_fields = GEOCLUE_POSITION_FIELDS_NONE;
			gpsd->last_velo_fields = GEOCLUE_VELOCITY_FIELDS_NONE;
		}
		
		gc_iface_geoclue_emit_status_changed (GC_IFACE_GEOCLUE (gpsd),
		                                      gpsd->last_status);
	}
}

static void 
gpsd_callback (struct gps_data_t *gpsdata, char *message, size_t len, int level)
{
	char *tag_str = gpsd->gpsdata->tag;
	NmeaTag nmea_tag = NMEA_NONE;
	
	if (tag_str[0] == 'G' && tag_str[1] == 'S' && tag_str[2] == 'A') {
		nmea_tag = NMEA_GSA;
	} else if (tag_str[0] == 'G' && tag_str[1] == 'G' && tag_str[2] == 'A') {
		nmea_tag = NMEA_GGA;
	} else if (tag_str[0] == 'G' && tag_str[1] == 'S' && tag_str[2] == 'V') {
		nmea_tag = NMEA_GSV;
	} else if (tag_str[0] == 'R' && tag_str[1] == 'M' && tag_str[2] == 'C') {
		nmea_tag = NMEA_RMC;
	}
	
	geoclue_gpsd_update_status (gpsd, nmea_tag);
	geoclue_gpsd_update_position (gpsd, nmea_tag);
	geoclue_gpsd_update_velocity (gpsd, nmea_tag);
}

static void
geoclue_gpsd_stop_gpsd (GeoclueGpsd *self)
{
	if (self->gpsdata) {
		gps_del_callback (self->gpsdata, self->gps_thread);
		gps_close (self->gpsdata);
		self->gpsdata = NULL;
		self->last_status = GEOCLUE_STATUS_ERROR;
	}
	
}

static gboolean
geoclue_gpsd_start_gpsd (GeoclueGpsd *self, char *host, char *port)
{
	self->gpsdata = gps_open (host, port);
	if (self->gpsdata) {
		self->last_status = GEOCLUE_STATUS_UNAVAILABLE;
		
		/* FIXME: This will block for a long time (10-80 seconds) if device is not available */
		gps_set_callback (self->gpsdata, gpsd_callback, self->gps_thread);
		return TRUE;
	} else {
		self->last_status = GEOCLUE_STATUS_ERROR;
		return FALSE;
	}
}

static void
geoclue_gpsd_init (GeoclueGpsd *self)
{
	self->gpsdata = NULL;
	self->gps_thread = g_new0 (pthread_t, 1);
	self->last_fix = g_new0 (gps_fix, 1);
	
	self->last_pos_fields = GEOCLUE_POSITION_FIELDS_NONE;
	self->last_velo_fields = GEOCLUE_VELOCITY_FIELDS_NONE;
	self->last_accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
	
	gc_provider_set_details (GC_PROVIDER (self),
				 "org.freedesktop.Geoclue.Providers.Gpsd",
				 "/org/freedesktop/Geoclue/Providers/Gpsd",
				 "Gpsd", "Gpsd provider");
	
	geoclue_gpsd_start_gpsd (self, NULL, DEFAULT_GPSD_PORT);
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
	GeoclueGpsd *gpsd = GEOCLUE_GPSD (gc);
	
	*timestamp = (int)(gpsd->last_fix->time+0.5);
	*latitude = gpsd->last_fix->latitude;
	*longitude = gpsd->last_fix->longitude;
	*altitude = gpsd->last_fix->altitude;
	*fields = gpsd->last_pos_fields;
	*accuracy = geoclue_accuracy_copy (gpsd->last_accuracy);
	
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
	GeoclueGpsd *gpsd = GEOCLUE_GPSD (gc);
	
	*timestamp = (int)(gpsd->last_fix->time+0.5);
	*speed = gpsd->last_fix->speed;
	*direction = gpsd->last_fix->track;
	*climb = gpsd->last_fix->climb;
	*fields = gpsd->last_velo_fields;
	
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
	
	g_main_loop_unref (gpsd->loop);
	g_object_unref (gpsd);
	
	return 0;
}
