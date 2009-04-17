/*
 * Geoclue
 * geoclue-error.h - Error handling
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GEOCLUE_ERROR_H
#define _GEOCLUE_ERROR_H

#include <glib.h>
#include <geoclue/geoclue-types.h>

#define GEOCLUE_ERROR (geoclue_error_quark ())

GQuark geoclue_error_quark (void);

#endif
