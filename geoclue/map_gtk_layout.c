/* Geocluemap - A DBus api and wrapper for getting geography pictures
 * Copyright (C) 2006 Garmin
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
 
#include "map.h"
#include "map_gtk.h"
#include "map_gtk_layout.h"
#include "gtk/gtkprivate.h"
#include "gtk/gtkimage.h"
#include "gtk/gtkeventbox.h"
#include "gtk/gtkcontainer.h"




#define DEFAULT_HEIGHT 4000
#define DEFAULT_WIDTH 4000
#define DEFAULT_X 2000
#define DEFAULT_Y 2000


typedef struct _GeocluemapGtkLayoutChild GeocluemapGtkLayoutChild;

struct _GeocluemapGtkLayoutChild {
  GtkWidget *widget;
  GeocluemapGtkLayout *parent;
  gint x;
  gint y;
  gdouble latitude;
  gdouble longitude;
  gint zoom;
  
};

enum {
   PROP_0,
   PROP_LATITUDE,
   PROP_LONGITUDE,
   PROP_ZOOM,
   PROP_EVENTBOX
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_LATITUDE,
  CHILD_PROP_LONGITUDE,
  CHILD_PROP_ZOOM
};

static void geoclue_map_gtk_layout_class_init(  GeocluemapGtkLayoutClass *class);
static void geoclue_map_gtk_layout_get_property(GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void geoclue_map_gtk_layout_set_property(GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
                                           
static void geoclue_map_gtk_layout_init        (GeocluemapGtkLayout      *layout);
static void geoclue_map_gtk_layout_finalize    (GObject        *object);
static void geoclue_map_gtk_layout_size_allocate(GtkWidget      *widget,
                                           GtkAllocation  *allocation);
static gboolean 
geoclue_map_gtk_layout_button_press(        GtkWidget      *widget, 
                                       GdkEventButton *event,
                                       gpointer userdata );
static gboolean 
geoclue_map_gtk_layout_motion(              GtkWidget      *widget,
                                       GdkEventMotion *event,
                                       gpointer userdata) ;
static gboolean 
geoclue_map_gtk_layout_button_realease(     GtkWidget      *widget, 
                                       GdkEventButton *event,
                                       gpointer userdata );


static void geoclue_map_clear_map_images(GeocluemapGtkLayout *geoclue_maplayout);
static void geoclue_map_clear_widgets(GeocluemapGtkLayout *geoclue_maplayout);

static GtkWidgetClass *parent_class = NULL;


 
GtkWidget*     geoclue_map_gtk_layout_new( GtkEventBox*  eventbox,gdouble latitude, gdouble longitude, gint zoom)
{
    GeocluemapGtkLayout *layout;
    GtkAdjustment* adjustmentx =   GTK_ADJUSTMENT(gtk_adjustment_new(DEFAULT_X, 0,DEFAULT_WIDTH, 2,500 ,20));
    GtkAdjustment* adjustmenty =   GTK_ADJUSTMENT(gtk_adjustment_new(DEFAULT_Y, 0,DEFAULT_HEIGHT, 2,500 ,20)); 

    layout = g_object_new (GEOCLUE_MAP_GTK_TYPE_LAYOUT,
			 "hadjustment", adjustmentx,
			 "vadjustment", adjustmenty,
             "latitude", latitude,
             "longitude", longitude,
             "zoom", zoom,
             "eventbox", eventbox,
			 NULL);
 
    gtk_layout_set_size (GTK_LAYOUT(layout), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_adjustment_set_value        (adjustmentx,DEFAULT_X);
    gtk_adjustment_set_value        (adjustmenty,DEFAULT_Y);  
    return GTK_WIDGET (layout);
}

GtkWidget* geoclue_map_gtk_layout_new_corners(GtkEventBox*  eventbox, gdouble top_left_latitude, gdouble top_left_longitude, gdouble bottom_right_latitude, gdouble bottom_right_longitude)
{
    
    //Here's a guess for 640 x 480 now!!!
    int zoom;
    double center_lat = top_left_latitude - bottom_right_latitude;
    double center_lon = top_left_longitude - bottom_right_longitude;
   geoclue_map_find_zoom_level (top_left_latitude, top_left_longitude, bottom_right_latitude, bottom_right_longitude, 640, 480,&zoom);
 
 
    
    GeocluemapGtkLayout *layout;
    GtkAdjustment* adjustmentx =   GTK_ADJUSTMENT(gtk_adjustment_new(DEFAULT_X, 0,DEFAULT_WIDTH, 2,500 ,20));
    GtkAdjustment* adjustmenty =   GTK_ADJUSTMENT(gtk_adjustment_new(DEFAULT_Y, 0,DEFAULT_HEIGHT, 2,500 ,20)); 

    layout = g_object_new (GEOCLUE_MAP_GTK_TYPE_LAYOUT,
             "hadjustment", adjustmentx,
             "vadjustment", adjustmenty,
             "latitude", center_lat,
             "longitude", center_lon,
             "zoom", zoom,
             "eventbox", eventbox,
             NULL);
 
    gtk_layout_set_size (GTK_LAYOUT(layout), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_adjustment_set_value        (adjustmentx,DEFAULT_X);
    gtk_adjustment_set_value        (adjustmenty,DEFAULT_Y);  
    return GTK_WIDGET (layout);
  
}





static void
geoclue_map_gtk_layout_finalize (GObject *object)
{
    GeocluemapGtkLayout *layout = GEOCLUE_MAP_GTK_LAYOUT (object);
    
    geoclue_map_clear_map_images(layout);
    geoclue_map_clear_widgets(layout);
    
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

GType
geoclue_map_gtk_layout_get_type (void)
{
  static GType geoclue_map_layout_type = 0;

  if (!geoclue_map_layout_type)
    {
        static const GTypeInfo geoclue_map_layout_info =
        {
        	sizeof (GeocluemapGtkLayoutClass),
        	NULL,		/* base_init */
        	NULL,		/* base_finalize */
        	(GClassInitFunc) geoclue_map_gtk_layout_class_init,
        	NULL,		/* class_finalize */
        	NULL,		/* class_data */
        	sizeof (GeocluemapGtkLayout),
        	0,		/* n_preallocs */
        	(GInstanceInitFunc) geoclue_map_gtk_layout_init,
        };

        geoclue_map_layout_type = g_type_register_static (GTK_TYPE_LAYOUT, "GeocluemapGtkLayout",
					    &geoclue_map_layout_info, 0);
    }

  return geoclue_map_layout_type;
}

static void
geoclue_map_gtk_layout_class_init (GeocluemapGtkLayoutClass *class)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GtkLayoutClass *layout_class;

    gobject_class = (GObjectClass*) class;
    widget_class = (GtkWidgetClass*) class;
    container_class = (GtkContainerClass*) class;
    layout_class  = (GtkLayoutClass*) class;
  
    parent_class = g_type_class_peek_parent (class);

    gobject_class->set_property = geoclue_map_gtk_layout_set_property;
    gobject_class->get_property = geoclue_map_gtk_layout_get_property;
    widget_class->size_allocate = geoclue_map_gtk_layout_size_allocate; 

    g_object_class_install_property (gobject_class,
				   PROP_LATITUDE,
				   g_param_spec_double ("latitude",
							"Selected Latitude",
							"The Selected Latitude",
                            -180.0,
                            180.0,
                            0,
							GTK_PARAM_READWRITE));
  
    g_object_class_install_property (gobject_class,
				   PROP_LONGITUDE,
				   g_param_spec_double ("longitude",
							"Selected longitude",
							"The Selected longitude",
                            -180.0,
                            180.0,
                            0,
							GTK_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
				   PROP_ZOOM,
				   g_param_spec_int ("zoom",
                                    "zoom",
                                    "zoom of the map",
                                    1, //minimum
                                    G_MAXINT,
                                    1,
						     GTK_PARAM_READWRITE));
    g_object_class_install_property (gobject_class,
                   PROP_EVENTBOX,
                   g_param_spec_object ("eventbox",
                                    "eventbox",
                                    "eventbox to capture events",
                                    GTK_TYPE_EVENT_BOX,
                             GTK_PARAM_READWRITE));


}



static void
geoclue_map_gtk_layout_get_property (GObject     *object,
			 guint        prop_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
    
    GeocluemapGtkLayout *layout = GEOCLUE_MAP_GTK_LAYOUT (object);
 
    //Reference image is always the first on the list
    GeocluemapGtkLayoutChild* reference_image = ( GeocluemapGtkLayoutChild*)g_list_nth_data(layout->geoclue_map_image_children,0);

    if(reference_image != NULL)
    {
      switch (prop_id)
        {
        case PROP_LATITUDE:
            g_value_set_double (value, reference_image->latitude);   
            break;
        case PROP_LONGITUDE:
            g_value_set_double (value, reference_image->longitude);  
            break;
        case PROP_ZOOM:
            g_value_set_int (value, reference_image->zoom);
   
            break;
        case PROP_EVENTBOX:
            g_value_set_object (value, layout->eventbox);   
            break;
    
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
        }   
    }   
}


void geoclue_map_gtk_layout_update(GeocluemapGtkLayout* geoclue_maplayout, GeocluemapGtkLayoutChild* new_image);

gboolean geoclue_mapserver_idle_grab_map(gpointer data)
{
    GeocluemapGtkLayout* geoclue_maplayout = GEOCLUE_MAP_GTK_LAYOUT (data);
    GeocluemapGtkLayoutChild* reference_image = ( GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->geoclue_map_image_children, 0);
    if(reference_image != NULL)
    {
        geoclue_map_gtk_layout_update(geoclue_maplayout, reference_image);
    }
    geoclue_maplayout->idle_queued = FALSE;
    return FALSE;   
}

static void geoclue_map_clear_map_images(GeocluemapGtkLayout *geoclue_maplayout)
{
    GeocluemapGtkLayoutChild* image;  
    image = ( GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->geoclue_map_image_children,0);
   
    while(image != NULL)
    {
        geoclue_maplayout->geoclue_map_image_children = g_list_remove(geoclue_maplayout->geoclue_map_image_children, image);
        if(image->widget != NULL)
        {
            gtk_container_remove(GTK_CONTAINER(geoclue_maplayout), image->widget);
        }
        g_free(image);
        image = ( GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->geoclue_map_image_children,0); 
    }   
}

static void geoclue_map_clear_widgets(GeocluemapGtkLayout *geoclue_maplayout)
{
    GeocluemapGtkLayoutChild* widget;  
    widget = ( GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->widget_children,0);
   
    while(widget != NULL)
    {
        geoclue_maplayout->widget_children = g_list_remove(geoclue_maplayout->widget_children, widget);
        if(widget->widget != NULL)
        {
            gtk_container_remove(GTK_CONTAINER(geoclue_maplayout), widget->widget);
        }
        g_free(widget);
        widget = ( GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->widget_children,0); 
    }   
}




static void
geoclue_map_gtk_layout_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
    GeocluemapGtkLayout *geoclue_maplayout = GEOCLUE_MAP_GTK_LAYOUT (object);
    gboolean changed = FALSE;
    gdouble old_latitude;
    gdouble old_longitude;
    gint old_zoom;
    

    GeocluemapGtkLayoutChild* reference_image = ( GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->geoclue_map_image_children,0);
 
    if(reference_image == NULL)
    {
        reference_image = g_malloc(sizeof(GeocluemapGtkLayoutChild)); 
        reference_image->widget = NULL;
        reference_image->parent = geoclue_maplayout;
        reference_image->x = DEFAULT_X;
        reference_image->y = DEFAULT_Y;
        reference_image->latitude = 0;
        reference_image->longitude = 0;
        reference_image->zoom = 0;
        geoclue_maplayout->geoclue_map_image_children = g_list_prepend(geoclue_maplayout->geoclue_map_image_children, (gpointer)reference_image);
    }


  switch (prop_id)
    {
    case PROP_LATITUDE:
        old_latitude = reference_image->latitude;
        reference_image->latitude = g_value_get_double (value );       
        if(old_latitude != reference_image->latitude)
            changed = TRUE;
        break;
    case PROP_LONGITUDE:
        old_longitude = reference_image->longitude;
        reference_image->longitude = g_value_get_double (value);
        if(old_longitude != reference_image->longitude)
            changed = TRUE;
        break;
    case PROP_ZOOM:
        old_zoom = reference_image->zoom;
        reference_image->zoom = g_value_get_int (value );
        int max_zoom;
            geoclue_map_max_zoom(&max_zoom);
        if(reference_image->zoom > max_zoom)
            reference_image->zoom = max_zoom;
        if(old_longitude != reference_image->zoom)
            changed = TRUE;
        break;
    case PROP_EVENTBOX:
        geoclue_maplayout->eventbox = g_value_get_object (value );

        gtk_container_add(GTK_CONTAINER (geoclue_maplayout->eventbox), GTK_WIDGET(geoclue_maplayout));
    
        gtk_event_box_set_above_child(GTK_EVENT_BOX(geoclue_maplayout->eventbox), TRUE);
    
        g_signal_connect( G_OBJECT(geoclue_maplayout->eventbox), "button-press-event",
                            GTK_SIGNAL_FUNC(geoclue_map_gtk_layout_button_press), 
                            geoclue_maplayout);
                            
        g_signal_connect( G_OBJECT(geoclue_maplayout->eventbox), "button-release-event",
                            GTK_SIGNAL_FUNC(geoclue_map_gtk_layout_button_realease), 
                            geoclue_maplayout);
        g_signal_connect( G_OBJECT(geoclue_maplayout->eventbox), "motion-notify-event",
                            GTK_SIGNAL_FUNC(geoclue_map_gtk_layout_motion), 
                            geoclue_maplayout);
        break;
      

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
    
    if(changed == TRUE);
    {
        GeocluemapGtkLayoutChild* new_image;
        new_image = g_malloc(sizeof(GeocluemapGtkLayoutChild)); 
        new_image->widget = NULL;
        new_image->parent = geoclue_maplayout;
        new_image->x = reference_image->x;
        new_image->y = reference_image->y;
        new_image->latitude = reference_image->latitude;
        new_image->longitude = reference_image->longitude;
        new_image->zoom = reference_image->zoom;
               
        //Clear Images
        geoclue_map_clear_map_images( geoclue_maplayout);       
        
        //Readd new image
        geoclue_maplayout->geoclue_map_image_children = g_list_prepend(geoclue_maplayout->geoclue_map_image_children, (gpointer)new_image);

        if (geoclue_maplayout->idle_queued == FALSE)
        {
            geoclue_maplayout->idle_queued = TRUE;
            g_idle_add(geoclue_mapserver_idle_grab_map,geoclue_maplayout); 
        }
    }   
}


static void
geoclue_map_gtk_layout_init (GeocluemapGtkLayout *geoclue_maplayout)
{
    geoclue_maplayout->geoclue_map_image_children = NULL;
    geoclue_maplayout->eventbox = NULL;  

    geoclue_maplayout->pending = FALSE;
    geoclue_maplayout->idle_queued = FALSE;
    geoclue_maplayout->last_width = -1;
    GTK_WIDGET(geoclue_maplayout)->allocation.width = 0;
    GTK_WIDGET(geoclue_maplayout)->allocation.height = 0;
    geoclue_maplayout->button_click_x = 0;
    geoclue_maplayout->button_click_y = 0;
    
}





static void 
geoclue_map_gtk_layout_realize (GtkWidget *widget)
{
     if (GTK_WIDGET_CLASS (parent_class)->realize)
            (* GTK_WIDGET_CLASS (parent_class)->realize) (widget);
    return;
}



static void 
geoclue_map_gtk_layout_unrealize (GtkWidget *widget)
{
     if (GTK_WIDGET_CLASS (parent_class)->unrealize)
            (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
    return; 
}
gboolean       
geoclue_map_gtk_layout_add_widget(GeocluemapGtkLayout* geoclue_maplayout, GtkWidget* widget, gdouble IN_latitude, gdouble IN_longitude)
{
    GeocluemapGtkLayoutChild* reference_image = (GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->geoclue_map_image_children, 0);
   
    if(reference_image != NULL)
    {
        geoclue_maplayout->pending = TRUE;
        gint delta_x;
        gint delta_y;
        
        
        geoclue_map_latlong_to_offset(IN_latitude, IN_longitude, reference_image->zoom, reference_image->latitude, reference_image->longitude, &delta_x, &delta_y);
        GeocluemapGtkLayoutChild* new_widget = g_malloc(sizeof(GeocluemapGtkLayoutChild)); 
        new_widget->widget = widget;
        new_widget->parent = geoclue_maplayout;
    
        //X and Y are backwards inside a layout 0,0 is the top left corner, where in geoclue_map 0,0 is bottom lef
        new_widget->x = reference_image->x + delta_x;
        new_widget->y = reference_image->y - delta_y;
        new_widget->latitude = IN_latitude;
        new_widget->longitude = IN_longitude;
        new_widget->zoom = reference_image->zoom;
        geoclue_maplayout->widget_children = g_list_prepend(geoclue_maplayout->widget_children, (gpointer)new_widget);

        gtk_layout_put( GTK_LAYOUT(geoclue_maplayout),GTK_WIDGET(widget),new_widget->x,new_widget->y );
        gtk_widget_show_all(GTK_WIDGET(geoclue_maplayout->eventbox));  
    }
    
    return TRUE;
}



static void geoclue_map_gtk_layout_update_map(GdkPixbuf* map_buffer, void* userdata)
{  
    GeocluemapGtkLayoutChild* child = (GeocluemapGtkLayoutChild*)userdata;
    GeocluemapGtkLayout* geoclue_maplayout = child->parent;
    
    GtkWidget* image;
    image = gtk_image_new_from_pixbuf(map_buffer);
      
    gtk_layout_put( GTK_LAYOUT(geoclue_maplayout),GTK_WIDGET(image),child->x,child->y );
   
    child->widget = image;
   
    gtk_widget_show(GTK_WIDGET(image));
    gtk_widget_show_all(GTK_WIDGET(geoclue_maplayout->eventbox));      
      
    geoclue_maplayout->pending = FALSE;
    if(GTK_WIDGET(geoclue_maplayout)->allocation.width != geoclue_maplayout->last_width || GTK_WIDGET(geoclue_maplayout)->allocation.height != geoclue_maplayout->last_height)
    {
        if (geoclue_maplayout->idle_queued == FALSE)
        {
            geoclue_maplayout->idle_queued = TRUE;
            g_idle_add(geoclue_mapserver_idle_grab_map,geoclue_maplayout); 
        }
    }    
}

gboolean geoclue_map_gtk_layout_zoom_in(GeocluemapGtkLayout* geoclue_maplayout )
{
    int zoom;
    g_object_get(geoclue_maplayout, "zoom", &zoom, NULL);
    g_object_set(geoclue_maplayout, "zoom", --zoom, NULL);
}
gboolean geoclue_map_gtk_layout_zoom_out(GeocluemapGtkLayout* geoclue_maplayout)
{
    int zoom;
    g_object_get(geoclue_maplayout, "zoom", &zoom, NULL);
    g_object_set(geoclue_maplayout, "zoom", ++zoom, NULL);
}
gboolean geoclue_map_gtk_layout_zoom(GeocluemapGtkLayout* geoclue_maplayout, gint zoom)
{
      g_object_set(geoclue_maplayout, "zoom", zoom, NULL);  
}
gboolean   geoclue_map_gtk_layout_lat_lon(GeocluemapGtkLayout* geoclue_maplayout, gdouble IN_latitude, gdouble IN_longitude)
{
        g_object_set(geoclue_maplayout, "latitude", IN_latitude , "longitude", IN_longitude , NULL);
}

void geoclue_map_gtk_layout_update(GeocluemapGtkLayout* geoclue_maplayout, GeocluemapGtkLayoutChild* new_image)
{                
    int returnvalue = geoclue_map_gtk_get_gdk_pixbuf(new_image->latitude,new_image->longitude,GTK_WIDGET(geoclue_maplayout)->allocation.width,GTK_WIDGET(geoclue_maplayout)->allocation.height,new_image->zoom,geoclue_map_gtk_layout_update_map,new_image);    
    geoclue_maplayout->last_width = GTK_WIDGET(geoclue_maplayout)->allocation.width;
    geoclue_maplayout->last_height = GTK_WIDGET(geoclue_maplayout)->allocation.height;               
}


static gboolean geoclue_map_gtk_layout_button_press( GtkWidget      *widget, 
                                       GdkEventButton *event,
                                       gpointer userdata)
{
    GeocluemapGtkLayout* geoclue_maplayout = GEOCLUE_MAP_GTK_LAYOUT(userdata); 
    geoclue_maplayout->button_click_x = event->x;
    geoclue_maplayout->button_click_y = event->y;                  
    return FALSE;    
}


static gboolean geoclue_map_gtk_layout_motion(GtkWidget      *widget,
                                            GdkEventMotion *event,
                                       gpointer userdata)
                                            
{
    GeocluemapGtkLayout* geoclue_maplayout = GEOCLUE_MAP_GTK_LAYOUT(userdata); 

    gdouble next_x = gtk_adjustment_get_value(GTK_LAYOUT(geoclue_maplayout)->hadjustment) + geoclue_maplayout->button_click_x - event->x;
    gdouble next_y = gtk_adjustment_get_value(GTK_LAYOUT(geoclue_maplayout)->vadjustment) + geoclue_maplayout->button_click_y - event->y;      
    geoclue_maplayout->button_click_x = event->x;
    geoclue_maplayout->button_click_y = event->y;
   
    gtk_adjustment_set_value        (GTK_LAYOUT(geoclue_maplayout)->hadjustment,next_x );   
    gtk_adjustment_set_value        (GTK_LAYOUT(geoclue_maplayout)->vadjustment,next_y );
    gtk_widget_show_all (geoclue_maplayout->eventbox); 
    return FALSE;    
}





static gboolean geoclue_map_gtk_layout_button_realease( GtkWidget      *widget, 
                                       GdkEventButton *event,
                                       gpointer userdata)
{
    GeocluemapGtkLayout* geoclue_maplayout = GEOCLUE_MAP_GTK_LAYOUT(userdata); 

    //Reference image is always the first on the list
    GeocluemapGtkLayoutChild* reference_image = (GeocluemapGtkLayoutChild*)g_list_nth_data(geoclue_maplayout->geoclue_map_image_children, 0);
   
    if(reference_image != NULL)
    {
       geoclue_maplayout->pending = TRUE;
         
        gint delta_x = (gtk_adjustment_get_value(GTK_LAYOUT(geoclue_maplayout)->hadjustment))  - reference_image->x;
        gint delta_y = reference_image->y -(gtk_adjustment_get_value(GTK_LAYOUT(geoclue_maplayout)->vadjustment))  ;
        gdouble new_lat;
        gdouble new_lon;
        
        //Check to See if they moved at all
        if(delta_x == 0 && delta_y ==0 )
            return FALSE;
        
        geoclue_map_offset_to_latlong(delta_x, delta_y, reference_image->zoom, reference_image->latitude, reference_image->longitude, &new_lat, &new_lon);
        GeocluemapGtkLayoutChild* new_image = g_malloc(sizeof(GeocluemapGtkLayoutChild)); 
        new_image->widget = NULL;
        new_image->parent = geoclue_maplayout;
    
        //X and Y are backwards inside a layout 0,0 is the top left corner, where in geoclue_map 0,0 is bottom lef
        new_image->x = reference_image->x + delta_x;
        new_image->y = reference_image->y - delta_y;
        new_image->latitude = new_lat;
        new_image->longitude = new_lon;
        new_image->zoom = reference_image->zoom;
        geoclue_maplayout->geoclue_map_image_children = g_list_prepend(geoclue_maplayout->geoclue_map_image_children, (gpointer)new_image);
        geoclue_map_gtk_layout_update(geoclue_maplayout, new_image);
    
    }
    return FALSE;   
}



static void     
geoclue_map_gtk_layout_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
    
    GeocluemapGtkLayout* geoclue_maplayout = GEOCLUE_MAP_GTK_LAYOUT(widget); 
    GTK_WIDGET(geoclue_maplayout)->allocation.width = allocation->width;
    GTK_WIDGET(geoclue_maplayout)->allocation.height = allocation->height;              
   
    if(geoclue_maplayout->pending == FALSE && (GTK_WIDGET(geoclue_maplayout)->allocation.width != geoclue_maplayout->last_width || GTK_WIDGET(geoclue_maplayout)->allocation.height != geoclue_maplayout->last_height))
    {
        if (geoclue_maplayout->idle_queued == FALSE)
        {
            geoclue_maplayout->idle_queued = TRUE;
            //Add a little delay to not grab teh first resize event
            g_timeout_add(200, geoclue_mapserver_idle_grab_map,geoclue_maplayout); 
        }
    }   
}



#define __GEOCLUE_MAP_GTK_LAYOUT_C__
