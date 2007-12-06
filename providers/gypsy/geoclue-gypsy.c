/*
 * Geoclue
 * geoclue-gypsy.c - Geoclue backend for Gypsy
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 */

#include <config.h>

#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-course.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-position.h>
#include <geoclue/gc-iface-velocity.h>

typedef struct {
	GcProvider parent;
	
	GypsyControl *control;
	GypsyDevice *device;
	GypsyPosition *position;
	GypsyCourse *course;

	GMainLoop *loop;

	int timestamp;

	/* Cached so we don't have to make D-Bus method calls all the time */
	GypsyPositionFields position_fields;
	double latitude;
	double longitude;
	double altitude;

	GypsyCourseFields course_fields;
	double speed;
	double direction;
	double climb;
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
	GeoclueGypsy *gypsy = GEOCLUE_GYPSY (gc);

        g_main_loop_quit (gypsy->loop);
        return TRUE;
}

static void
finalize (GObject *object)
{
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
	p_class->shutdown = shutdown;
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
		GeoclueAccuracy *accuracy;
		GeocluePositionFields fields;

		/* FIXME: get the real accuracy */
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
						 0.0, 0.0);

		fields = gypsy_position_to_geoclue (gypsy->position_fields);
		gc_iface_position_emit_position_changed 
			(GC_IFACE_POSITION (gypsy), fields,
			 timestamp, gypsy->latitude, gypsy->longitude, 
			 gypsy->altitude, accuracy);
		geoclue_accuracy_free (accuracy);
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
geoclue_gypsy_init (GeoclueGypsy *gypsy)
{
	GError *error = NULL;
	char *path;

	gypsy->control = gypsy_control_get_default ();

	/* FIXME: Need a properties interface to get the GPS to use */
	path = gypsy_control_create (gypsy->control, "00:02:76:C4:27:A8",
				     &error);
	if (error != NULL) {
		/* Need to return an error somehow */
	}
	
	gypsy->device = gypsy_device_new (path);
	gypsy->position = gypsy_position_new (path);
	g_signal_connect (gypsy->position, "position-changed",
			  G_CALLBACK (position_changed), gypsy);
	gypsy->course = gypsy_course_new (path);
	g_signal_connect (gypsy->course, "course-changed",
			  G_CALLBACK (course_changed), gypsy);

	g_free (path);

	gc_provider_set_details (GC_PROVIDER (gypsy),
				 "org.freedesktop.Geoclue.Providers.Gypsy",
				 "/org/freedesktop/Geoclue/Providers/Gypsy",
				 "Gypsy", "Gypsy provider");

	gypsy->position_fields = GYPSY_POSITION_FIELDS_NONE;
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

	/* FIXME: get the real accuracy */
	*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
					  0.0, 0.0);
		
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
