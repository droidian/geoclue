/*
 * Geoclue
 * geoclue-address-details.h - 
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
#ifndef _GEOCLUE_ADDRESS_DETAILS_H
#define _GEOCLUE_ADDRESS_DETAILS_H

#include <glib.h>

GHashTable *geoclue_address_details_new ();

GHashTable *geoclue_address_details_copy (GHashTable *source);

void geoclue_address_details_insert (GHashTable *address, const char *key, const char *value);

void geoclue_address_details_set_country_from_code (GHashTable *address);

GeoclueAccuracyLevel geoclue_address_details_get_accuracy_level (GHashTable *address);

#endif
