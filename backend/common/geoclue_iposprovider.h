#ifndef GEOCLUE_IPOSPROVIDER_H
#define GEOCLUE_IPOSPROVIDER_H

#include <glib-object.h>

#define GEOCLUE_TYPE_IPOSPROVIDER                (geoclue_iposprovider_get_type ())
#define GEOCLUE_IPOSPROVIDER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_IPOSPROVIDER, GeoclueIPosprovider))
#define GEOCLUE_IS_IPOSPROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_IPOSPROVIDER))
#define GEOCLUE_IPOSPROVIDER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GEOCLUE_TYPE_IPOSPROVIDER, GeoclueIPosproviderInterface))


typedef struct _GeoclueIPosprovider GeoclueIPosprovider; /* dummy object */
typedef struct _GeoclueIPosproviderInterface GeoclueIPosproviderInterface;

struct _GeoclueIPosproviderInterface {
  GTypeInterface parent;

  void (*do_action) (GeoclueIPosprovider *self);
};

GType geoclue_iposprovider_get_type (void);

/* Interface API */

void geoclue_iposprovider_do_action (GeoclueIPosprovider *self);

#endif /*GEOCLUE_IPOSPROVIDER_H*/

