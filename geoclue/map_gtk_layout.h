/* Geocluemap - A DBus api and wrapper for getting geography pictures
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
#ifndef __ORG_FREEDESKTOP_GEOCLUE_MAP_GTK_LAYOUT_H__
#define __ORG_FREEDESKTOP_GEOCLUE_MAP_GTK_LAYOUT_H__

#include <gtk/gtklayout.h>
#include <gtk/gtkeventbox.h>

G_BEGIN_DECLS

#define GEOCLUE_MAP_GTK_TYPE_LAYOUT            (geoclue_map_gtk_layout_get_type ())
#define GEOCLUE_MAP_GTK_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_MAP_GTK_TYPE_LAYOUT, GeocluemapGtkLayout))
#define GEOCLUE_MAP_GTK_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_MAP_GTK_TYPE_LAYOUT, GeocluemapGtkLayoutClass))
#define GEOCLUE_MAP_GTK_IS_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_MAP_GTK_TYPE_LAYOUT))
#define GEOCLUE_MAP_GTK_IS_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_MAP_GTK_TYPE_LAYOUT))
#define GEOCLUE_MAP_GTK_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCLUE_MAP_GTK_TYPE_LAYOUT, GeocluemapGtkLayoutClass))


typedef struct _GeocluemapGtkLayout        GeocluemapGtkLayout;
typedef struct _GeocluemapGtkLayoutClass   GeocluemapGtkLayoutClass;

struct _GeocluemapGtkLayout
{
    GtkLayout container;
    GList *geoclue_map_image_children;
    GList *widget_children;
  
    GtkWidget*  eventbox;

    /* private */
    gboolean pending;
    gboolean idle_queued;
    gdouble button_click_x;
    gdouble button_click_y;
    gint last_width;
    gint last_height;
};

struct _GeocluemapGtkLayoutClass
{
  GtkLayoutClass parent_class;

};

GType          geoclue_map_gtk_layout_get_type        (void) G_GNUC_CONST;
GtkWidget*     geoclue_map_gtk_layout_new             (GtkEventBox*  eventbox, gdouble latitude, gdouble longitude, gint zoom);
GtkWidget*     geoclue_map_gtk_layout_new_corners             (GtkEventBox*  eventbox, gdouble top_left_latitude, gdouble top_left_longitude, gdouble bottom_right_latitude, gdouble bottom_right_longitude);
gboolean       geoclue_map_gtk_layout_zoom_in(GeocluemapGtkLayout* geoclue_maplayout );
gboolean       geoclue_map_gtk_layout_zoom_out(GeocluemapGtkLayout* geoclue_maplayout);
gboolean       geoclue_map_gtk_layout_zoom(GeocluemapGtkLayout* geoclue_maplayout, gint zoom);
gboolean       geoclue_map_gtk_layout_lat_lon(GeocluemapGtkLayout* geoclue_maplayout, gdouble IN_latitude, gdouble IN_longitude);
gboolean       geoclue_map_gtk_layout_add_widget(GeocluemapGtkLayout* geoclue_maplayout, GtkWidget* widget, gdouble IN_latitude, gdouble IN_longitude);



G_END_DECLS

#endif /* __ORG_FREEDESKTOP_GEOCLUE_MAP_GTK_LAYOUT_H__ */
