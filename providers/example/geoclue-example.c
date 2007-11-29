/*
 * Geoclue
 * geoclue-example.c - Example provider
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <config.h>

#include <geoclue/gc-provider.h>

typedef struct {
	GcProvider parent;

	GMainLoop *loop;
} GeoclueExample;

typedef struct {
	GcProviderClass parent_class;
} GeoclueExampleClass;

#define GEOCLUE_TYPE_EXAMPLE (geoclue_example_get_type ())
#define GEOCLUE_EXAMPLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_EXAMPLE, GeoclueExample))

G_DEFINE_TYPE (GeoclueExample, geoclue_example, GC_TYPE_PROVIDER)


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
	GeoclueExample *example = GEOCLUE_EXAMPLE (gc);

	g_main_loop_quit (example->loop);
	return TRUE;
}

static void
geoclue_example_class_init (GeoclueExampleClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *) klass;

	p_class->get_status = get_status;
	p_class->shutdown = shutdown;
}

static void
geoclue_example_init (GeoclueExample *example)
{
	gc_provider_set_details (GC_PROVIDER (example),
				 "org.freedesktop.Geoclue.Providers.Example",
				 "/org/freedesktop/Geoclue/Providers/Example",
				 "Example", "Example provider");
}

int
main (int    argc,
      char **argv)
{
	GeoclueExample *example;

	g_type_init ();

	example = g_object_new (GEOCLUE_TYPE_EXAMPLE, NULL);

	example->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (example->loop);

	return 0;
}
