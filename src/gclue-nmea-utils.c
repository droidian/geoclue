/* vim: set et ts=8 sw=8: */
/*
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>
#include "gclue-nmea-utils.h"

/**
 * gclue_nmea_type_is:
 * @msg: NMEA sentence
 * @nmeatype: A three character NMEA sentence type string ("GGA", "RMC" etc.)
 *
 * Returns: whether given NMEA sentence is of the given type
 **/
gboolean
gclue_nmea_type_is (const char *msg, const char *nmeatype)
{
        g_assert (strnlen (nmeatype, 4) < 4);

        return strnlen (msg, 7) > 6 &&
                g_str_has_prefix (msg, "$") &&
                g_str_has_prefix (msg+3, nmeatype);
}

