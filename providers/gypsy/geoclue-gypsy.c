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

typedef struct {
	GcProvider parent;
	
	GypsyControl *control;
	GypsyDevice *device;
	GypsyPosition *position;
	GypsyCourse *course;

	GMainLoop *loop;

	/* Cached so we don't have to make D-Bus method calls all the time */
	GypsyPositionFields position_fields;
	double latitude;
	double longitude;
	double altitude;
} GeoclueGypsy;

typedef struct {
	GcProviderClass parent_class;
} GeoclueGypsyClass;

static void geoclue_gypsy_position_init (GcIfacePositionClass *iface);

#define GEOCLUE_TYPE_GYPSY (geoclue_gypsy_get_type ())
#define GEOCLUE_GYPSY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GYPSY, GeoclueGypsy))

G_DEFINE_TYPE_WITH_CODE (GeoclueGypsy, geoclue_gypsy, GC_TYPE_PROVIDER,
			 G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
						geoclue_gypsy_position_init))

/* GcIfaceGeoclue methods */
static gboolean
get_version (GcIfaceGeoclue *gc,
             int            *major,
             int            *minor,
             int            *micro,
             GError        **error)
{
        *major = 1;
        *minor = 0;
        *micro = 0;

        return TRUE;
}

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

	p_class->get_version = get_version;
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
	return ((fields_a & field) == (fields_b & field));
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

	if (compare_field (gypsy->position_fields, gypsy->latitude,
			   fields, latitude, GYPSY_POSITION_FIELDS_LATITUDE)) {
		gypsy->position_fields |= GYPSY_POSITION_FIELDS_LATITUDE;
		gypsy->latitude = latitude;
		changed = TRUE;
	}

	if (compare_field (gypsy->position_fields, gypsy->longitude,
			   fields, longitude, GYPSY_POSITION_FIELDS_LONGITUDE)) {
		gypsy->position_fields |= GYPSY_POSITION_FIELDS_LONGITUDE;
		gypsy->longitude = longitude;
		changed = TRUE;
	}

	if (compare_field (gypsy->position_fields, gypsy->altitude,
			   fields, altitude, GYPSY_POSITION_FIELDS_ALTITUDE)) {
		gypsy->position_fields |= GYPSY_POSITION_FIELDS_ALTITUDE;
		gypsy->altitude = altitude;
		changed = TRUE;
	}

	if (changed) {
		GeoclueAccuracy *accuracy;
		
		/* FIXME: get the real accuracy */
		accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
						 0.0, 0.0);
		gc_iface_position_emit_position_changed 
			(GC_IFACE_POSITION (gypsy), gypsy->position_fields,
			 timestamp, gypsy->latitude, gypsy->longitude, 
			 gypsy->altitude, accuracy);
		geoclue_accuracy_free (accuracy);
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

	g_free (path);

	gc_provider_set_details (GC_PROVIDER (gypsy),
				 "org.freedesktop.Geoclue.Providers.Gypsy",
				 "/org/freedesktop/Geoclue/Providers/Gypsy");

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
	return TRUE;
}

static void
geoclue_gypsy_position_init (GcIfacePositionClass *iface)
{
	iface->get_position = get_position;
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
