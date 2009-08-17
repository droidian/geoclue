/*
 * Geoclue
 * geoclue-types.c - 
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <geoclue/geoclue-marshal.h>
#include <geoclue/geoclue-types.h>
#include <geoclue/geoclue-accuracy.h>

void
geoclue_types_init (void)
{
	dbus_g_object_register_marshaller (geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE,
					   G_TYPE_NONE,
					   G_TYPE_INT,
					   G_TYPE_INT,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_INVALID);
        dbus_g_object_register_marshaller (geoclue_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE_BOXED,
                                           G_TYPE_NONE,
                                           G_TYPE_INT,
					   G_TYPE_INT,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
                                           G_TYPE_BOXED,
					   G_TYPE_INVALID);
	
	dbus_g_object_register_marshaller (geoclue_marshal_VOID__INT_BOXED_BOXED,
					   G_TYPE_NONE,
					   G_TYPE_INT,
					   G_TYPE_BOXED,
					   G_TYPE_BOXED,
					   G_TYPE_INVALID);
	
	dbus_g_object_register_marshaller (geoclue_marshal_VOID__STRING_STRING_STRING_STRING,
	                                   G_TYPE_NONE,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_INVALID);
}
