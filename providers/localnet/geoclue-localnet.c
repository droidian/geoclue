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
 * Any application that can obtain a reliable address can submit it 
 * to localnet provider through the D-Bus API -- it will then be provided
 * whenever connected to the same router:
 *    org.freedesktop.Geoclue.Localnet.SetAddress
 *    org.freedesktop.Geoclue.Localnet.SetAddressFields
 *
 * SetAddress allows setting the current address as a GeoclueAddress, 
 * while SetAddressFields is a convenience version with separate 
 * address fields. Shell example using SetAddressFields:
 * 
  dbus-send --print-reply --type=method_call \
            --dest=org.freedesktop.Geoclue.Providers.Localnet \
            /org/freedesktop/Geoclue/Providers/Localnet \
            org.freedesktop.Geoclue.Localnet.SetAddressFields \
            string: \
            string:"Finland" \
            string: \
            string:"Helsinki" \
            string: \
            string: \
            string:"Solnantie 24"
  
 * This would make the provider save the specified address with current 
 * router mac address. It will provide the address to clients whenever 
 * the computer is connected to the same router again.
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

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus.h>

#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-address.h>

#include "connectivity.h"

#define KEYFILE_NAME "geoclue-localnet-gateways"

typedef struct {
	char *mac;
	GHashTable *address;
	GeoclueAccuracy *accuracy;
} Gateway;


typedef struct {
	GcProvider parent;
	
	GMainLoop *loop;
	GeoclueConnectivity *conn;

	char *keyfile_name;
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

static gboolean geoclue_localnet_set_address (GeoclueLocalnet *localnet, GHashTable *address, GError **error);
static gboolean geoclue_localnet_set_address_fields (GeoclueLocalnet *localnet, char *country_code, char *country, char *region, char *locality, char *area, char *postalcode, char *street, GError **error);
#include "geoclue-localnet-glue.h"


static gboolean
get_status (GcIfaceGeoclue *gc,
            GeoclueStatus  *status,
            GError        **error)
{
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueLocalnet *localnet;
	
	localnet = GEOCLUE_LOCALNET (provider);
	g_main_loop_quit (localnet->loop);
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

	if (localnet->conn != NULL) {
		g_object_unref (localnet->conn);
		localnet->conn = NULL;
	}
	g_free (localnet->keyfile_name);
	free_gateway_list (localnet->gateways);
	
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
	
	dbus_g_object_type_install_info (geoclue_localnet_get_type (),
	                                 &dbus_glib_geoclue_localnet_object_info);

}

static void
geoclue_localnet_load_gateways_from_keyfile (GeoclueLocalnet  *localnet, 
                                             GKeyFile         *keyfile)
{
	char **groups;
	char **g;
	GError *error = NULL;
	
	groups = g_key_file_get_groups (keyfile, NULL);
	g = groups;
	while (*g) {
		GeoclueAccuracyLevel level;
		char **keys;
		char **k;
		Gateway *gateway = g_new0 (Gateway, 1);
		
		gateway->mac = g_ascii_strdown (*g, -1);
		gateway->address = geoclue_address_details_new ();
		
		/* read all keys in the group as address fields */
		keys = g_key_file_get_keys (keyfile, *g,
		                            NULL, &error);
		if (error) {
			g_warning ("Could not load keys for group [%s] from %s: %s", 
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
		
		level = geoclue_address_details_get_accuracy_level (gateway->address);
		gateway->accuracy = geoclue_accuracy_new (level, 0, 0);
		
		localnet->gateways = g_slist_prepend (localnet->gateways, gateway);
		
		g++;
	}
	g_strfreev (groups);
}

static Gateway *
geoclue_localnet_find_gateway (GeoclueLocalnet *localnet, char *mac)
{
	GSList *l;
	
	l = localnet->gateways;
	/* eww, should be using a hashtable or something here */
	while (l) {
		Gateway *gw = l->data;
		
		if (g_ascii_strcasecmp (gw->mac, mac) == 0) {
			return gw;
		}
		
		l = l->next;
	}
	
	return NULL;
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
	                         "Localnet", "provides Address based on current gateway mac address and a local address file (which can be updated through D-Bus)");
	
	
	localnet->gateways = NULL;
	
	/* load known addresses from keyfile */
	dir = g_get_user_config_dir ();
	g_mkdir_with_parents (dir, 0755);
	localnet->keyfile_name = g_build_filename (dir, KEYFILE_NAME, NULL);
	
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, localnet->keyfile_name, 
	                                G_KEY_FILE_NONE, &error)) {
		g_warning ("Could not load keyfile %s: %s", 
		           localnet->keyfile_name, error->message);
		g_error_free (error);
	}
	geoclue_localnet_load_gateways_from_keyfile (localnet, keyfile);
	g_key_file_free (keyfile);

	localnet->conn = geoclue_connectivity_new ();
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

static gboolean
geoclue_localnet_set_address (GeoclueLocalnet *localnet,
                              GHashTable *details,
                              GError **error)
{
	char *str, *mac;
	GKeyFile *keyfile;
	GError *int_err = NULL;
	localnet_keyfile_group *keyfile_group;
	Gateway *gw;
	
	if (!details) {
		/* TODO set error */
		return FALSE;
	}

	mac = geoclue_connectivity_get_router_mac (localnet->conn);
	if (!mac) {
		g_warning ("Couldn't get current gateway mac address");
		/* TODO set error */
		return FALSE;
	}
	/* reload keyfile just in case it's changed */
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, localnet->keyfile_name, 
	                                G_KEY_FILE_NONE, &int_err)) {
		g_warning ("Could not load keyfile %s: %s", 
		         localnet->keyfile_name, int_err->message);
		g_error_free (int_err);
		int_err = NULL;
	}
	
	/* remove old group (if exists) and add new to GKeyFile */
	g_key_file_remove_group (keyfile, mac, NULL);
	
	keyfile_group = g_new0 (localnet_keyfile_group, 1);
	keyfile_group->keyfile = keyfile;
	keyfile_group->group_name = mac;
	g_hash_table_foreach (details, (GHFunc) add_address_detail_to_keyfile, keyfile_group);
	g_free (keyfile_group);
	
	/* save keyfile*/
	str = g_key_file_to_data (keyfile, NULL, &int_err);
	if (int_err) {
		g_warning ("Failed to get keyfile data as string: %s", int_err->message);
		g_error_free (int_err);
		g_key_file_free (keyfile);
		g_free (mac);
		/* TODO set error */
		return FALSE;
	}
	
	g_file_set_contents (localnet->keyfile_name, str, -1, &int_err);
	g_free (str);
	if (int_err) {
		g_warning ("Failed to save keyfile: %s", int_err->message);
		g_error_free (int_err);
		g_key_file_free (keyfile);
		g_free (mac);
		/* TODO set error */
		return FALSE;
	}
	
	/* re-parse keyfile */
	free_gateway_list (localnet->gateways);
	localnet->gateways = NULL;
	geoclue_localnet_load_gateways_from_keyfile (localnet, keyfile);
	g_key_file_free (keyfile);
	
	gw = geoclue_localnet_find_gateway (localnet, mac);
	g_free (mac);
	
	if (gw) {
		gc_iface_address_emit_address_changed (GC_IFACE_ADDRESS (localnet),
		                                       time (NULL), gw->address, gw->accuracy);
	} else {
		/* empty address -- should emit anyway? */
	}
	return TRUE;
}

static gboolean
geoclue_localnet_set_address_fields (GeoclueLocalnet *localnet,
                                     char *country_code,
                                     char *country,
                                     char *region,
                                     char *locality,
                                     char *area,
                                     char *postalcode,
                                     char *street,
                                     GError **error)
{
	GHashTable *address;
	gboolean ret;
	
	address = geoclue_address_details_new ();
	if (country_code && (strlen (country_code) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_COUNTRYCODE), 
		                     g_strdup (country_code));
		if (!country) {
			geoclue_address_details_set_country_from_code (address);
		}
	}
	if (country && (strlen (country) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_COUNTRY), 
		                     g_strdup (country));
	}
	if (region && (strlen (region) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_REGION), 
		                     g_strdup (region));
	}
	if (locality && (strlen (locality) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_LOCALITY), 
		                     g_strdup (locality));
	}
	if (area && (strlen (area) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_AREA), 
		                     g_strdup (area));
	}
	if (postalcode && (strlen (postalcode) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_POSTALCODE), 
		                     g_strdup (postalcode));
	}
	if (street && (strlen (street) > 0)) {
		g_hash_table_insert (address,
		                     g_strdup (GEOCLUE_ADDRESS_KEY_STREET), 
		                     g_strdup (street));
	}
	
	ret = geoclue_localnet_set_address (localnet,
	                                    address,
	                                    error);
	g_hash_table_destroy (address);
	return ret;
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

	/* we may be trying to read /proc/net/arp right after network connection. 
	 * It's sometimes not up yet, try a couple of times */
	mac = geoclue_connectivity_get_router_mac (localnet->conn);

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
			*address = geoclue_address_details_copy (gw->address);
		} else {
			*address = geoclue_address_details_new ();
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
