/*
 * Geoclue
 * geoclue-accuracy.c - Code for manipulating the GeoclueAccuracy structure
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

/**
 * SECTION:geoclue-accuracy
 * @short_description: Methods for manipulating #GeoclueAccuracy structure
 * 
 * A #GeoclueAccuracy holds accuracy information: a 
 * #GeoclueAccuracyLevel and metric values for horizontal and vertical
 * accuracy. The last two will only be defined if #GeoclueAccuracyLevel is
 * %GEOCLUE_ACCURACY_LEVEL_DETAILED. These values should be set and queried 
 * using provided functions.
 **/

#include <glib-object.h>

#include <geoclue/geoclue-accuracy.h>

/**
 * geoclue_accuracy_new:
 * @level: A #GeoclueAccuracyLevel
 * @horizontal_accuracy: Horizontal accuracy in meters
 * @vertical_accuracy: Vertical accuracy in meters
 *
 * Creates a new #GeoclueAccuracy with given values. Use 0 for 
 * horizontal_accuracy and vertical_accuracy if @level is not
 * %GEOCLUE_ACCURACY_LEVEL_DETAILED.
 * 
 * Return value: New #GeoclueAccuracy.
 */
GeoclueAccuracy *
geoclue_accuracy_new (GeoclueAccuracyLevel level,
		      double               horizontal_accuracy,
		      double               vertical_accuracy)
{
	GValue accuracy_struct = {0, };

	g_value_init (&accuracy_struct, GEOCLUE_ACCURACY_TYPE);
	g_value_take_boxed (&accuracy_struct,
			    dbus_g_type_specialized_construct
			    (GEOCLUE_ACCURACY_TYPE));

	dbus_g_type_struct_set (&accuracy_struct,
				0, level,
				1, horizontal_accuracy,
				2, vertical_accuracy,
				G_MAXUINT);
	
	return (GeoclueAccuracy *) g_value_get_boxed (&accuracy_struct);
}

/**
 * geoclue_accuracy_free:
 * @accuracy: A #GeoclueAccuracy
 *
 * Frees the #GeoclueAccuracy.
 */
void
geoclue_accuracy_free (GeoclueAccuracy *accuracy)
{
	if (!accuracy) {
		return;
	}
	
        g_boxed_free (GEOCLUE_ACCURACY_TYPE, accuracy);
}

/**
 * geoclue_accuracy_get_details:
 * @accuracy: A #GeoclueAccuracy
 * @level: Pointer to returned #GeoclueAccuracyLevel or %NULL
 * @horizontal_accuracy: Pointer to returned horizontal accuracy in meters or %NULL
 * @vertical_accuracy: Pointer to returned vertical accuracy in meters or %NULL
 * 
 * @horizontal_accuracy and @vertical_accuracy will only be defined 
 * if @level is %GEOCLUE_ACCURACY_LEVEL_DETAILED.
 */
void
geoclue_accuracy_get_details (GeoclueAccuracy      *accuracy,
			      GeoclueAccuracyLevel *level,
			      double               *horizontal_accuracy,
			      double               *vertical_accuracy)
{
	GValueArray *vals;
	
	vals = accuracy;
	if (level != NULL) {
		*level = g_value_get_int (g_value_array_get_nth (vals, 0));
	}
	if (horizontal_accuracy != NULL) {
		*horizontal_accuracy = g_value_get_double (g_value_array_get_nth (vals, 1));
	}
	if (vertical_accuracy != NULL) {
		*vertical_accuracy = g_value_get_double (g_value_array_get_nth (vals, 2));
	}
}

/**
 * geoclue_accuracy_set_details:
 * @accuracy: A #GeoclueAccuracy
 * @level: A #GeoclueAccuracyLevel
 * @horizontal_accuracy: Horizontal accuracy in meters
 * @vertical_accuracy: Vertical accuracy in meters
 *
 * Replaces @accuracy values with given ones. 
 */
void
geoclue_accuracy_set_details (GeoclueAccuracy     *accuracy,
			      GeoclueAccuracyLevel level,
			      double               horizontal_accuracy,
			      double               vertical_accuracy)
{
	GValueArray *vals = accuracy;

	g_value_set_int (g_value_array_get_nth (vals, 0), level);
	g_value_set_double (g_value_array_get_nth (vals, 1), 
			    horizontal_accuracy);
	g_value_set_double (g_value_array_get_nth (vals, 2),
			    vertical_accuracy);
}

/**
 * geoclue_accuracy_copy:
 * @accuracy: A #GeoclueAccuracy
 *
 * Creates a copy of @accuracy.
 *
 * Return value: A newly allocated #GeoclueAccuracy
 */
GeoclueAccuracy *
geoclue_accuracy_copy (GeoclueAccuracy *accuracy)
{
	GeoclueAccuracyLevel level;
	double hor, ver;
	
	geoclue_accuracy_get_details (accuracy, &level, &hor, &ver);
	return geoclue_accuracy_new (level, hor, ver);
}
