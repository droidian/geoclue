/*
 * Geoclue
 * geoclue-types.h - Types for Geoclue
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GEOCLUE_TYPES_H
#define _GEOCLUE_TYPES_H

/**
 * GeoclueAccuracyLevel:
 *
 * Enum values used to define the approximate accuracy of 
 * Position or Address information.
 **/
typedef enum {
	GEOCLUE_ACCURACY_LEVEL_NONE,
	GEOCLUE_ACCURACY_LEVEL_COUNTRY,
	GEOCLUE_ACCURACY_LEVEL_REGION,
	GEOCLUE_ACCURACY_LEVEL_LOCALITY,
	GEOCLUE_ACCURACY_LEVEL_POSTALCODE,
	GEOCLUE_ACCURACY_LEVEL_STREET,
	GEOCLUE_ACCURACY_LEVEL_DETAILED,
} GeoclueAccuracyLevel;

/**
 * GeocluePositionFields:
 *
 * #GeocluePositionFields can be used as a bitfield that defines
 * the validity of Position values.
 * 
 * Example:
 * <informalexample>
 * <programlisting>
 * GeocluePositionFields fields;
 * fields = geoclue_position_get_position (. . .);
 * 
 * if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
 *     fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
 * 	g_print("latitude and longitude are valid");
 * }
 * </programlisting>
 * </informalexample>
 **/
typedef enum {
	GEOCLUE_POSITION_FIELDS_NONE = 0,
	GEOCLUE_POSITION_FIELDS_LATITUDE = 1 << 0,
	GEOCLUE_POSITION_FIELDS_LONGITUDE = 1 << 1,
	GEOCLUE_POSITION_FIELDS_ALTITUDE = 1 << 2
} GeocluePositionFields;

/**
 * GeoclueVelocityFields:
 *
 * GeoclueVelocityFields values can be used as a bitfield that defines 
 * the validity of Velocity values.
 **/
typedef enum {
	GEOCLUE_VELOCITY_FIELDS_NONE = 0,
	GEOCLUE_VELOCITY_FIELDS_SPEED = 1 << 0,
	GEOCLUE_VELOCITY_FIELDS_DIRECTION = 1 << 1,
	GEOCLUE_VELOCITY_FIELDS_CLIMB = 1 << 2
} GeoclueVelocityFields;

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

/* Address keys from XEP-0080: http://www.xmpp.org/extensions/xep-0080.html
 * region    = province, US state
 * locality  = city, town 
 * area      = neighborhood, campus
 * street    = street name or full street address
 **/

/**
 * GEOCLUE_ADDRESS_KEY_COUNTRYCODE:
 * 
 * A key for address hashtables. The value should be a ISO 3166 two 
 * letter country code. 
 */
#define GEOCLUE_ADDRESS_KEY_COUNTRYCODE "countrycode"
/**
 * GEOCLUE_ADDRESS_KEY_COUNTRY:
 * 
 * A key for address hashtables. The value should be a name of a country. 
 */
#define GEOCLUE_ADDRESS_KEY_COUNTRY "country"
/**
 * GEOCLUE_ADDRESS_KEY_REGION:
 * 
 * A key for address hashtables. The value should be a name of an 
 * administrative region of a nation, e.g. province or
 * US state. 
 */
#define GEOCLUE_ADDRESS_KEY_REGION "region" 
/**
 * GEOCLUE_ADDRESS_KEY_LOCALITY:
 * 
 * A key for address hashtables. The value should be a name of a town 
 * or city. 
 */
#define GEOCLUE_ADDRESS_KEY_LOCALITY "locality"
/**
 * GEOCLUE_ADDRESS_KEY_AREA:
 * 
 * A key for address hashtables. The value should be a name of an 
 * area, such as neighborhood or campus. 
 */
#define GEOCLUE_ADDRESS_KEY_AREA "area"
/**
 * GEOCLUE_ADDRESS_KEY_POSTALCODE:
 * 
 * A key for address hashtables. The value should be a code used for 
 * postal delivery.
 */
#define GEOCLUE_ADDRESS_KEY_POSTALCODE "postalcode"
/**
 * GEOCLUE_ADDRESS_KEY_STREET:
 * 
 * A key for address hashtables. The value should be a full street 
 * address.
 */
#define GEOCLUE_ADDRESS_KEY_STREET "street"

void geoclue_types_init (void);

#endif
