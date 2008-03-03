/**
 * 
 * Expects to find a keyfile in user data dir
 * (~/.local/share/geoclue-localnet-gateways). 
 * 
 * The keyfile should contain entries like this:
 * 
 * [00:1D:7E:55:8D:80]
 * country=Finland
 * street=Solnantie 24
 * locality=Helsinki
 *
 * Only address is supported so far. File is only read on startup.
 */ 

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <geoclue/gc-provider.h>
#include <geoclue/gc-iface-address.h>
#include <geoclue/geoclue-address-details.h>

#define KEYFILE_NAME "/geoclue-localnet-gateways"

typedef struct {
	char *mac;
	GHashTable *address;
	GeoclueAccuracy *accuracy;
} Gateway;


typedef struct {
	GcProvider parent;
	
	GMainLoop *loop;
	
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
finalize (GObject *object)
{
	GeoclueLocalnet *localnet;
	GSList *l;
	
	localnet = GEOCLUE_LOCALNET (object);
	
	l = localnet->gateways;
	while (l) {
		Gateway *gw = l->data;
		
		g_free (gw->mac);
		g_hash_table_destroy (gw->address);
		geoclue_accuracy_free (gw->accuracy);
		g_free (gw);
		
		l = l->next;
	}
	g_slist_free (localnet->gateways);
	
	G_OBJECT_CLASS (geoclue_localnet_parent_class)->finalize (object);
}


static void
geoclue_localnet_class_init (GeoclueLocalnetClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *) klass;
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	
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
		g_warning ("mac address lookup failed");
		return NULL;
	}
	
	mac = g_malloc (mac_len);
	if (fgets (mac, mac_len, in) == NULL) {
		g_debug ("mac address lookup returned nothing");
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

static gboolean
geoclue_localnet_read_keyfile (GeoclueLocalnet *localnet, char *filename)
{
	GKeyFile *keyfile;
	char **groups;
	char **g;
	GError *error;
	
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, filename, 
	                                G_KEY_FILE_NONE, &error)) {
		g_warning ("Error loading keyfile %s", filename);
		g_error_free (error);
		return FALSE;
	}
	
	groups = g_key_file_get_groups (keyfile, NULL);
	g = groups;
	while (*g) {
		char **keys;
		char **k;
		Gateway *gateway = g_new0 (Gateway, 1);
		
		gateway->mac = *g;
		gateway->address = address_details_new ();
		
		keys = g_key_file_get_keys (keyfile, *g,
		                            NULL, NULL);
		k = keys;
		while (*k) {
			char *value;
			
			/*TODO error checks */
			value = g_key_file_get_string (keyfile, *g, *k, NULL);
			g_hash_table_insert (gateway->address, *k, value);
			
			k++;
		}
		
		gateway->accuracy = 
			geoclue_accuracy_new (get_accuracy_for_address (gateway->address), 0, 0);
		
		localnet->gateways = g_slist_prepend (localnet->gateways, gateway);
		
		g++;
	}
	return TRUE;
}

static void
geoclue_localnet_init (GeoclueLocalnet *localnet)
{
	const char *dir;
	char *filename;
	
	dir = g_get_user_data_dir ();
	filename = g_strconcat (dir, KEYFILE_NAME, NULL);
	
	localnet->gateways = NULL;
	
	gc_provider_set_details (GC_PROVIDER (localnet),
	                         "org.freedesktop.Geoclue.Providers.Localnet",
	                         "/org/freedesktop/Geoclue/Providers/Localnet",
	                         "Localnet", "Localnet provider");
	
	geoclue_localnet_read_keyfile (localnet, filename);
	g_free (filename);
}


static gboolean
get_address (GcIfaceAddress   *gc,
             int              *timestamp,
             GHashTable      **address,
             GeoclueAccuracy **accuracy,
             GError          **error)
{
	GSList *l;
	GeoclueLocalnet *localnet;
	char *mac;
	
	localnet = GEOCLUE_LOCALNET (gc);
	mac = get_mac_address ();
	
	l = localnet->gateways;
	while (l) {
		Gateway *gw = l->data;
		
		if (strcmp (gw->mac, mac) == 0) {
			if (timestamp) {
				*timestamp = time(NULL);
			}
			if (address) {
				*address = address_details_copy (gw->address);
			}
			if (accuracy) {
				*accuracy = geoclue_accuracy_copy (gw->accuracy);
			}
			g_free (mac);
			return TRUE;
		}
		
		l = l->next;
	}
	
	if (accuracy) {
		*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE, 0, 0);
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
