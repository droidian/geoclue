/*
 * Geoclue
 * geoclue-error.h - Error handling
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GEOCLUE_ERROR_H
#define _GEOCLUE_ERROR_H

#include <glib.h>
#include <geoclue/geoclue-types.h>

#define GEOCLUE_ERROR (geoclue_error_quark ())

GQuark geoclue_error_quark (void);

#endif
