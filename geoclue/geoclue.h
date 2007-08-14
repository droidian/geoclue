/* Geoclue - A DBus api and wrapper for geography information
 * Copyright (C) 2006-2007 by Garmin Ltd. or its subsidiaries
 * 
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation; 
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __ORG_FOINSE_PROJECT_GEOCLUE_GEOCLUE_H__
#define __ORG_FOINSE_PROJECT_GEOCLUE_GEOCLUE_H__


#include <geoclue/position.h>
#include <geoclue/geocode.h>
#include <geoclue/map.h>
#include <geoclue/find.h>
#include <config.h>

/** \mainpage
 *
 * \section intro_sec Introduction
 * 
 * GeoClue is a D-Bus API and library that provides all kinds of geographic information to applications. The information may originally come in various forms from many sources (backends), but GeoClue offers it to applications in a simple, abstracted form.
 * 
 * \section intro_sec Introduction
 * Geoclue exist to try and establish a geographic information standard api as to facilitate application development around this information.  
 * 
 * 
 *  
 * \section basics_sec Sections
 * 
 * These are the current apis offered by Geoclue
 * \li \ref Position (Beta)
 * \li Maps (Experimental)
 * \li Geocoding (Experimental)
 * \li Find (Experimental)

 * 
 * 
**/


DBusBusType
geoclue_get_dbus_bus_type ()
{
	return GEOCLUE_DBUS_BUS;
}




#endif