#ifndef _GEOCLUE_GSMLOC_OFONO
#define _GEOCLUE_GSMLOC_OFONO

#include <glib-object.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_GSMLOC_OFONO geoclue_gsmloc_ofono_get_type()

#define GEOCLUE_GSMLOC_OFONO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GSMLOC_OFONO, GeoclueGsmlocOfono))

#define GEOCLUE_GSMLOC_OFONO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_TYPE_GSMLOC_OFONO, GeoclueGsmlocOfonoClass))

#define GEOCLUE_IS_GSMLOC_OFONO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_GSMLOC_OFONO))

#define GEOCLUE_IS_GSMLOC_OFONO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_TYPE_GSMLOC_OFONO))

#define GEOCLUE_GSMLOC_OFONO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCLUE_TYPE_GSMLOC_OFONO, GeoclueGsmlocOfonoClass))

typedef struct {
  GObject parent;
} GeoclueGsmlocOfono;

typedef struct {
  GObjectClass parent_class;

  void (*network_data_changed) (GeoclueGsmlocOfono *ofono,
                                char *mcc, char *mnc,
                                char *lac, char *cid);
} GeoclueGsmlocOfonoClass;

GType geoclue_gsmloc_ofono_get_type (void);

GeoclueGsmlocOfono* geoclue_gsmloc_ofono_new (void);

G_END_DECLS

#endif
