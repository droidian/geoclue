/*
 * Geoclue
 * geoclue-types.h - Types for Geoclue
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GEOCLUE_TYPES_H
#define _GEOCLUE_TYPES_H

typedef enum {
	GEOCLUE_ACCURACY_LEVEL_NONE,
	GEOCLUE_ACCURACY_LEVEL_COUNTRY,
	GEOCLUE_ACCURACY_LEVEL_REGION,
	GEOCLUE_ACCURACY_LEVEL_LOCALITY,
	GEOCLUE_ACCURACY_LEVEL_POSTALCODE,
	GEOCLUE_ACCURACY_LEVEL_STREET,
	GEOCLUE_ACCURACY_LEVEL_DETAILED,
} GeoclueAccuracyLevel;

typedef enum {
	GEOCLUE_POSITION_FIELDS_NONE = 0,
	GEOCLUE_POSITION_FIELDS_LATITUDE = 1 << 0,
	GEOCLUE_POSITION_FIELDS_LONGITUDE = 1 << 1,
	GEOCLUE_POSITION_FIELDS_ALTITUDE = 1 << 2
} GeocluePositionFields;

typedef enum {
	GEOCLUE_ERROR_NOT_IMPLEMENTED, /* method is not implemented (yet) */
	GEOCLUE_ERROR_NOT_AVAILABLE,   /* e.g. web service did not respond */
	GEOCLUE_ERROR_FAILED,          /* generic fatal error */
} GeoclueError;

#endif
