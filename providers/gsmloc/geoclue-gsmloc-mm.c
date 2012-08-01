/*
 * Geoclue
 * geoclue-gsmloc-mm.c - An Address/Position provider for ModemManager
 *
 * Author: Dan Williams <dcbw@redhat.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <config.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>

#include "geoclue-gsmloc-mm.h"

#include "mm-marshal.h"

#define MM_DBUS_SERVICE       "org.freedesktop.ModemManager"
#define MM_DBUS_PATH          "/org/freedesktop/ModemManager"
#define MM_DBUS_INTERFACE     "org.freedesktop.ModemManager"
#define MM_DBUS_LOC_INTERFACE "org.freedesktop.ModemManager.Modem.Location"
#define DBUS_PROPS_INTERFACE  "org.freedesktop.DBus.Properties"
#define MM_DBUS_MODEM_INTERFACE "org.freedesktop.ModemManager.Modem"

G_DEFINE_TYPE (GeoclueGsmlocMm, geoclue_gsmloc_mm, G_TYPE_OBJECT)

#define GEOCLUE_GSMLOC_MM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEOCLUE_TYPE_GSMLOC_MM, GeoclueGsmlocMmPrivate))

typedef struct {
	char *path;

	DBusGProxy *loc_proxy;
	DBusGProxy *props_proxy;
	DBusGProxy *modem_proxy;

	gboolean got_enabled;
	gboolean enabled;
	gboolean enabling;

	gboolean got_loc_enabled;
	gboolean loc_enabled;
	gboolean loc_enabling;
	gboolean got_initial_loc;

	/* Whether the modem signals its location or whether we
	 * have to poll for it.
	 */
	gboolean signals;
	guint loc_idle;

	gboolean has_location;

	gpointer owner;
} Modem;

typedef struct {
	DBusGConnection *bus;
	DBusGProxy *dbus_proxy;

	/* Listens for device add/remove events */
	DBusGProxy *mm_proxy;
	DBusGProxy *props_proxy;

	/* List of Modem objects */
	GSList *modems;
} GeoclueGsmlocMmPrivate;

enum {
	PROP_0,
	PROP_AVAILABLE,
};

enum {
	NETWORK_DATA_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0};

#define LOC_CAP_GSM_LACCI 0x02

gboolean mm_debug = FALSE;

#define debugmsg(fmt, args...) \
	{ if (mm_debug) { g_debug (fmt, ##args); } }


static gboolean
is_available (GeoclueGsmlocMm *self)
{
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);
	GSList *iter;

	for (iter = priv->modems; iter; iter = g_slist_next (iter)) {
		Modem *modem = iter->data;

		if (modem->enabled && modem->loc_enabled && modem->has_location)
			return TRUE;
	}

	return FALSE;
}

static Modem *
find_modem (GeoclueGsmlocMm *self, const char *path)
{
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);
	GSList *iter;

	g_return_val_if_fail (path != NULL, NULL);

	for (iter = priv->modems; iter; iter = g_slist_next (iter)) {
		Modem *modem = iter->data;

		if (strcmp (path, modem->path) == 0)
			return modem;
	}

	return NULL;
}

static void
recheck_available (GeoclueGsmlocMm *self)
{
	g_object_notify (G_OBJECT (self), "available");
}

static void
location_update (GeoclueGsmlocMm *self, const char *loc)
{
	char **components = NULL;
	char *dec_lac = NULL, *dec_cid = NULL;
	unsigned long int num;

	components = g_strsplit (loc, ",", 0);
	if (!components || g_strv_length (components) < 4) {
		g_warning ("%s: invalid GSM LAC/CI location: '%s'", __func__, loc);
		goto out;
	}

	/* convert lac to decimal */
	errno = 0;
	num = strtoul (components[2], NULL, 16);
	if (errno != 0) {
		g_warning ("%s: cannot convert LAC '%s' to decimal!",
		           __func__, components[2]);
		goto out;
	}
	dec_lac = g_strdup_printf ("%u", num);

	/* convert cell id to decimal */
	errno = 0;
	num = strtoul (components[3], NULL, 16);
	if (errno != 0) {
		g_warning ("%s: cannot convert Cell ID '%s' to decimal!",
		           __func__, components[3]);
		goto out;
	}
	dec_cid = g_strdup_printf ("%u", num);

	debugmsg ("%s: emitting location: %s/%s/%s/%s",
	           __func__, components[0], components[1], dec_lac, dec_cid);
	g_signal_emit (G_OBJECT (self), signals[NETWORK_DATA_CHANGED], 0,
	               components[0],  /* MCC */
	               components[1],  /* MNC */
	               dec_lac,        /* LAC */
	               dec_cid);       /* CID */

out:
	if (components)
		g_strfreev (components);
	g_free (dec_lac);
	g_free (dec_cid);
}

static void
modem_location_update (Modem *modem, GHashTable *locations)
{
	GValue *lacci;

	/* GSMLOC only handles GSM LAC/CI location info */
	lacci = g_hash_table_lookup (locations, GUINT_TO_POINTER (LOC_CAP_GSM_LACCI));
	if (!lacci)
		return;
	if (!G_VALUE_HOLDS_STRING (lacci)) {
		g_warning ("%s: GSM LAC/CI location member not a string!", __func__);
		return;
	}

	debugmsg ("%s: GSM LAC/CI: %s", __func__, g_value_get_string (lacci));
	location_update (modem->owner, g_value_get_string (lacci));
}

#define DBUS_TYPE_LOCATIONS (dbus_g_type_get_map ("GHashTable", G_TYPE_UINT, G_TYPE_VALUE))
#define DBUS_TYPE_G_MAP_OF_VARIANT (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

static void
loc_poll_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	GError *error = NULL;
	GHashTable *locations = NULL;

	if (!dbus_g_proxy_end_call (proxy, call, &error,
	                            DBUS_TYPE_LOCATIONS, &locations,
	                            G_TYPE_INVALID)) {
		g_warning ("%s: failed to get location: (%d) %s",
		           __func__,
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}

	modem_location_update ((Modem *) user_data, locations);
	g_hash_table_destroy (locations);
}

static gboolean
modem_loc_poll (gpointer user_data)
{
	Modem *modem = user_data;

	dbus_g_proxy_begin_call (modem->loc_proxy, "GetLocation",
	                         loc_poll_cb, modem, NULL,
	                         G_TYPE_INVALID);

	return TRUE;
}

static void
modem_loc_enable_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	Modem *modem = user_data;
	GError *error = NULL;

	modem->loc_enabling = FALSE;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID)) {
		g_warning ("%s: failed to enable modem location services: (%d) %s",
		           __func__,
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}
}

static void
modem_try_loc_enable (Modem *modem)
{
	/* Don't enable location services if we don't have all the modem's
	 * status yet or if location services are already enabled.
	 */

	if (!modem->got_loc_enabled ||
	    !modem->enabled ||
	    !modem->has_location ||
	    !modem->got_loc_enabled ||
	    modem->loc_enabled ||
	    modem->loc_enabling)
		return;

	modem->loc_enabling = TRUE;
	debugmsg ("%s: (%s) enabling location services...", __func__, modem->path);
	dbus_g_proxy_begin_call (modem->loc_proxy, "Enable",
		                 modem_loc_enable_cb, modem, NULL,
		                 G_TYPE_BOOLEAN, TRUE,  /* enable */
		                 G_TYPE_BOOLEAN, TRUE,  /* signal location changes */
		                 G_TYPE_INVALID);
}

static void
modem_enable_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	Modem *modem = user_data;
	GError *error = NULL;

	modem->enabling = FALSE;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID)) {
		g_warning ("%s: failed to enable modem: (%d) %s",
		           __func__,
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}

	/* enable location services */
	modem_try_loc_enable (modem);
}

static void
modem_properties_changed (DBusGProxy *proxy,
                          const char *interface,
                          GHashTable *props,
                          gpointer user_data)
{
	Modem *modem = user_data;
	GValue *value;
	gboolean old_avail = modem->enabled && modem->loc_enabled && modem->has_location;
	gboolean new_avail;

	if (strcmp (interface, MM_DBUS_MODEM_INTERFACE) == 0) {
		value = g_hash_table_lookup (props, "Enabled");
		if (value && G_VALUE_HOLDS_BOOLEAN (value)) {
			modem->enabled = g_value_get_boolean (value);
			modem->got_enabled = TRUE;
			debugmsg ("%s: (%s) modem %s", __func__, modem->path,
			           modem->enabled ? "enabled" : "disabled");
		}
	} else if (strcmp (interface, MM_DBUS_LOC_INTERFACE) == 0) {
		value = g_hash_table_lookup (props, "Enabled");
		if (value && G_VALUE_HOLDS_BOOLEAN (value)) {
			modem->loc_enabled = g_value_get_boolean (value);
			modem->got_loc_enabled = TRUE;
			debugmsg ("%s: (%s) modem location services %s",
			           __func__, modem->path,
			           modem->loc_enabled ? "enabled" : "disabled");
		}

		value = g_hash_table_lookup (props, "SignalsLocation");
		if (value && G_VALUE_HOLDS_BOOLEAN (value)) {
			modem->signals = g_value_get_boolean (value);
			debugmsg ("%s: (%s) modem %s signal location updates",
			           __func__, modem->path,
			           modem->signals ? "will" : "does not");
		}

		value = g_hash_table_lookup (props, "Capabilities");
		if (value && G_VALUE_HOLDS_UINT (value)) {
			debugmsg ("%s: (%s) modem location capabilities: 0x%X",
			           __func__, modem->path,
			           g_value_get_uint (value));

			if (g_value_get_uint (value) & LOC_CAP_GSM_LACCI)
				modem->has_location = TRUE;
		}

		value = g_hash_table_lookup (props, "Location");
		if (value && G_VALUE_HOLDS_BOXED (value))
			modem_location_update (modem, (GHashTable *) g_value_get_boxed (value));
	}

	new_avail = modem->enabled && modem->loc_enabled && modem->has_location;

	/* If the modem doesn't signal its location, start polling for the
	 * location now.
	 */
	if (new_avail && !modem->signals && !modem->loc_idle) {
		modem->loc_idle = g_timeout_add_seconds (20, modem_loc_poll, modem);
		/* Kick off a quick location request */
		modem_loc_poll (modem);
	}

	/* If the modem is no longer enabled, or it now signals its location
	 * then we no longer need to poll.
	 */
	if ((!new_avail || modem->signals) && modem->loc_idle)
		g_source_remove (modem->loc_idle);

	/* Tell the manager to recheck availability of location info */
	if (old_avail != new_avail)
		recheck_available (modem->owner);

	/* If we've successfully retrieved modem properties and the modem
	 * isn't enabled, do that now.
	 */
	if (modem->got_enabled && !modem->enabled && !modem->enabling) {
		debugmsg ("%s: (%s) enabling...", __func__, modem->path);
		modem->enabling = TRUE;
		dbus_g_proxy_begin_call (modem->modem_proxy, "Enable",
			                 modem_enable_cb, modem, NULL,
			                 G_TYPE_BOOLEAN, TRUE, G_TYPE_INVALID);
	}

	/* If the modem was already enabled but location services weren't,
	 * enable them now.
	 */
	modem_try_loc_enable (modem);

	/* After location is enabled, try to get the location ASAP */
	if (modem->has_location && modem->loc_enabled && !modem->got_initial_loc) {
		modem->got_initial_loc = TRUE;
		modem_loc_poll (modem);
	}
}

static void
modem_props_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	GError *error = NULL;
	GHashTable *props = NULL;
	Modem *modem = user_data;

	if (!dbus_g_proxy_end_call (proxy, call, &error,
	                            DBUS_TYPE_G_MAP_OF_VARIANT, &props,
	                            G_TYPE_INVALID)) {
		g_warning ("%s: failed to get modem interface properties: (%d) %s",
		           __func__,
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}

	modem_properties_changed (modem->loc_proxy, MM_DBUS_MODEM_INTERFACE, props, modem);
	g_hash_table_destroy (props);
}

static void
loc_props_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	GError *error = NULL;
	GHashTable *props = NULL;
	Modem *modem = user_data;

	if (!dbus_g_proxy_end_call (proxy, call, &error,
	                            DBUS_TYPE_G_MAP_OF_VARIANT, &props,
	                            G_TYPE_INVALID)) {
		g_warning ("%s: failed to get location interface properties: (%d) %s",
		           __func__,
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}

	modem_properties_changed (modem->loc_proxy, MM_DBUS_LOC_INTERFACE, props, modem);
	g_hash_table_destroy (props);

	/* Now that we know the device supports location services, get basic
	 * modem properties and start grabbing location info.
	 */
	dbus_g_proxy_begin_call (modem->props_proxy, "GetAll",
	                         modem_props_cb, modem, NULL,
	                         G_TYPE_STRING, MM_DBUS_MODEM_INTERFACE, G_TYPE_INVALID);
}

static Modem *
modem_new (DBusGConnection *bus, const char *path, gpointer owner)
{
	Modem *modem;

	modem = g_slice_new0 (Modem);
	modem->owner = owner;
	modem->path = g_strdup (path);

	modem->loc_proxy = dbus_g_proxy_new_for_name (bus,
	                                              MM_DBUS_SERVICE,
	                                              path,
	                                              MM_DBUS_LOC_INTERFACE);

	modem->modem_proxy = dbus_g_proxy_new_for_name (bus,
	                                                MM_DBUS_SERVICE,
	                                                path,
	                                                MM_DBUS_MODEM_INTERFACE);

	/* Listen for property changes */
	modem->props_proxy = dbus_g_proxy_new_for_name (bus,
	                                                MM_DBUS_SERVICE,
	                                                path,
	                                                "org.freedesktop.DBus.Properties");
	dbus_g_object_register_marshaller (mm_marshal_VOID__STRING_BOXED,
	                                   G_TYPE_NONE,
	                                   G_TYPE_STRING, DBUS_TYPE_G_MAP_OF_VARIANT,
	                                   G_TYPE_INVALID);
	dbus_g_proxy_add_signal (modem->props_proxy, "MmPropertiesChanged",
	                         G_TYPE_STRING, DBUS_TYPE_G_MAP_OF_VARIANT,
	                         G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (modem->props_proxy, "MmPropertiesChanged",
	                             G_CALLBACK (modem_properties_changed),
	                             modem,
	                             NULL);

	debugmsg ("%s: (%s) modem created", __func__, path);

	/* Check if the Location interface is actually supported before doing
	 * anything with the modem, because if it's not, we don't care about
	 * the modem at all.
	 */
	dbus_g_proxy_begin_call (modem->props_proxy, "GetAll",
	                         loc_props_cb, modem, NULL,
	                         G_TYPE_STRING, MM_DBUS_LOC_INTERFACE, G_TYPE_INVALID);

	return modem;
}

static void
modem_free (Modem *modem)
{

	debugmsg ("%s: (%s) modem removed", __func__, modem->path);

	g_free (modem->path);
	g_object_unref (modem->loc_proxy);
	g_object_unref (modem->modem_proxy);
	g_object_unref (modem->props_proxy);

	if (modem->loc_idle)
		g_source_remove (modem->loc_idle);

	memset (modem, 0, sizeof (Modem));
	g_slice_free (Modem, modem);
}

static void
modem_added (DBusGProxy *proxy, const char *path, gpointer user_data)
{
	GeoclueGsmlocMm *self = GEOCLUE_GSMLOC_MM (user_data);
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);
	Modem *modem;

	if (!find_modem (self, path)) {
		modem = modem_new (priv->bus, path, self);
		priv->modems = g_slist_prepend (priv->modems, modem);
	}
}

static void
enumerate_modems_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	GPtrArray *modems;
	GError *error = NULL;
	int i;

	if (!dbus_g_proxy_end_call (proxy, call, &error,
	                            dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &modems,
	                            G_TYPE_INVALID)) {
		g_warning ("%s: failed to enumerate modems: (%d) %s",
		           __func__,
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}

	for (i = 0; i < modems->len; i++) {
		char *path = g_ptr_array_index (modems, i);

		modem_added (NULL, path, GEOCLUE_GSMLOC_MM (user_data));
		g_free (path);
	}
	g_ptr_array_free (modems, TRUE);
}

static void
enumerate_modems (GeoclueGsmlocMm *self)
{
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);

	dbus_g_proxy_begin_call (priv->mm_proxy, "EnumerateDevices",
	                         enumerate_modems_cb, self, NULL,
	                         G_TYPE_INVALID);
}

static void
modem_removed (DBusGProxy *proxy, const char *path, gpointer user_data)
{
	GeoclueGsmlocMm *self = GEOCLUE_GSMLOC_MM (user_data);
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);
	Modem *modem;

	modem = find_modem (self, path);
	if (modem) {
		gboolean old_available = is_available (self);

		priv->modems = g_slist_remove (priv->modems, modem);
		modem_free (modem);
		if (is_available (self) != old_available)
			g_object_notify (G_OBJECT (self), "available");
	}
}

static void
kill_modems (GeoclueGsmlocMm *self)
{
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);
	gboolean old_available = is_available (self);
	GSList *iter;

	/* Kill all modems */
	for (iter = priv->modems; iter; iter = g_slist_next (iter))
		modem_free ((Modem *) iter->data);
	g_slist_free (priv->modems);
	priv->modems = NULL;

	/* No more modems; clearly location is no longer available */
	if (old_available)
		g_object_notify (G_OBJECT (self), "available");
}

static void
name_owner_changed (DBusGProxy *proxy,
                    const char *name,
                    const char *old_owner,
                    const char *new_owner,
                    gpointer user_data)
{
	gboolean old_owner_good;
	gboolean new_owner_good;

	if (strcmp (MM_DBUS_SERVICE, name) != 0)
		return;

	old_owner_good = (old_owner && strlen (old_owner));
	new_owner_good = (new_owner && strlen (new_owner));

	if (!old_owner_good && new_owner_good) {
		debugmsg ("ModemManager appeared");
		enumerate_modems (GEOCLUE_GSMLOC_MM (user_data));
	} else if (old_owner_good && !new_owner_good) {
		debugmsg ("ModemManager disappeared");
		kill_modems (GEOCLUE_GSMLOC_MM (user_data));
	}
}

GeoclueGsmlocMm *
geoclue_gsmloc_mm_new (void)
{
	return (GeoclueGsmlocMm *) g_object_new (GEOCLUE_TYPE_GSMLOC_MM, NULL);
}

static gboolean
mm_alive (DBusGProxy *proxy)
{
	char *owner = NULL;
	gboolean owned = FALSE;
	GError *error = NULL;

	if (dbus_g_proxy_call_with_timeout (proxy,
	                                    "GetNameOwner", 2000, &error,
	                                    G_TYPE_STRING, MM_DBUS_SERVICE,
	                                    G_TYPE_INVALID,
	                                    G_TYPE_STRING, &owner,
	                                    G_TYPE_INVALID)) {
		owned = !!owner;
		g_free (owner);
	}
	return owned;
}

static void
geoclue_gsmloc_mm_init (GeoclueGsmlocMm *self)
{
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (self);

	if (getenv ("GEOCLUE_GSMLOC_MM_DEBUG"))
		mm_debug = TRUE;

	priv->bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);
	if (!priv->bus) {
		g_warning ("Failed to acquire a connection to the D-Bus system bus.");
		return;
	}

	priv->dbus_proxy = dbus_g_proxy_new_for_name (priv->bus,
	                                              DBUS_SERVICE_DBUS,
	                                              DBUS_PATH_DBUS,
	                                              DBUS_INTERFACE_DBUS);
	/* Handle ModemManager restarts */
	dbus_g_proxy_add_signal (priv->dbus_proxy, "NameOwnerChanged",
				 G_TYPE_STRING, G_TYPE_STRING,
				 G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->dbus_proxy, "NameOwnerChanged",
				     G_CALLBACK (name_owner_changed),
				     self, NULL);

	priv->mm_proxy = dbus_g_proxy_new_for_name (priv->bus,
	                                            MM_DBUS_SERVICE,
	                                            MM_DBUS_PATH,
	                                            MM_DBUS_INTERFACE);
	g_assert (priv->mm_proxy);

	dbus_g_proxy_add_signal (priv->mm_proxy, "DeviceAdded",
	                         DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->mm_proxy, "DeviceAdded",
	                             G_CALLBACK (modem_added), self, NULL);
	dbus_g_proxy_add_signal (priv->mm_proxy, "DeviceRemoved",
	                         DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->mm_proxy, "DeviceRemoved",
	                             G_CALLBACK (modem_removed), self, NULL);

	if (mm_alive (priv->dbus_proxy)) {
		debugmsg ("ModemManager is alive");
		enumerate_modems (self);
	}
}

static void
dispose (GObject *object)
{
	GeoclueGsmlocMmPrivate *priv = GEOCLUE_GSMLOC_MM_GET_PRIVATE (object);

	kill_modems (GEOCLUE_GSMLOC_MM (object));

	/* Stop listening to ModemManager */
	if (priv->mm_proxy) {
		g_object_unref (priv->mm_proxy);
		priv->mm_proxy = NULL;
	}

	if (priv->props_proxy) {
		g_object_unref (priv->props_proxy);
		priv->props_proxy = NULL;
	}

	if (priv->dbus_proxy) {
		g_object_unref (priv->dbus_proxy);
		priv->dbus_proxy = NULL;
	}

	G_OBJECT_CLASS (geoclue_gsmloc_mm_parent_class)->dispose (object);
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_AVAILABLE:
		g_value_set_boolean (value, is_available (GEOCLUE_GSMLOC_MM (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
geoclue_gsmloc_mm_class_init (GeoclueGsmlocMmClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GeoclueGsmlocMmPrivate));

	/* virtual methods */
	object_class->get_property = get_property;
	object_class->dispose = dispose;

	/* properties */
	g_object_class_install_property
		(object_class, PROP_AVAILABLE,
		 g_param_spec_boolean ("available",
		                       "Available",
		                       "Whether any mobile broadband device is "
		                       "providing location information at this "
		                       "time.",
		                       FALSE,
		                       G_PARAM_READABLE));

	/* signals */
	signals[NETWORK_DATA_CHANGED] =
		g_signal_new ("network-data-changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST, 0,
		              NULL, NULL,
		              mm_marshal_VOID__STRING_STRING_STRING_STRING,
		              G_TYPE_NONE, 4,
		              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}

