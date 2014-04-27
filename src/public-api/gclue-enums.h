/* vim: set et ts=8 sw=8: */
/* gclue-enum.h
 *
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#ifndef GCLUE_ENUM_H
#define GCLUE_ENUM_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * GClueAccuracyLevel:
 * @GCLUE_ACCURACY_LEVEL_NONE: Accuracy level unknown or unset.
 * @GCLUE_ACCURACY_LEVEL_COUNTRY: Country-level accuracy.
 * @GCLUE_ACCURACY_LEVEL_CITY: City-level accuracy.
 * @GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD: neighborhood-level accuracy.
 * @GCLUE_ACCURACY_LEVEL_STREET: Street-level accuracy.
 * @GCLUE_ACCURACY_LEVEL_EXACT: Exact accuracy. Typically requires GPS receiver.
 *
 * Used to specify level of accuracy requested by, or allowed for a client.
 **/
typedef enum {/*< underscore_name=gclue_accuracy_level>*/
        GCLUE_ACCURACY_LEVEL_NONE = 0,
        GCLUE_ACCURACY_LEVEL_COUNTRY = 1,
        GCLUE_ACCURACY_LEVEL_CITY = 4,
        GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD = 5,
        GCLUE_ACCURACY_LEVEL_STREET = 6,
        GCLUE_ACCURACY_LEVEL_EXACT = 8,
} GClueAccuracyLevel;

G_END_DECLS

#endif /* GCLUE_ENUM_H */
