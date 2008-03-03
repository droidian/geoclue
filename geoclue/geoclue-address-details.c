/*
 * Geoclue
 * geoclue-address-details.c - Helper functions for GeoclueAddress
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

#include "geoclue-address-details.h"

static void 
copy_address_key_and_value (char *key, char *value, GHashTable *target)
{
	g_hash_table_insert (target, g_strdup (key), g_strdup (value));
}

GHashTable *
address_details_new ()
{
	return g_hash_table_new_full (g_str_hash, g_str_equal,
	                              g_free, g_free);
}
GHashTable *
address_details_copy (GHashTable *source)
{
	GHashTable *target;
	
	g_assert (source != NULL);
	
	target = address_details_new ();
	g_hash_table_foreach (source, 
	                      (GHFunc)copy_address_key_and_value, 
	                      target);
	return target;
}
