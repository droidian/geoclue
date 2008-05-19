/*
 * Geoclue
 * geoclue-gypsy.c - Geoclue backend for Gypsy which provides the Position.
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-course.h>
#include <gypsy/gypsy-accuracy.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-velocity.h>

typedef struct {
	GcProvider parent;

        char *device_name;
	
	GypsyControl *control;
	GypsyDevice *device;
	GypsyPosition *position;
	GypsyCourse *course;
	GypsyAccuracy *acc;

	GMainLoop *loop;

	int timestamp;

	GeoclueStatus status;

	/* Cached so we don't have to make D-Bus method calls all the time */
	GypsyPositionFields position_fields;
	double latitude;
	double longitude;
	double altitude;

	GypsyCourseFields course_fields;
	double speed;
	double direction;
	double climb;

	GeoclueAccuracy *accuracy;
} GeoclueGypsy;

typedef struct {
	GcProviderClass parent_class;
} GeoclueGypsyClass;

static void geoclue_gypsy_position_init (GcIfacePositionClass *iface);
static void geoclue_gypsy_velocity_init (GcIfaceVelocityClass *iface);

#define GEOCLUE_TYPE_GYPSY (geoclue_gypsy_get_type ())
#define GEOCLUE_GYPSY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GYPSY, GeoclueGypsy))

G_DEFINE_TYPE_WITH_CODE (GeoclueGypsy, geoclue_gypsy, GC_TYPE_PROVIDER,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						geoclue_gypsy_position_init)
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_VELOCITY,
						geoclue_gypsy_velocity_init))


/* GcIfaceGeoclue methods */

static gboolean
get_status (GcIfaceGeoclue *gc,
            GeoclueStatus  *status,
            GError        **error)
{
	GeoclueGypsy *gypsy = GEOCLUE_GYPSY (gc);

	*status = gypsy->status;

        return TRUE;
}


/* Compare the two fields and return TRUE if they have changed */
static gboolean
compare_field (GypsyPositionFields fields_a,
	       double              value_a,
	       GypsyPositionFields fields_b,
	       double              value_b,
	       GypsyPositionFields field)
{
	/* If both fields are valid, compare the values */
	if ((fields_a & field) && (fields_b & field)) {
		if (value_a == value_b) {
			return FALSE;
		} else {
			return TRUE;
		}
	}

	/* Otherwise return if both the fields set are the same */
	return ((fields_a & field) != (fields_b & field));
}

static GeocluePositionFields
gypsy_position_to_geoclue (GypsyPositionFields fields)
{
	GeocluePositionFields gc_fields = GEOCLUE_POSITION_FIELDS_NONE;
	
	gc_fields |= (fields & GYPSY_POSITION_FIELDS_LATITUDE) ? GEOCLUE_POSITION_FIELDS_LATITUDE : 0;
	gc_fields |= (fields & GYPSY_POSITION_FIELDS_LONGITUDE) ? GEOCLUE_POSITION_FIELDS_LONGITUDE : 0;
	gc_fields |= (fields & GYPSY_POSITION_FIELDS_ALTITUDE) ? GEOCLUE_POSITION_FIELDS_ALTITUDE : 0;
	
	return gc_fields;
}
	      
static GeoclueVelocityFields
gypsy_course_to_geoclue (GypsyCourseFields fields)
{
	GeoclueVelocityFields gc_fields = GEOCLUE_VELOCITY_FIELDS_NONE;

	gc_fields |= (fields & GYPSY_COURSE_FIELDS_SPEED) ? GEOCLUE_VELOCITY_FIELDS_SPEED : 0;
	gc_fields |= (fields & GYPSY_COURSE_FIELDS_DIRECTION) ? GEOCLUE_VELOCITY_FIELDS_DIRECTION : 0;
	gc_fields |= (fields & GYPSY_COURSE_FIELDS_CLIMB) ? GEOCLUE_VELOCITY_FIELDS_CLIMB : 0;

	return gc_fields;
}

static void
position_changed (GypsyPosition      *position,
		  GypsyPositionFields fields,
		  int                 timestamp,
		  double              latitude,
		  double              longitude,
		  double              altitude,
		  GeoclueGypsy       *gypsy)
{
	gboolean changed = FALSE;

        g_print ("Gypsy position changed\n");
	gypsy->timestamp = timestamp;
	if (compare_field (gypsy->position_fields, gypsy->latitude,
			   fields, latitude, GYPSY_POSITION_FIELDS_LATITUDE)) {
		if (fields | GYPSY_POSITION_FIELDS_LATITUDE) {
			gypsy->position_fields |= GYPSY_POSITION_FIELDS_LATITUDE;
			gypsy->latitude = latitude;
			changed = TRUE;
		}
	}

	if (compare_field (gypsy->position_fields, gypsy->longitude,
			   fields, longitude, GYPSY_POSITION_FIELDS_LONGITUDE)) {
		if (fields | GYPSY_POSITION_FIELDS_LONGITUDE) {
			gypsy->position_fields |= GYPSY_POSITION_FIELDS_LONGITUDE;
			gypsy->longitude = longitude;
			changed = TRUE;
		}
	}

	if (compare_field (gypsy->position_fields, gypsy->altitude,
			   fields, altitude, GYPSY_POSITION_FIELDS_ALTITUDE)) {
		if (fields | GYPSY_POSITION_FIELDS_ALTITUDE) {
			gypsy->position_fields |= GYPSY_POSITION_FIELDS_ALTITUDE;
			gypsy->altitude = altitude;
			changed = TRUE;
		}
	}

	if (changed) {
		GeocluePositionFields fields;

                g_print ("Emitting signal\n");
		fields = gypsy_position_to_geoclue (gypsy->position_fields);
		gc_iface_position_emit_position_changed 
			(GC_IFACE_POSITION (gypsy), fields,
			 timestamp, gypsy->latitude, gypsy->longitude, 
			 gypsy->altitude, gypsy->accuracy);
	}
}

static void
course_changed (GypsyCourse      *course,
		GypsyCourseFields fields,
		int               timestamp,
		double            speed,
		double            direction,
		double            climb,
		GeoclueGypsy     *gypsy)
{
	gboolean changed = FALSE;

	gypsy->timestamp = timestamp;
	if (compare_field (gypsy->course_fields, gypsy->speed,
			   fields, speed, GYPSY_COURSE_FIELDS_SPEED)) {
		if (fields & GYPSY_COURSE_FIELDS_SPEED) {
			gypsy->course_fields |= GYPSY_COURSE_FIELDS_SPEED;
			gypsy->speed = speed;
			changed = TRUE;
		}
	}

	if (compare_field (gypsy->course_fields, gypsy->direction,
			   fields, direction, GYPSY_COURSE_FIELDS_DIRECTION)) {
		if (fields & GYPSY_COURSE_FIELDS_DIRECTION) {
			gypsy->course_fields |= GYPSY_COURSE_FIELDS_DIRECTION;
			gypsy->direction = direction;
			changed = TRUE;
		}
	}

	if (compare_field (gypsy->course_fields, gypsy->climb,
			   fields, climb, GYPSY_COURSE_FIELDS_CLIMB)) {
		if (fields & GYPSY_COURSE_FIELDS_CLIMB) {
			gypsy->course_fields |= GYPSY_COURSE_FIELDS_CLIMB;
			gypsy->climb = climb;
			changed = TRUE;
		}
	}

	if (changed) {
		GeoclueVelocityFields fields;

		fields = gypsy_course_to_geoclue (gypsy->course_fields);
		gc_iface_velocity_emit_velocity_changed 
			(GC_IFACE_VELOCITY (gypsy), fields,
			 timestamp, gypsy->speed, gypsy->direction, gypsy->climb);
	}
}
		
static void
accuracy_changed (GypsyAccuracy      *accuracy,
		  GypsyAccuracyFields fields,
		  double              pdop,
		  double              hdop,
		  double              vdop,
		  GeoclueGypsy       *gypsy)
{
	gboolean changed = FALSE;
	GeoclueAccuracyLevel level;
	double horiz, vert;

	geoclue_accuracy_get_details (gypsy->accuracy, &level, &horiz, &vert);
	if (fields & (GYPSY_ACCURACY_FIELDS_HORIZONTAL | 
		      GYPSY_ACCURACY_FIELDS_VERTICAL)){
		if (level != GEOCLUE_ACCURACY_LEVEL_DETAILED ||
		    horiz != hdop || vert != vdop) {
			changed = TRUE;
		}

		geoclue_accuracy_set_details (gypsy->accuracy,
					      GEOCLUE_ACCURACY_LEVEL_DETAILED,
					      hdop, vdop);
	} else {

		if (level != GEOCLUE_ACCURACY_LEVEL_NONE ||
		    horiz != 0.0 || vert != 0.0) {
			changed = TRUE;
		}

		geoclue_accuracy_set_details (gypsy->accuracy,
					      GEOCLUE_ACCURACY_LEVEL_NONE,
					      0.0, 0.0);
	}

	if (changed) {
		GeocluePositionFields fields;
		
		fields = gypsy_position_to_geoclue (gypsy->position_fields);
		gc_iface_position_emit_position_changed 
			(GC_IFACE_POSITION (gypsy), fields,
			 gypsy->timestamp, gypsy->latitude, gypsy->longitude, 
			 gypsy->altitude, gypsy->accuracy);
	}
}

static void
connection_changed (GypsyDevice  *device,
		    gboolean      connected,
		    GeoclueGypsy *gypsy)
{
	if (connected == FALSE && 
	    gypsy->status != GEOCLUE_STATUS_UNAVAILABLE) {
		gypsy->status = GEOCLUE_STATUS_UNAVAILABLE;
		gc_iface_geoclue_emit_status_changed (GC_IFACE_GEOCLUE (gypsy),
						      gypsy->status);
	}
}

static void
fix_status_changed (GypsyDevice         *device,
		    GypsyDeviceFixStatus status,
		    GeoclueGypsy        *gypsy)
{
	gboolean changed = FALSE;

	switch (status) {
	case GYPSY_DEVICE_FIX_STATUS_INVALID:
		if (gypsy->status != GEOCLUE_STATUS_UNAVAILABLE) {
			changed = TRUE;
			gypsy->status = GEOCLUE_STATUS_UNAVAILABLE;
		}
		break;

	case GYPSY_DEVICE_FIX_STATUS_NONE:
		if (gypsy->status != GEOCLUE_STATUS_ACQUIRING) {
			changed = TRUE;
			gypsy->status = GEOCLUE_STATUS_ACQUIRING;
		}
		break;

	case GYPSY_DEVICE_FIX_STATUS_2D:
	case GYPSY_DEVICE_FIX_STATUS_3D:
		if (gypsy->status != GEOCLUE_STATUS_AVAILABLE) {
			changed = TRUE;
			gypsy->status = GEOCLUE_STATUS_AVAILABLE;
		}
		break;
	}

	if (changed) {
		gc_iface_geoclue_emit_status_changed (GC_IFACE_GEOCLUE (gypsy),
						      gypsy->status);
	}
}

static void
get_initial_status (GeoclueGypsy *gypsy)
{
	gboolean connected;
	GypsyDeviceFixStatus status;
	GError *error = NULL;

	connected = gypsy_device_get_connection_status (gypsy->device, &error);
	if (connected == FALSE) {
		gypsy->status = GEOCLUE_STATUS_UNAVAILABLE;
		g_print ("Initial status - %d (disconnected)\n", gypsy->status);
		return;
	}

	status = gypsy_device_get_fix_status (gypsy->device, &error);
	switch (status) {
	case GYPSY_DEVICE_FIX_STATUS_INVALID:
		gypsy->status = GEOCLUE_STATUS_UNAVAILABLE;
		break;

	case GYPSY_DEVICE_FIX_STATUS_NONE:
		gypsy->status = GEOCLUE_STATUS_ACQUIRING;
		break;

	case GYPSY_DEVICE_FIX_STATUS_2D:
	case GYPSY_DEVICE_FIX_STATUS_3D:
		gypsy->status = GEOCLUE_STATUS_AVAILABLE;
		break;
	}

	g_print ("Initial status - %d (connected)\n", gypsy->status);
}

static gboolean
set_options (GcIfaceGeoclue *gc,
             GHashTable     *options,
             GError        **error)
{
        GeoclueGypsy *gypsy = GEOCLUE_GYPSY (gc);
        gpointer device_name;
        char *path;

        device_name = g_hash_table_lookup (options, 
                                           "org.freedesktop.Geoclue.GPSDevice");
        if (device_name == NULL) {
                return TRUE;
        }

        gypsy->device_name = g_strdup (device_name);
        g_print ("Gypsy provider using %s\n", gypsy->device_name);
	path = gypsy_control_create (gypsy->control, gypsy->device_name,
				     error);
	if (*error != NULL) {
                g_print ("Error - %s?\n", (*error)->message);
                gypsy->status = GEOCLUE_STATUS_ERROR;
                return FALSE;
	}

        /* If we've got here, then we are out of the ERROR condition */
        gypsy->status = GEOCLUE_STATUS_UNAVAILABLE;

	gypsy->device = gypsy_device_new (path);
	g_signal_connect (gypsy->device, "connection-changed",
			  G_CALLBACK (connection_changed), gypsy);
	g_signal_connect (gypsy->device, "fix-status-changed",
			  G_CALLBACK (fix_status_changed), gypsy);
	
	gypsy->position = gypsy_position_new (path);
	g_signal_connect (gypsy->position, "position-changed",
			  G_CALLBACK (position_changed), gypsy);
	gypsy->course = gypsy_course_new (path);
	g_signal_connect (gypsy->course, "course-changed",
			  G_CALLBACK (course_changed), gypsy);
	gypsy->acc = gypsy_accuracy_new (path);
	g_signal_connect (gypsy->acc, "accuracy-changed",
			  G_CALLBACK (accuracy_changed), gypsy);
	
	g_debug ("starting device");
	gypsy_device_start (gypsy->device, error);
	if (*error != NULL) {
		g_print ("Error - %s?\n", (*error)->message);
		gypsy->status = GEOCLUE_STATUS_ERROR;
		g_free (path);
		return FALSE;
	}
	get_initial_status (gypsy);
	g_free (path);
	
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueGypsy *gypsy = GEOCLUE_GYPSY (provider);
	
	g_main_loop_quit (gypsy->loop);
}

static void
finalize (GObject *object)
{
	GeoclueGypsy *gypsy = GEOCLUE_GYPSY (object);

	geoclue_accuracy_free (gypsy->accuracy);
        g_free (gypsy->device_name);

	((GObjectClass *) geoclue_gypsy_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GeoclueGypsy *gypsy = GEOCLUE_GYPSY (object);

	if (gypsy->control) {
		g_object_unref (gypsy->control);
		gypsy->control = NULL;
	}

	if (gypsy->device) {
		g_object_unref (gypsy->device);
		gypsy->device = NULL;
	}

	if (gypsy->position) {
		g_object_unref (gypsy->position);
		gypsy->position = NULL;
	}

	((GObjectClass *) geoclue_gypsy_parent_class)->dispose (object);
}

static void
geoclue_gypsy_class_init (GeoclueGypsyClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	GcProviderClass *p_class = (GcProviderClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;

	p_class->get_status = get_status;
	p_class->set_options = set_options;
	p_class->shutdown = shutdown;
}

static void
geoclue_gypsy_init (GeoclueGypsy *gypsy)
{
	gypsy->status = GEOCLUE_STATUS_ERROR;
	gypsy->control = gypsy_control_get_default ();

	gc_provider_set_details (GC_PROVIDER (gypsy),
				 "org.freedesktop.Geoclue.Providers.Gypsy",
				 "/org/freedesktop/Geoclue/Providers/Gypsy",
				 "Gypsy", "Gypsy provider");

	gypsy->position_fields = GYPSY_POSITION_FIELDS_NONE;

	gypsy->accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
						0.0, 0.0);
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
	GeoclueGypsy *gypsy = GEOCLUE_GYPSY (gc);
	GeoclueAccuracyLevel level;
	double horizontal, vertical;
	
	*timestamp = gypsy->timestamp;

	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (gypsy->position_fields & GYPSY_POSITION_FIELDS_LATITUDE) {
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
		*latitude = gypsy->latitude;
	}
	if (gypsy->position_fields & GYPSY_POSITION_FIELDS_LONGITUDE) {
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
		*longitude = gypsy->longitude;
	}
	if (gypsy->position_fields & GYPSY_POSITION_FIELDS_ALTITUDE) {
		*fields |= GEOCLUE_POSITION_FIELDS_ALTITUDE;
		*altitude = gypsy->altitude;
	}

	geoclue_accuracy_get_details (gypsy->accuracy, &level,
				      &horizontal, &vertical);
	*accuracy = geoclue_accuracy_new (level, horizontal, vertical);
		
	return TRUE;
}

static void
geoclue_gypsy_position_init (GcIfacePositionClass *iface)
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
geoclue_gypsy_velocity_init (GcIfaceVelocityClass *iface)
{
	iface->get_velocity = get_velocity;
}

int
main (int    argc,
      char **argv)
{
	GeoclueGypsy *gypsy;

	g_type_init ();

	gypsy = g_object_new (GEOCLUE_TYPE_GYPSY, NULL);

	gypsy->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (gypsy->loop);

	/* Unref the object so that gypsy-daemon knows we've shutdown */
	g_object_unref (gypsy);
	return 0;
}
