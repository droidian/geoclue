#ifndef _GEOCLUE_ADDRESS_DETAILS_H
#define _GEOCLUE_ADDRESS_DETAILS_H

#include <glib.h>

GHashTable *
address_details_new ();

GHashTable *
address_details_copy (GHashTable *source);

#endif
