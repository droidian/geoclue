
#include <time.h>
#include <string.h>

#include <glib-object.h>
#include <gtk/gtk.h>

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-master-client.h>
#include <geoclue/geoclue-address.h>
#include <geoclue/geoclue-position.h>

enum {
	COL_ADDRESS_PROVIDER = 0,
	COL_ADDRESS_PROVIDER_NAME,
	COL_ADDRESS_COUNTRY,
	COL_ADDRESS_COUNTRYCODE,
	COL_ADDRESS_REGION,
	COL_ADDRESS_LOCALITY,
	COL_ADDRESS_AREA,
	COL_ADDRESS_POSTALCODE,
	COL_ADDRESS_STREET,
	NUM_ADDRESS_COLS
};
enum {
	COL_POSITION_PROVIDER = 0,
	COL_POSITION_PROVIDER_NAME,
	COL_POSITION_LAT,
	COL_POSITION_LON,
	COL_POSITION_ALT,
	NUM_POSITION_COLS
};


#define GEOCLUE_TYPE_TEST_GUI geoclue_test_gui_get_type()
#define GEOCLUE_TEST_GUI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_TEST_GUI, GeoclueTestGui))
#define GEOCLUE_TEST_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_TYPE_TEST_GUI, GeoclueTestGuiClass))
#define GEOCLUE_TEST_GUI_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCLUE_TYPE_TEST_GUI, GeoclueTestGuiClass))

typedef struct {
	GObject parent;
	
	GtkWidget *window;
	GtkTextBuffer *buffer;
	
	GeoclueMasterClient *client;
	char *master_client_path;
	
	GtkListStore *position_store;
	GList *position_providers; /* PositionProviders, first one is master */
	
	GtkListStore *address_store; 
	GList *address_providers; /* AddressProviders, first one is master */
} GeoclueTestGui;

typedef struct {
	GObjectClass parent_class;
} GeoclueTestGuiClass;

G_DEFINE_TYPE (GeoclueTestGui, geoclue_test_gui, G_TYPE_OBJECT)


static void
geoclue_test_gui_dispose (GObject *object)
{
	
	G_OBJECT_CLASS (geoclue_test_gui_parent_class)->dispose (object);
}


static void
geoclue_test_gui_class_init (GeoclueTestGuiClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->dispose = geoclue_test_gui_dispose;
}


static void
geoclue_test_gui_master_log_message (GeoclueTestGui *gui, char *message)
{
	GtkTextIter iter;
	char *line;
	time_t rawtime;
	struct tm *timeinfo;
	char time_buffer [20];
	
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	
	strftime (time_buffer, 19, "%X", timeinfo);
	line = g_strdup_printf ("%s: %s\n", time_buffer, message);
	
	gtk_text_buffer_get_end_iter (gui->buffer, &iter);
	gtk_text_buffer_insert (gui->buffer, &iter, line, -1);
	
	g_free (line);
}

static void
update_address (GeoclueTestGui *gui, GeoclueAddress *address, GHashTable *details)
{
	GtkTreeIter iter;
	gboolean valid;
	GeoclueAddress *a = NULL;
	
	g_assert (details);
	
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->address_store), &iter);
	while (valid) {
		gtk_tree_model_get (GTK_TREE_MODEL (gui->address_store), &iter,
		                    COL_ADDRESS_PROVIDER, &a,
		                    -1);
		if (address == a) {
			gtk_list_store_set (gui->address_store, &iter,
			                    COL_ADDRESS_COUNTRY, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_COUNTRY),
			                    COL_ADDRESS_COUNTRYCODE, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_COUNTRYCODE),
			                    COL_ADDRESS_REGION, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_REGION),
			                    COL_ADDRESS_LOCALITY, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_LOCALITY),
			                    COL_ADDRESS_AREA, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_AREA),
			                    COL_ADDRESS_POSTALCODE, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_POSTALCODE),
			                    COL_ADDRESS_STREET, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_STREET),
			                    -1);
			break;
		}
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (gui->address_store), &iter);
	}
	
}


static void
update_position (GeoclueTestGui *gui, GeocluePosition *position, 
                 double lat, double lon, double alt)
{
	GtkTreeIter iter;
	gboolean valid;
	GeocluePosition *p = NULL;
	
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->position_store), &iter);
	
	while (valid) {
		gtk_tree_model_get (GTK_TREE_MODEL (gui->position_store), &iter,
		                    COL_POSITION_PROVIDER, &p,
		                    -1);
		if (position == p) {
			gtk_list_store_set (gui->position_store, &iter,
			                    COL_POSITION_LAT, lat,
			                    COL_POSITION_LON, lon,
			                    COL_POSITION_ALT, alt,
			                    -1);
			break;
		}
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (gui->position_store), &iter);
	}
	
}


static void
address_changed (GeoclueAddress  *address,
                 int              timestamp,
                 GHashTable      *details,
                 GeoclueAccuracy *accuracy,
                 GeoclueTestGui  *gui)
{
	update_address (gui, address, details);
}


static void
position_changed (GeocluePosition      *position,
                  GeocluePositionFields fields,
                  int                   timestamp,
                  double                latitude,
                  double                longitude,
                  double                altitude,
                  GeoclueAccuracy      *accuracy,
                  GeoclueTestGui       *gui)
{
	update_position (gui, position, latitude, longitude, altitude);
}

static void 
append_to_address_store (GeoclueTestGui *gui, GeoclueAddress *address)
{
	GHashTable *details = NULL;
	GtkTreeIter iter;
	char *name;
	
	g_assert (address);
	
	geoclue_provider_get_provider_info (GEOCLUE_PROVIDER (address), 
	                                    &name, NULL, NULL);
	
	gtk_list_store_append (gui->address_store, &iter);
	gtk_list_store_set (gui->address_store, &iter,
	                    COL_ADDRESS_PROVIDER, address,
	                    COL_ADDRESS_PROVIDER_NAME, name,
	                    -1);
	
	g_signal_connect (G_OBJECT (address), "address-changed",
				G_CALLBACK (address_changed), gui);
	
	if (!geoclue_address_get_address (address, NULL, 
						&details, NULL, 
						NULL)) {
		g_warning ("Error getting address\n");
		details = geoclue_address_details_new ();
	}
	update_address (gui, address, details);
	g_hash_table_destroy (details);
}

static gboolean
get_next_provider (GDir *dir, char **name, char **service, char **path, char **ifaces)
{
	const char *filename;
	char *fullname;
	GKeyFile *keyfile;
	gboolean ret;
	GError *error;
	
	filename = g_dir_read_name (dir);
	if (!filename) {
		return FALSE;
	}
	
	fullname = g_build_filename (GEOCLUE_PROVIDERS_DIR, 
	                             filename, NULL);
	keyfile = g_key_file_new ();
	ret = g_key_file_load_from_file (keyfile, fullname, 
	                                 G_KEY_FILE_NONE, &error);
	g_free (fullname);
	
	if (!ret) {
		g_warning ("Error loading %s: %s", filename, error->message);
		g_error_free (error);
	} else {
		*name = g_key_file_get_value (keyfile, "Geoclue Provider",
		                              "Name", NULL);
		
		*service = g_key_file_get_value (keyfile, "Geoclue Provider",
		                                 "Service", NULL);
		*path = g_key_file_get_value (keyfile, "Geoclue Provider",
		                              "Path", NULL);
		*ifaces = g_key_file_get_value (keyfile, "Geoclue Provider",
		                              "Interfaces", NULL);
	}
	
	g_key_file_free (keyfile);
	return TRUE;
}

static void 
append_to_position_store (GeoclueTestGui *gui, GeocluePosition *position)
{
	double lat, lon, alt;
	GtkTreeIter iter;
	GError *error = NULL;
	char *name;
	
	g_assert (position);
	
	lat = lon = alt = 0.0/0.0;
	geoclue_provider_get_provider_info (GEOCLUE_PROVIDER (position), 
	                                    &name, NULL, NULL);
	
	gtk_list_store_append (gui->position_store, &iter);
	gtk_list_store_set (gui->position_store, &iter,
	                    COL_POSITION_PROVIDER, position,
	                    COL_POSITION_PROVIDER_NAME, name,
	                    -1);
	
	g_signal_connect (G_OBJECT (position), "position-changed",
				G_CALLBACK (position_changed), gui);
	
	if (!geoclue_position_get_position (position, NULL, 
	                                    &lat, &lon, &alt, 
	                                    NULL, &error)) {
		if (error) {
			g_warning ("Error getting position: %s\n", error->message);
			g_error_free (error);
		}
	}
	
	update_position (gui, position, lat, lon, alt);
}


static GtkWidget *
get_address_tree_view (GeoclueTestGui *gui)
{
	GtkTreeView *view;
	GtkCellRenderer *renderer;
	GList *l;
	
	view = GTK_TREE_VIEW (gtk_tree_view_new ());
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             "Provider",  
	                                             renderer,
	                                             "text",
	                                             COL_ADDRESS_PROVIDER_NAME,
	                                             NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_COUNTRY,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_COUNTRY,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_COUNTRYCODE,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_COUNTRYCODE,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_REGION,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_REGION,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_LOCALITY,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_LOCALITY,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_AREA,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_AREA,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_POSTALCODE,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_POSTALCODE,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             GEOCLUE_ADDRESS_KEY_STREET,  
	                                             renderer,
	                                             "text", 
	                                             COL_ADDRESS_STREET,
	                                             NULL);
	
	gui->address_store = gtk_list_store_new (NUM_ADDRESS_COLS, 
	                            G_TYPE_POINTER, 
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING);
	
	for (l = gui->address_providers; l; l = l->next) {
		append_to_address_store (gui, l->data);
	}
	
	gtk_tree_view_set_model (view, GTK_TREE_MODEL(gui->address_store));
	
	return GTK_WIDGET (view);
}


static GtkWidget *
get_position_tree_view (GeoclueTestGui *gui)
{
	GtkTreeView *view;
	GtkCellRenderer *renderer;
	GList *l;
	
	view = GTK_TREE_VIEW (gtk_tree_view_new ());
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             "Provider",  
	                                             renderer,
	                                             "text",
	                                             COL_POSITION_PROVIDER_NAME,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             "latitude",  
	                                             renderer,
	                                             "text", 
	                                             COL_POSITION_LAT,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             "longitude",  
	                                             renderer,
	                                             "text", 
	                                             COL_POSITION_LON,
	                                             NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1,      
	                                             "altitude",  
	                                             renderer,
	                                             "text", 
	                                             COL_POSITION_ALT,
	                                             NULL);
	
	gui->position_store = gtk_list_store_new (NUM_POSITION_COLS, 
	                            G_TYPE_POINTER, 
	                            G_TYPE_STRING,
	                            G_TYPE_DOUBLE,
	                            G_TYPE_DOUBLE,
	                            G_TYPE_DOUBLE);
	
	for (l = gui->position_providers; l; l = l->next) {
		append_to_position_store (gui, l->data);
	}

	gtk_tree_view_set_model (view, GTK_TREE_MODEL(gui->position_store));
	
	return GTK_WIDGET (view);
}

static void
master_position_provider_changed (GeoclueMasterClient *client,
                                  char *name,
                                  char *description, 
                                  char *service,
                                  char *path,
                                  GeoclueTestGui *gui)
{
	GtkTreeIter iter;
	char *msg;
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->position_store), 
					   &iter)) {
		gtk_list_store_set (gui->position_store, &iter,
				    COL_POSITION_PROVIDER_NAME, g_strdup_printf ("Master (%s)", name),
				    -1);
	}
	msg = g_strdup_printf ("Master: position provider changed: %s", name);
	geoclue_test_gui_master_log_message (gui, msg);
	g_free (msg);
}

static void
master_address_provider_changed (GeoclueMasterClient *client,
                                 char *name,
                                 char *description, 
                                 char *service,
                                 char *path,
                                 GeoclueTestGui *gui)
{
	GtkTreeIter iter;
	char *msg;
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->address_store), 
					   &iter)) {
		gtk_list_store_set (gui->address_store, &iter,
				    COL_ADDRESS_PROVIDER_NAME, g_strdup_printf ("Master (%s)", name),
				    -1);
	}
	msg = g_strdup_printf ("Master: address provider changed: %s", name);
	geoclue_test_gui_master_log_message (gui, msg);
	g_free (msg);
}

static void
geoclue_test_gui_load_providers (GeoclueTestGui *gui)
{
	GeoclueMaster *master;
	GDir *dir;
	char *name, *path, *service, *ifaces;
	GError *error;
	
	master = geoclue_master_get_default ();
	gui->client = geoclue_master_create_client (master, &gui->master_client_path, NULL);
	g_object_unref (master);
	
	if (!gui->client) {
		g_printerr ("No Geoclue master client!");
		return;
	}
	
	g_signal_connect (G_OBJECT (gui->client), "position-provider-changed",
	                  G_CALLBACK (master_position_provider_changed), gui);
	g_signal_connect (G_OBJECT (gui->client), "address-provider-changed",
	                  G_CALLBACK (master_address_provider_changed), gui);
	
	if (!geoclue_master_client_set_requirements (gui->client, 
	                                             GEOCLUE_ACCURACY_LEVEL_COUNTRY,
	                                             0,
	                                             FALSE,
	                                             GEOCLUE_RESOURCE_GPS | GEOCLUE_RESOURCE_NETWORK,
	                                             NULL)){
		g_printerr ("set_requirements failed");
	}
	
	
	/* add individual providers based on files in GEOCLUE_PROVIDERS_DIR  */
	dir = g_dir_open (GEOCLUE_PROVIDERS_DIR, 0, &error);
	if (!dir) {
		g_warning ("Error opening %s: %s\n", 
		           GEOCLUE_PROVIDERS_DIR, error->message);
		g_error_free (error);
		return;
	}
	
	name = service = path = ifaces = NULL;
	while (get_next_provider (dir, &name, &service, &path, &ifaces)) {
		if (ifaces && strstr (ifaces, "org.freedesktop.Geoclue.Position")) {
			gui->position_providers = 
				g_list_prepend (gui->position_providers,
				                geoclue_position_new (service, path));
		}
		
		if (ifaces && strstr (ifaces, "org.freedesktop.Geoclue.Address")) {
			gui->address_providers = 
				g_list_prepend (gui->address_providers,
				                geoclue_address_new (service, path));
		}
		g_free (name);
		g_free (path);
		g_free (service);
		g_free (ifaces);
	}
	g_dir_close (dir);
	
	/* add master providers to the lists */
	gui->position_providers = 
		g_list_prepend (gui->position_providers,
		                geoclue_master_client_create_position (gui->client, NULL));
	gui->address_providers = 
		g_list_prepend (gui->address_providers,
		                geoclue_master_client_create_address (gui->client, NULL));
}


static void
geoclue_test_gui_init (GeoclueTestGui *gui)
{
	GtkWidget *address_view;
	GtkWidget *position_view;
	GtkWidget *notebook;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *scrolled_win;
	GtkWidget *view;
	
	geoclue_test_gui_load_providers (gui);
	
	gui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (gui->window), "destroy",
	                  G_CALLBACK (gtk_main_quit), NULL);
	
	view = gtk_text_view_new ();
	gtk_widget_set_size_request (GTK_WIDGET (view), 500, 200);
	g_object_set (G_OBJECT (view), "editable", FALSE, NULL);
	gui->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	
	
	box = gtk_vbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (gui->window), box);
	
	notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (box), notebook, FALSE, FALSE, 0);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), TRUE);
	
	address_view = get_address_tree_view (gui);
	label = gtk_label_new ("Address");
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), address_view, label);
	
	position_view = get_position_tree_view (gui);
	label = gtk_label_new ("Position");
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), position_view, label);
	
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
	label = gtk_label_new ("Master log");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), 
	                                GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (box), scrolled_win);
	gtk_container_add (GTK_CONTAINER (scrolled_win), view);
	
	geoclue_test_gui_master_log_message (gui, "Started Geoclue test UI");
	
	gtk_widget_show_all (gui->window);
}


int main (int argc, char **argv)
{
	GeoclueTestGui *gui;
	
	gtk_init (&argc, &argv);
	
	gui = g_object_new (GEOCLUE_TYPE_TEST_GUI, NULL);
	gtk_main ();
	
	g_object_unref (gui);
	
	return 0;
}
