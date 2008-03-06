/*
 * Geoclue
 * geoclue-address-details.c - Helper functions for GeoclueAddress
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-address-details
 * @short_description: Convenience functions for handling Geoclue address
 * #GHashTables
 */
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
 * 
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
