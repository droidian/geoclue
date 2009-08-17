/*
 * Geoclue
 * geoclue-accuracy.h - Functions for manipulating GeoclueAccuracy structs
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

#ifndef _GEOCLUE_ACCURACY_H
#define _GEOCLUE_ACCURACY_H

#include <glib.h>
#include <geoclue/geoclue-types.h>

#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GEOCLUE_ACCURACY_TYPE (dbus_g_type_get_struct ("GValueArray", G_TYPE_INT, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID))

typedef GValueArray GeoclueAccuracy;

GeoclueAccuracy *geoclue_accuracy_new (GeoclueAccuracyLevel level,
				       double               horizontal_accuracy,
				       double               vertical_accuracy);
void geoclue_accuracy_free (GeoclueAccuracy *accuracy);
void geoclue_accuracy_get_details (GeoclueAccuracy      *accuracy,
				   GeoclueAccuracyLevel *level,
				   double               *horizontal_accuracy,
				   double               *vertical_accuracy);
void geoclue_accuracy_set_details (GeoclueAccuracy      *accuracy,
				   GeoclueAccuracyLevel  level,
				   double                horizontal_accuracy,
				   double                vertical_accuracy);

GeoclueAccuracy *geoclue_accuracy_copy (GeoclueAccuracy *accuracy);

G_END_DECLS

#endif
