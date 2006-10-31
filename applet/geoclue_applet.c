#include <string.h>

#include <panel-applet.h>
#include <gtk/gtkimage.h>
#include <geoclue/position.h>
#include <glib.h>



static gboolean
  on_button_press (GtkWidget      *event_box, 
                         GdkEventButton *event,
                         gpointer        data)
  {
    static int window_shown;
    static GtkWidget *window, *label;
    
    /* Don't react to anything other than the left mouse button;
       return FALSE so the event is passed to the default handler */
    if (event->button != 1)
        return FALSE;

    if (!window_shown) {
        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        geoclue_position_init();
        gdouble lat, lon;
        geoclue_position_current_position(&lat, &lon);
        gchar* text = g_strdup_printf("You are at %f %f\n", lat, lon);
        label = GTK_WIDGET(gtk_label_new (text));
        free(text);
        geoclue_position_close();
        gtk_container_add (GTK_CONTAINER (window), label);
        
        gtk_widget_show_all (window);
    }
    else
        gtk_widget_hide (GTK_WIDGET (window));

    window_shown = !window_shown;
    return TRUE;
  }
    
 


static gboolean
geoclue_applet_fill (PanelApplet *applet,
   const gchar *iid,
   gpointer data)
{
    GtkWidget *image;

    if (strcmp (iid, "OAFIID:GeoclueApplet") != 0)
        return FALSE;

    image = gtk_image_new_from_file ("/usr/share/pixmaps/geoclue.png");


    GtkWidget *event_box;
    event_box = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (event_box), image);

    g_signal_connect (G_OBJECT (event_box), 
                  "button_press_event",
                  G_CALLBACK (on_button_press),
                  image);

    gtk_container_add (GTK_CONTAINER (applet), event_box);



    gtk_widget_show_all (GTK_WIDGET (applet));

    return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:GeoclueApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "The Geoclue Applet",
                             "0",
                             geoclue_applet_fill,
                             NULL);
                             
                            
