/*
 * Geoclue
 * geoclue-error.h - Error handling
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _GEOCLUE_ERROR_H
#define _GEOCLUE_ERROR_H

#include <glib-object.h>
#include <geoclue/geoclue-enum-types.h>

/**
 * GeoclueError:
 * @GEOCLUE_ERROR_NOT_IMPLEMENTED: Method is not implemented
 * @GEOCLUE_ERROR_NOT_AVAILABLE: Needed information is not currently
 * available (e.g. web service did not respond)
 * @GEOCLUE_ERROR_FAILED: Generic fatal error
 *
 * Error values for providers.
 **/
typedef enum {
	GEOCLUE_ERROR_NOT_IMPLEMENTED,
	GEOCLUE_ERROR_NOT_AVAILABLE,
	GEOCLUE_ERROR_FAILED,
} GeoclueError;

#define GEOCLUE_ERROR_DBUS_INTERFACE "org.freedesktop.Geoclue.Error"
#define GEOCLUE_ERROR (geoclue_error_quark ())

GQuark geoclue_error_quark (void);

#endif
