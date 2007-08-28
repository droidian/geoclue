#include <glib.h>
#include "geoclue_position_error.h"

/* Error domain for position backend errors */
GQuark geoclue_position_error_quark () {
    static GQuark q = 0;
    if (q == 0)
        q = g_quark_from_static_string ("geoclue-position-error-quark");
    return q;
}
