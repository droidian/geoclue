/**
 * 
 * Expects to find a keyfile in user config dir
 * (~/.config/geoclue-localnet-gateways). 
 * 
 * The keyfile should contain entries like this:
 * 
 * [00:1D:7E:55:8D:80]
 * country=Finland
 * street=Solnantie 24
 * locality=Helsinki
 *
 * Only address interface is supported so far. 
 * 
 * Provider connects to manual provider "address-changed" signals
 * and adds updates addresses in the keyfile based on them (using 
 * current gateway mac address as group name).
 */ 

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-address.h>
#include <geoclue/geoclue-address.h>

#define KEYFILE_NAME "geoclue-localnet-gateways"

typedef struct {
	char *mac;
	GHashTable *address;
	GeoclueAccuracy *accuracy;
} Gateway;


typedef struct {
	GcProvider parent;
	
	GMainLoop *loop;
	
	char *keyfile_name;
	GeoclueAddress *manual;
	GSList *gateways;
} GeoclueLocalnet;

typedef struct {
	GcProviderClass parent_class;
} GeoclueLocalnetClass;

#define GEOCLUE_TYPE_LOCALNET (geoclue_localnet_get_type ())
#define GEOCLUE_LOCALNET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_LOCALNET, GeoclueLocalnet))

static void geoclue_localnet_address_init (GcIfaceAddressClass *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueLocalnet, geoclue_localnet, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_ADDRESS,
                                                geoclue_localnet_address_init))

static gboolean
get_status (GcIfaceGeoclue *gc,
            GeoclueStatus  *status,
            GError        **error)
{
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static gboolean
shutdown (GcIfaceGeoclue *gc,
          GError        **error)
{
	GeoclueLocalnet *localnet;
	
	localnet = GEOCLUE_LOCALNET (gc);
	g_main_loop_quit (localnet->loop);
	return TRUE;
}

static void
free_gateway_list (GSList *gateways)
{
	GSList *l;
	
	l = gateways;
	while (l) {
		Gateway *gw;
		
		gw = l->data;
		g_free (gw->mac);
		g_hash_table_destroy (gw->address);
		geoclue_accuracy_free (gw->accuracy);
		g_free (gw);
		
		l = l->next;
	}
	g_slist_free (gateways);
}

static void
finalize (GObject *object)
{
	GeoclueLocalnet *localnet;
	
	localnet = GEOCLUE_LOCALNET (object);
	
	g_free (localnet->keyfile_name);
	free_gateway_list (localnet->gateways);
	
	G_OBJECT_CLASS (geoclue_localnet_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GeoclueLocalnet *localnet;
	
	localnet = GEOCLUE_LOCALNET (object);
	g_object_unref (localnet->manual);
	
	G_OBJECT_CLASS (geoclue_localnet_parent_class)->dispose (object);
}

static void
geoclue_localnet_class_init (GeoclueLocalnetClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *) klass;
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;
	
	p_class->get_status = get_status;
	p_class->shutdown = shutdown;
}

static char *
get_mac_address ()
	{
	/* this is an ugly hack, but it seems there is no easy 
	 * ioctl-based way to get the mac address of the router. This 
	 * implementation expects the system to have netstat, grep and awk 
	 * */
	
	FILE *in;
	char *mac;
	int mac_len = sizeof (char) * 18;
	
	if (!(in = popen ("ROUTER_IP=`netstat -rn | grep '^0.0.0.0 ' | awk '{ print $2 }'` && grep \"^$ROUTER_IP \" /proc/net/arp | awk '{print $4}'", "r"))) {
		return NULL;
	}
	
	mac = g_malloc (mac_len);
	if (fgets (mac, mac_len, in) == NULL) {
		g_free (mac);
		pclose (in);
		return NULL;
	}
	
	
	pclose (in);
	return mac;
}


static GeoclueAccuracyLevel
get_accuracy_for_address (GHashTable *address)
{
	if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_STREET)) {
		return GEOCLUE_ACCURACY_LEVEL_STREET;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_POSTALCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_POSTALCODE;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_LOCALITY)) {
		return GEOCLUE_ACCURACY_LEVEL_LOCALITY;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_REGION)) {
		return GEOCLUE_ACCURACY_LEVEL_REGION;
	} else if (g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRY) ||
	           g_hash_table_lookup (address, GEOCLUE_ADDRESS_KEY_COUNTRYCODE)) {
		return GEOCLUE_ACCURACY_LEVEL_COUNTRY;
	}
	return GEOCLUE_ACCURACY_LEVEL_NONE;
}
static void
geoclue_localnet_load_gateways_from_keyfile (GeoclueLocalnet  *localnet, 
                                             GKeyFile         *keyfile)
{
	char **groups;
	char **g;
	GError *error;
	
	groups = g_key_file_get_groups (keyfile, NULL);
	g = groups;
	while (*g) {
		char **keys;
		char **k;
		Gateway *gateway = g_new0 (Gateway, 1);
		
		gateway->mac = *g;
		gateway->address = address_details_new ();
		
		/* read all keys in the group as address fields */
		keys = g_key_file_get_keys (keyfile, *g,
		                            NULL, &error);
		if (error) {
			g_warning ("Could not load keys for group %s from %s: %s", 
			           *g, localnet->keyfile_name, error->message);
			g_error_free (error);
			error = NULL;
		}
		
		k = keys;
		while (*k) {
			char *value;
			
			value = g_key_file_get_string (keyfile, *g, *k, NULL);
			g_hash_table_insert (gateway->address, 
			                     *k, value);
			k++;
		}
		g_free (keys);
		
		gateway->accuracy = 
			geoclue_accuracy_new (get_accuracy_for_address (gateway->address), 0, 0);
		
		localnet->gateways = g_slist_prepend (localnet->gateways, gateway);
		
		g++;
	}
	g_free (groups);
}

static Gateway *
geoclue_localnet_find_gateway (GeoclueLocalnet *localnet, char *mac)
{
	GSList *l;
	
	l = localnet->gateways;
	while (l) {
		Gateway *gw = l->data;
		
		if (strcmp (gw->mac, mac) == 0) {
			return gw;
		}
		
		l = l->next;
	}
	
	return NULL;
}

typedef struct {
	GKeyFile *keyfile;
	char *group_name;
} localnet_keyfile_group;

static void
add_address_detail_to_keyfile (char *key, char *value, 
                               localnet_keyfile_group *group)
{
	g_key_file_set_string (group->keyfile, group->group_name,
	                       key, value);
}

static void
manual_address_changed (GeoclueAddress  *manual,
                        int              timestamp,
                        GHashTable      *details,
                        GeoclueAccuracy *accuracy,
                        GeoclueLocalnet *localnet)
{
	char *mac, *str;
	GeoclueAccuracyLevel level;
	GKeyFile *keyfile;
	GError *error = NULL;
	localnet_keyfile_group *keyfile_group;
	
	g_assert (accuracy);
	g_assert (details);
	
	geoclue_accuracy_get_details (accuracy, &level, NULL, NULL);
	if (level == GEOCLUE_ACCURACY_LEVEL_NONE) {
		return;
	}
	
	mac = get_mac_address ();
	if (!mac) {
		g_warning ("Couldn't get current gateway mac address");
		return;
	}
	/* reload keyfile just in case it's changed */
	
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, localnet->keyfile_name, 
	                                G_KEY_FILE_NONE, &error)) {
		g_warning ("Error loading keyfile %s: %s", 
		         localnet->keyfile_name, error->message);
		g_error_free (error);
		error = NULL;
	}
	
	/* remove old group (if exists) and add new to GKeyFile */
	g_key_file_remove_group (keyfile, mac, NULL);
	
	keyfile_group = g_new0 (localnet_keyfile_group, 1);
	keyfile_group->keyfile = keyfile;
	keyfile_group->group_name = mac;
	g_hash_table_foreach (details, (GHFunc) add_address_detail_to_keyfile, keyfile_group);
	g_free (keyfile_group);
	g_free (mac);
	
	/* save keyfile*/
	str = g_key_file_to_data (keyfile, NULL, &error);
	if (error) {
		g_warning ("Failed to get keyfile data as string: %s", error->message);
		g_error_free (error);
		g_key_file_free (keyfile);
		return;
	}
	
	g_file_set_contents (localnet->keyfile_name, str, -1, &error);
	if (error) {
		g_warning ("Failed to save keyfile: %s", error->message);
		g_error_free (error);
		g_key_file_free (keyfile);
		g_free (str);
		return;
	}
	g_free (str);
	
	/* re-parse keyfile */
	free_gateway_list (localnet->gateways);
	localnet->gateways = NULL;
	geoclue_localnet_load_gateways_from_keyfile (localnet, keyfile);
	
	g_key_file_free (keyfile);
}

static void
geoclue_localnet_init (GeoclueLocalnet *localnet)
{
	const char *dir;
	GKeyFile *keyfile;
	GError *error = NULL;
	
	gc_provider_set_details (GC_PROVIDER (localnet),
	                         "org.freedesktop.Geoclue.Providers.Localnet",
	                         "/org/freedesktop/Geoclue/Providers/Localnet",
	                         "Localnet", "provides Address based using current gateway mac address and a local file. Also saves data from manual-provider");
	
	
	localnet->gateways = NULL;
	
	/* load known addresses from keyfile */
	dir = g_get_user_config_dir ();
	localnet->keyfile_name = g_build_filename (dir, KEYFILE_NAME, NULL);
	
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, localnet->keyfile_name, 
	                                G_KEY_FILE_NONE, &error)) {
		g_warning ("Error loading keyfile %s: %s", 
		           localnet->keyfile_name, error->message);
		g_error_free (error);
	}
	geoclue_localnet_load_gateways_from_keyfile (localnet, keyfile);
	g_key_file_free (keyfile);
	
	
	/* connect to manual provider signals */
	localnet->manual = 
		geoclue_address_new ("org.freedesktop.Geoclue.Providers.Manual",
	                             "/org/freedesktop/Geoclue/Providers/Manual");
	g_signal_connect (localnet->manual, "address-changed", 
	                  G_CALLBACK (manual_address_changed), localnet);
}

static gboolean
get_address (GcIfaceAddress   *gc,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GeoclueLocalnet *localnet;
	char *mac;
	Gateway *gw;
	
	localnet = GEOCLUE_LOCALNET (gc);
	mac = get_mac_address ();
	if (!mac) {
		g_warning ("Couldn't get current gateway mac address");
		if (error) {
			g_set_error (error, GEOCLUE_ERROR, 
			             GEOCLUE_ERROR_NOT_AVAILABLE, "Could not get current gateway mac address");
		}
		return FALSE;
	}
	
	gw = geoclue_localnet_find_gateway (localnet, mac);
	g_free (mac);
	
	if (timestamp) {
		*timestamp = time(NULL);
	}
	if (address) {
		if (gw) {
			*address = address_details_copy (gw->address);
		} else {
			*address = address_details_new ();
		}
	}
	if (accuracy) {
		if (gw) {
			*accuracy = geoclue_accuracy_copy (gw->accuracy);
		} else {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
		}
	}
	return TRUE;
}

static void
geoclue_localnet_address_init (GcIfaceAddressClass *iface)
{
	iface->get_address = get_address;
}

int
main (int    argc,
      char **argv)
{
	GeoclueLocalnet *localnet;
	
	g_type_init ();
	
	localnet = g_object_new (GEOCLUE_TYPE_LOCALNET, NULL);
	localnet->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (localnet->loop);
	
	g_main_loop_unref (localnet->loop);
	g_object_unref (localnet);
	
	return 0;
}
