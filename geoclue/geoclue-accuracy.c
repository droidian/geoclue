/*
 * Geoclue
 * geoclue-accuracy.c - Code for manipulating the GeoclueAccuracy structure
 * 
 * Author: Iain Holmes <iain@openedhand.com>
 */

/**
 * SECTION:geoclue-accuracy
 * @short_description: Methods for manipulating #GeoclueAccuracy structure
 * 
 * A #GeoclueAccuracy holds accuracy information: a 
 * #GeoclueAccuracyLevel and metric values for horizontal and vertical
 * accuracy. These values should be set and queried using provided 
 * functions.
 **/

#include <glib-object.h>

#include <geoclue/geoclue-accuracy.h>

/**
 * geoclue_accuracy_new:
 * @level: The #GeoclueAccuracyLevel
 * @horizontal_accuracy: The horizontal accuracy in meters
 * @vertical_accuracy: The vertical accuracy in meters
 *
 * Creates a new #GeoclueAccuracy with given values.
 * 
 * Return value: New #GeoclueAccuracy.
 */
GeoclueAccuracy *
geoclue_accuracy_new (GeoclueAccuracyLevel level,
		      double               horizontal_accuracy,
		      double               vertical_accuracy)
{
	GPtrArray *accuracy;
	GValue accuracy_struct = {0, };

	accuracy = g_ptr_array_new ();

	g_value_init (&accuracy_struct, GEOCLUE_ACCURACY_TYPE);
	g_value_take_boxed (&accuracy_struct,
			    dbus_g_type_specialized_construct
			    (GEOCLUE_ACCURACY_TYPE));

	dbus_g_type_struct_set (&accuracy_struct,
				0, level,
				1, horizontal_accuracy,
				2, vertical_accuracy,
				G_MAXUINT);
	g_ptr_array_add (accuracy, g_value_get_boxed (&accuracy_struct));
	
	return (GeoclueAccuracy *) accuracy;
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
	GPtrArray *arr = (GPtrArray *) accuracy;
	int i;
	
	for (i = 0; i < arr->len; i++) {
		g_boxed_free (GEOCLUE_ACCURACY_TYPE,
			      g_ptr_array_index (arr, i));
	}
	g_ptr_array_free (arr, TRUE);
}

/**
 * geoclue_accuracy_get_details:
 * @accuracy: A #GeoclueAccuracy
 * @level: The returned #GeoclueAccuracyLevel
 * @horizontal_accuracy: The returned horizontal accuracy in meters
 * @vertical_accuracy: The returned vertical accuracy in meters
 *
 */
void
geoclue_accuracy_get_details (GeoclueAccuracy      *accuracy,
			      GeoclueAccuracyLevel *level,
			      double               *horizontal_accuracy,
			      double               *vertical_accuracy)
{
	GPtrArray *arr = (GPtrArray *) accuracy;
	GValueArray *vals = arr->pdata[0];

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
