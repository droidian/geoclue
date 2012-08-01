
#include <connectivity.h>

static void
print_ap (gpointer key,
	  gpointer value,
	  gpointer data)
{
	g_message ("\t%s : %d dBm",
		   key,
		   GPOINTER_TO_INT (value));
}

static void
print_aps (GeoclueConnectivity *conn)
{
	GHashTable *ht;

	ht = geoclue_connectivity_get_aps (conn);
	if (ht == NULL) {
		g_message ("No Access Points available");
		return;
	}
	g_message ("APs:");
	g_hash_table_foreach (ht, print_ap, NULL);
}

static void
print_if_avail (GeoclueConnectivity *self,
		GeoclueNetworkStatus status)
{
	char *router, *ap;
	if (status != GEOCLUE_CONNECTIVITY_ONLINE)
		return;
	print_aps (self);
	ap = geoclue_connectivity_get_ap_mac (self);
	g_message ("AP is '%s'", ap ? ap : "Unavailable");
	g_free (ap);

	router = geoclue_connectivity_get_router_mac (self);
	g_message ("Router is '%s'", router);
	g_free (router);
}

static void
status_changed_cb (GeoclueConnectivity *self,
		   GeoclueNetworkStatus status,
		   gpointer data)
{
	const char *str;

	switch (status) {
	case GEOCLUE_CONNECTIVITY_UNKNOWN:
		str = "GEOCLUE_CONNECTIVITY_UNKNOWN";
		break;
	case GEOCLUE_CONNECTIVITY_OFFLINE:
		str = "GEOCLUE_CONNECTIVITY_OFFLINE";
		break;
	case GEOCLUE_CONNECTIVITY_ACQUIRING:
		str = "GEOCLUE_CONNECTIVITY_ACQUIRING";
		break;
	case GEOCLUE_CONNECTIVITY_ONLINE:
		str = "GEOCLUE_CONNECTIVITY_ONLINE";
		break;
	default:
		g_assert_not_reached ();
	}

	g_message ("Connectivity status switch to '%s'", str);

	print_if_avail (self, status);
}

int main (int argc, char **argv)
{
	GMainLoop *mainloop;
	GeoclueConnectivity *conn;
	char *router;

	g_type_init ();
	mainloop = g_main_loop_new (NULL, FALSE);
	conn = geoclue_connectivity_new ();

	if (conn == NULL) {
		router = geoclue_connectivity_get_router_mac (conn);
		g_message ("Router MAC is detected as '%s'", router ? router : "empty");

		return 1;
	}
	print_if_avail (conn, geoclue_connectivity_get_status (conn));
	g_signal_connect (conn, "status-changed",
			  G_CALLBACK (status_changed_cb), NULL);

	g_main_loop_run (mainloop);

	return 0;
}
