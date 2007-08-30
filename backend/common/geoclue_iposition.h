#ifndef GEOCLUE_IPOSITION_H
#define GEOCLUE_IPOSITION_H

#include <glib-object.h>

#define GEOCLUE_TYPE_IPOSITION                (geoclue_iposition_get_type ())
#define GEOCLUE_IPOSITION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_IPOSITION, GeoclueIPosition))
#define GEOCLUE_IS_IPOSITION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_IPOSITION))
#define GEOCLUE_IPOSITION_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GEOCLUE_TYPE_IPOSITION, GeoclueIPositionInterface))


typedef struct _GeoclueIPosition GeoclueIPosition; /* dummy object */
typedef struct _GeoclueIPositionInterface GeoclueIPositionInterface;

struct _GeoclueIPositionInterface {
  GTypeInterface parent;

  void (*do_action) (GeoclueIPosition *self);
};

GType geoclue_iposition_get_type (void);

/* Actual interface */

void geoclue_iposition_do_action (GeoclueIPosition *self);

#endif /*GEOCLUE_IPOSITION_H*/

