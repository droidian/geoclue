#include "geoclue_iposition.h"

static void
geoclue_iposition_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
geoclue_iposition_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GeoclueIPositionInterface),
			geoclue_iposition_base_init, /* base_init */
			NULL,                          /* base_finalize */
			NULL,                          /* class_init */
			NULL,                          /* class_finalize */
			NULL,                          /* class_data */
			0,
			0,                             /* n_preallocs */
			NULL                           /* instance_init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE, 
		                               "GeoclueIPosition", 
		                               &info, 0);
	}
	return type;
}

void geoclue_iposition_do_action (GeoclueIPosition *self)
{
	GEOCLUE_IPOSITION_GET_INTERFACE (self)->do_action (self);
}
