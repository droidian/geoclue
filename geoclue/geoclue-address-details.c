/*
 * Geoclue
 * geoclue-address-details.c - Helper functions for GeoclueAddress
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
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

/**
 * SECTION:geoclue-address-details
 * @short_description: Convenience functions for handling Geoclue address
 * #GHashTables
 */
#include <geoclue/geoclue-types.h>
#include "geoclue-address-details.h"

static void 
copy_address_key_and_value (char *key, char *value, GHashTable *target)
{
	g_hash_table_insert (target, g_strdup (key), g_strdup (value));
}


/**
 * geoclue_address_details_new:
 * 
 * Creates a new #GHashTable suitable for Geoclue Address details.
 * Both keys and values inserted to this #GHashTable will be freed
 * on g_hash_table_destroy().
 * 
 * Return value: New #GHashTable
 */
GHashTable *
geoclue_address_details_new ()
{
	return g_hash_table_new_full (g_str_hash, g_str_equal,
	                              g_free, g_free);
}

/**
 * geoclue_address_details_copy:
 * @source: #GHashTable to copy
 * 
 * Deep-copies a #GHashTable.
 * 
 * Return value: New, deep copied #GHashTable
 */
GHashTable *
geoclue_address_details_copy (GHashTable *source)
{
	GHashTable *target;
	
	g_assert (source != NULL);
	
	target = geoclue_address_details_new ();
	g_hash_table_foreach (source, 
	                      (GHFunc)copy_address_key_and_value, 
	                      target);
	return target;
}

/**
 * geoclue_address_details_get_accuracy_level:
 * @address: A #GHashTable with address hash values
 * 
 * Returns a #GeoclueAccuracy that best describes the accuracy of @address
 * 
 * Return value: #GeoclueAccuracy
 */
GeoclueAccuracyLevel
geoclue_address_details_get_accuracy_level (GHashTable *address)
{
	if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_STREET)) {
		return GEOCLUE_ACCURACY_LEVEL_STREET;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY)) {
		return GEOCLUE_ACCURACY_LEVEL_LOCALITY;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_REGION)) {
		return GEOCLUE_ACCURACY_LEVEL_REGION;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRY) ||
	           g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	}
	return GEOCLUE_ACCURACY_LEVEL_NONE;
}
