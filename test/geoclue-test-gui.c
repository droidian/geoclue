/*
 * Geoclue
 * client-test-gui.c - Geoclue Test GUI
 *
 * Authors: Jussi Kukkonen <jku@linux.intel.com>
 * 
 * Copyright 2008 OpenedHand Ltd
 *           2008 Intel Corporation
 */

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
	COL_ADDRESS_IS_MASTER,
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
	COL_POSITION_IS_MASTER,
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
	GeoclueAccuracyLevel master_accuracy;
	GeoclueResourceFlags master_resources;
	
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

static gboolean
get_matching_tree_iter (GtkTreeModel *model, GeoclueProvider *provider, GtkTreeIter *iter)
{
	GeoclueProvider *p = NULL;
	gboolean valid;
	
	g_assert (model);
	g_assert (provider);
	
	valid = gtk_tree_model_get_iter_first (model, iter);
	while (valid) {
		gtk_tree_model_get (model, iter,
		                    COL_ADDRESS_PROVIDER, &p,
		                    -1);
		if (p == provider) {
			return TRUE;
		}
		
		valid = gtk_tree_model_iter_next (model, iter);
	}
	
	return FALSE;
}

static void
update_address (GeoclueTestGui *gui, GeoclueAddress *address, GHashTable *details)
{
	GtkTreeIter iter;
	
	g_assert (details);
	
	if (get_matching_tree_iter (GTK_TREE_MODEL (gui->address_store),
	                            GEOCLUE_PROVIDER (address),
	                            &iter)) {
		gtk_list_store_set (gui->address_store, &iter,
		                    COL_ADDRESS_COUNTRY, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_COUNTRY),
		                    COL_ADDRESS_COUNTRYCODE, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_COUNTRYCODE),
		                    COL_ADDRESS_REGION, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_REGION),
		                    COL_ADDRESS_LOCALITY, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_LOCALITY),
		                    COL_ADDRESS_AREA, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_AREA),
		                    COL_ADDRESS_POSTALCODE, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_POSTALCODE),
		                    COL_ADDRESS_STREET, g_hash_table_lookup (details, GEOCLUE_ADDRESS_KEY_STREET),
		                    -1);
	}
}


static void
update_position (GeoclueTestGui *gui, GeocluePosition *position, 
                 double lat, double lon, double alt)
{
	GtkTreeIter iter;
	
	if (get_matching_tree_iter (GTK_TREE_MODEL (gui->position_store),
	                            GEOCLUE_PROVIDER (position),
	                            &iter)) {
		gtk_list_store_set (gui->position_store, &iter,
		                    COL_POSITION_LAT, lat,
		                    COL_POSITION_LON, lon,
		                    COL_POSITION_ALT, alt,
		                    -1);
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
address_callback (GeoclueAddress *address,
                  int timestamp,
                  GHashTable *details,
                  GeoclueAccuracy *accuracy,
                  GError *error,
                  gpointer userdata)
{
	if (error) {
		g_warning ("Error getting address: %s\n", error->message);
		g_error_free (error);
		details = geoclue_address_details_new ();
	}
	
	update_address (GEOCLUE_TEST_GUI (userdata), address, details);
}

static void 
info_callback (GeoclueProvider *provider,
               char *name,
               char *description,
               GError *error,
               gpointer userdata)
{
	GtkTreeIter iter;
	GeoclueTestGui *gui = GEOCLUE_TEST_GUI (userdata);
	
	if (error) {
		g_warning ("Error getting provider info: %s\n", error->message);
		g_error_free (error);
		return;
	}
	
	if (get_matching_tree_iter (GTK_TREE_MODEL (gui->address_store),
	                            provider,
	                            &iter)) {
		/* skip master, that's handled in master_*_provider_changed */
		if (strcmp (name, "Geoclue Master") != 0) {
			gtk_list_store_set (gui->address_store, &iter,
					    COL_ADDRESS_PROVIDER_NAME, name,
					    -1);
		}
	}
	
	if (get_matching_tree_iter (GTK_TREE_MODEL (gui->position_store),
	                            provider,
	                            &iter)) {
		/* skip master, that's handled in master_*_provider_changed */
		if (strcmp (name, "Geoclue Master") != 0) {
			gtk_list_store_set (gui->position_store, &iter,
			                    COL_POSITION_PROVIDER_NAME, name,
			                    -1);
		}
	}
}

static void 
add_to_address_store (GeoclueTestGui *gui, GeoclueAddress *address, gboolean is_master)
{
	GtkTreeIter iter;
	
	g_assert (gui->address_store);
	g_assert (address);
	
	if (is_master) {
		/* master is already on the first line */
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->address_store), &iter);
	} else {
		gtk_list_store_append (gui->address_store, &iter);
	}
	gtk_list_store_set (gui->address_store, &iter,
	                    COL_ADDRESS_PROVIDER, address,
	                    COL_ADDRESS_IS_MASTER, is_master,
	                    -1);
	
	g_signal_connect (G_OBJECT (address), "address-changed",
				G_CALLBACK (address_changed), gui);
	
	geoclue_provider_get_provider_info_async (GEOCLUE_PROVIDER (address), 
	                                          info_callback,
	                                          gui);
	geoclue_address_get_address_async (address, address_callback, gui);
	
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
position_callback (GeocluePosition *position,
                   GeocluePositionFields fields,
                   int timestamp,
                   double lat, double lon, double alt,
                   GeoclueAccuracy *accuracy,
                   GError *error,
                   gpointer userdata)
{
	if (error) {
		g_warning ("Error getting position: %s\n", error->message);
		g_error_free (error);
		lat = lon = alt = 0.0/0.0;
	}
	update_position (GEOCLUE_TEST_GUI (userdata), position, lat, lon, alt);
}

static void 
add_to_position_store (GeoclueTestGui *gui, GeocluePosition *position, gboolean is_master)
{
	GtkTreeIter iter;
	
	g_assert (gui->position_store);
	g_assert (position);
	
	if (is_master) {
		/* master is already on the first line */
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->position_store), &iter);
	} else {
		gtk_list_store_append (gui->position_store, &iter);
	}
	gtk_list_store_set (gui->position_store, &iter,
	                    COL_POSITION_PROVIDER, position,
	                    COL_POSITION_IS_MASTER, is_master,
	                    -1);
	
	g_signal_connect (G_OBJECT (position), "position-changed",
				G_CALLBACK (position_changed), gui);
	
	geoclue_provider_get_provider_info_async (GEOCLUE_PROVIDER (position), 
	                                          info_callback,
	                                          gui);
	geoclue_position_get_position_async (position, position_callback, gui);
}


static GtkWidget *
get_address_tree_view (GeoclueTestGui *gui)
{
	GtkTreeView *view;
	GtkCellRenderer *renderer;
	
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
	                            G_TYPE_BOOLEAN,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING);
	
	gtk_tree_view_set_model (view, GTK_TREE_MODEL(gui->address_store));
	
	return GTK_WIDGET (view);
}


static GtkWidget *
get_position_tree_view (GeoclueTestGui *gui)
{
	GtkTreeView *view;
	GtkCellRenderer *renderer;
	
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
	                            G_TYPE_BOOLEAN,
	                            G_TYPE_DOUBLE,
	                            G_TYPE_DOUBLE,
	                            G_TYPE_DOUBLE);
	
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
set_requirements_callback (GeoclueMasterClient *client,
                           GError *error,
                           gpointer userdata)
{
	if (error) {
		g_printerr ("Setting requirements failed : %s", error->message);
		g_error_free (error);
	}
}

static void
update_master_requirements (GeoclueTestGui *gui)
{
	geoclue_master_client_set_requirements_async (gui->client, 
	                                              gui->master_accuracy,
	                                              0,
	                                              FALSE,
	                                              gui->master_resources,
	                                              set_requirements_callback,
	                                              NULL);
}

static void 
create_address_callback (GeoclueMasterClient *client,
                         GeoclueAddress      *address,
                         GError              *error,
                         gpointer             userdata)
{
	GeoclueTestGui *gui = GEOCLUE_TEST_GUI (userdata);
	
	if (error) {
		g_printerr ("Master Client: Failed to create address: %s", error->message);
		g_error_free (error);
		return;
	}
	add_to_address_store (gui, address, TRUE);
}

static void 
create_position_callback (GeoclueMasterClient *client,
                          GeocluePosition     *position,
                          GError              *error,
                          gpointer             userdata)
{
	GeoclueTestGui *gui = GEOCLUE_TEST_GUI (userdata);
	
	if (error) {
		g_printerr ("Master Client: Failed to create position: %s", error->message);
		g_error_free (error);
		return;
	}
	add_to_position_store (gui, position, TRUE);
}


static void
create_client_callback (GeoclueMaster       *master,
			GeoclueMasterClient *client,
			char                *object_path,
			GError              *error,
			gpointer             userdata)
{
	GDir *dir;
	char *name, *path, *service, *ifaces;
	GtkTreeIter iter;
	GeoclueTestGui *gui = GEOCLUE_TEST_GUI (userdata);
	
	if (error) {
		g_printerr ("Failed to create master client: %s", error->message);
		g_error_free (error);
		return;
	}
	
	gui->client = client;
	
	g_signal_connect (G_OBJECT (gui->client), "position-provider-changed",
	                  G_CALLBACK (master_position_provider_changed), gui);
	g_signal_connect (G_OBJECT (gui->client), "address-provider-changed",
	                  G_CALLBACK (master_address_provider_changed), gui);
	update_master_requirements (gui);
	
	/* add master providers to the lists */
	gtk_list_store_append (gui->position_store, &iter);
	geoclue_master_client_create_position_async (gui->client, 
	                                             create_position_callback,
	                                             gui);
	gtk_list_store_append (gui->address_store, &iter);
	geoclue_master_client_create_address_async (gui->client, 
	                                            create_address_callback,
	                                            gui);
	
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
			add_to_position_store (gui, geoclue_position_new (service, path), FALSE);
		}
		if (ifaces && strstr (ifaces, "org.freedesktop.Geoclue.Address")) {
			add_to_address_store (gui, geoclue_address_new (service, path), FALSE);
		}
		g_free (name);
		g_free (path);
		g_free (service);
		g_free (ifaces);
	}
	
	g_dir_close (dir);
	g_object_unref (master);
}

static void
geoclue_test_gui_load_providers (GeoclueTestGui *gui)
{
	GeoclueMaster *master;
	
	master = geoclue_master_get_default ();
	geoclue_master_create_client_async (master, 
	                                    create_client_callback,
	                                    gui);
}

static void
accuracy_combo_changed (GtkComboBox *combo, GeoclueTestGui *gui)
{
	GtkTreeIter iter;
	
	if (gtk_combo_box_get_active_iter (combo, &iter)) {
		GtkTreeModel *model;
		
		model = gtk_combo_box_get_model (combo);
		gtk_tree_model_get (model, &iter, 0, &gui->master_accuracy, -1); 
		
		update_master_requirements (gui);
	}
}

static void
gps_check_toggled (GtkCheckButton *btn, GeoclueTestGui *gui)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn))) {
		gui->master_resources |= GEOCLUE_RESOURCE_GPS;
	} else {
		gui->master_resources &= ~GEOCLUE_RESOURCE_GPS;
	}
	update_master_requirements (gui);
}

static void
network_check_toggled (GtkCheckButton *btn, GeoclueTestGui *gui)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn))) {
		gui->master_resources |= GEOCLUE_RESOURCE_NETWORK;
	} else {
		gui->master_resources &= ~GEOCLUE_RESOURCE_NETWORK;
	}
	update_master_requirements (gui);
}

static void
cell_check_toggled (GtkCheckButton *btn, GeoclueTestGui *gui)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn))) {
		gui->master_resources |= GEOCLUE_RESOURCE_CELL;
	} else {
		gui->master_resources &= ~GEOCLUE_RESOURCE_CELL;
	}
	update_master_requirements (gui);
}

static GtkWidget*
get_accuracy_combo (GeoclueTestGui *gui)
{
	GtkListStore *store;
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	
	store = gtk_list_store_new (2, G_TYPE_UINT, G_TYPE_STRING);
	
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
	                    0, GEOCLUE_ACCURACY_LEVEL_COUNTRY, 
	                    1, "Country",
	                    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
	                    0, GEOCLUE_ACCURACY_LEVEL_REGION, 
	                    1, "Region",
	                    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
	                    0, GEOCLUE_ACCURACY_LEVEL_LOCALITY, 
	                    1, "Locality",
	                    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
	                    0, GEOCLUE_ACCURACY_LEVEL_POSTALCODE, 
	                    1, "Postalcode",
	                    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
	                    0, GEOCLUE_ACCURACY_LEVEL_STREET, 
	                    1, "Street",
	                    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
	                    0, GEOCLUE_ACCURACY_LEVEL_DETAILED, 
	                    1, "Detailed",
	                    -1);
	
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
	                                "text", 1, NULL);
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	gui->master_accuracy = GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	
	return combo;
}


static void
geoclue_test_gui_init (GeoclueTestGui *gui)
{
	GtkWidget *address_view;
	GtkWidget *position_view;
	GtkWidget *notebook;
	GtkWidget *box;
	GtkWidget *frame;
	GtkWidget *hbox, *vbox;
	GtkWidget *label;
	GtkWidget *combo, *check;
	GtkWidget *scrolled_win;
	GtkWidget *view;
	
	gui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (gui->window), "destroy",
	                  G_CALLBACK (gtk_main_quit), NULL);
	
	view = gtk_text_view_new ();
	gtk_widget_set_size_request (GTK_WIDGET (view), 500, 200);
	g_object_set (G_OBJECT (view), "editable", FALSE, NULL);
	gui->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	
	
	box = gtk_vbox_new (FALSE, 8);
	gtk_container_add (GTK_CONTAINER (gui->window), box);
	
	frame = gtk_frame_new ("Master settings");
	gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 4);
	
	
	hbox = gtk_hbox_new (FALSE, 24);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 8);
	label = gtk_label_new ("Required accuracy level:");
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	combo = get_accuracy_combo (gui); 
	g_signal_connect (combo, "changed",
	                  G_CALLBACK (accuracy_combo_changed), gui);
	gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	label = gtk_label_new ("Allow resources:");
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	check = gtk_check_button_new_with_label ("Network");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
	gui->master_resources |= GEOCLUE_RESOURCE_NETWORK;
	g_signal_connect (check, "toggled",
	                  G_CALLBACK (network_check_toggled), gui);
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	
	check = gtk_check_button_new_with_label ("GPS");
	g_signal_connect (check, "toggled",
	                  G_CALLBACK (gps_check_toggled), gui);
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	
	check = gtk_check_button_new_with_label ("Cell");
	g_signal_connect (check, "toggled",
	                  G_CALLBACK (cell_check_toggled), gui);
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	
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
	
	geoclue_test_gui_load_providers (gui);
	
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
