/*
 * Geoclue
 * geoclue-error.c - Error handling
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#include <geoclue/geoclue-error.h>

GQuark 
geoclue_error_quark (void)
{
	static GQuark quark = 0;

	if (quark == 0) {
		quark = g_quark_from_static_string ("geoclue-error-quark");
	}

	return quark;
}
