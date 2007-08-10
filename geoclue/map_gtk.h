/* GEOCLUE_MAP - A DBus api and wrapper for getting geography pictures
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
 * Boston, MA 02111-1307, USA.*/
#ifndef __ORG_FREEDESKTOP_GEOCLUE_MAP_GTK_H__
#define __ORG_FREEDESKTOP_GEOCLUE_MAP_GTK_H__

#include <geoclue/map.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


G_BEGIN_DECLS

 typedef void (*GEOCLUE_MAP_GTK_CALLBACK)(GdkPixbuf* map_buffer, void* userdata);

 

GEOCLUE_MAP_RETURNCODE geoclue_map_gtk_get_gdk_pixbuf (const gdouble IN_latitude, const gdouble IN_longitude, const gint IN_width, const gint IN_height, const gint IN_zoom, GEOCLUE_MAP_GTK_CALLBACK func, void* userdatain );


G_END_DECLS

#endif // __ORG_FREEDESKTOP_GEOCLUE_MAP_GTK_H__
