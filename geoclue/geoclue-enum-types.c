


#include <geoclue-error.h>
#include "geoclue-enum-types.h"
#include <glib-object.h>

/* enumerations from "geoclue-error.h" */
GType
geoclue_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GEOCLUE_ERROR_NOT_IMPLEMENTED, "GEOCLUE_ERROR_NOT_IMPLEMENTED", "not-implemented" },
      { GEOCLUE_ERROR_NOT_AVAILABLE, "GEOCLUE_ERROR_NOT_AVAILABLE", "not-available" },
      { GEOCLUE_ERROR_FAILED, "GEOCLUE_ERROR_FAILED", "failed" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GeoclueError", values);
  }
  return etype;
}



