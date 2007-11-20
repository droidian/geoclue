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
} GeoclueExample;

typedef struct {
	GcProviderClass parent_class;
} GeoclueExampleClass;

#define GEOCLUE_TYPE_EXAMPLE (geoclue_example_get_type ())

G_DEFINE_TYPE (GeoclueExample, geoclue_example, GC_TYPE_PROVIDER)

static void
geoclue_example_class_init (GeoclueExampleClass *klass)
{
}

static void
geoclue_example_init (GeoclueExample *example)
{
	gc_provider_set_details (GC_PROVIDER (example),
				 "org.freedesktop.Geoclue.Providers.Example",
				 "/org/freedesktop/Geoclue/Providers/Example");
}

int
main (int    argc,
      char **argv)
{
	GeoclueExample *example;
	GMainLoop *mainloop;

	g_type_init ();

	example = g_object_new (GEOCLUE_TYPE_EXAMPLE, NULL);

	mainloop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (mainloop);

	return 0;
}
