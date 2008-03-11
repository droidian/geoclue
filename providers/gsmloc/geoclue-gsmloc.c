/*
 * Geoclue
 * geoclue-gsmloc.c - A gsmloc.com and gammu -based Position provider
 * 
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2008 by Garmin Ltd. or its subsidiaries
 */

 /** 
  * This is mostly a proof-of-concept, and gsmloc.org itself is 
  * not too trustworthy either, rather "barely kept alive".
  * 
  * Gammu must be configured before running the provider (test by
  * running "gammu networkinfo"). Currently the first configuration 
  * in gammmu config file is used.
  **/
  
#include <config.h>

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>

#include <gammu-statemachine.h>
#include <gammu-info.h>

#include <geoclue/gc-web-service.h>
#include <geoclue/gc-provider.h>
#include <geoclue/geoclue-error.h>
#include <geoclue/gc-iface-position.h>

#define GEOCLUE_DBUS_SERVICE_GSMLOC "org.freedesktop.Geoclue.Providers.Gsmloc"
#define GEOCLUE_DBUS_PATH_GSMLOC "/org/freedesktop/Geoclue/Providers/Gsmloc"
#define GSMLOC_URL "http://gsmloc.org/api"

#define GEOCLUE_TYPE_GSMLOC (geoclue_gsmloc_get_type ())
#define GEOCLUE_GSMLOC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GSMLOC, GeoclueGsmloc))

typedef struct _GeoclueGsmloc {
	GcProvider parent;
	GMainLoop *loop;
	GcWebService *web_service;
} GeoclueGsmloc;

typedef struct _GeoclueGsmlocClass {
	GcProviderClass parent_class;
} GeoclueGsmlocClass;


static void geoclue_gsmloc_init (GeoclueGsmloc *gsmloc);
static void geoclue_gsmloc_position_init (GcIfacePositionClass  *iface);

G_DEFINE_TYPE_WITH_CODE (GeoclueGsmloc, geoclue_gsmloc, GC_TYPE_PROVIDER,
                         G_IMPLEMENT_INTERFACE (GC_TYPE_IFACE_POSITION,
                                                geoclue_gsmloc_position_init))


/* Geoclue interface implementation */
static gboolean
geoclue_gsmloc_get_status (GcIfaceGeoclue *iface,
			   GeoclueStatus  *status,
			   GError        **error)
{
	/* Assume available so long as all the requirements are satisfied
	   ie: Network is available */
	*status = GEOCLUE_STATUS_AVAILABLE;
	return TRUE;
}

static void
shutdown (GcProvider *provider)
{
	GeoclueGsmloc *gsmloc = GEOCLUE_GSMLOC (provider);
	g_main_loop_quit (gsmloc->loop);
}




static gboolean geoclue_gsmloc_get_cell (GeoclueGsmloc *gsmloc,
                                         char **mcc, 
                                         char **mnc,
                                         char **lac, 
                                         char **cid)
{
	GSM_StateMachine *state;
	GSM_NetworkInfo netinfo;
	GSM_Error error;
	INI_Section *cfg;
	int i_lac, i_cid, i, str_len;
	char **strings;
	
	state = GSM_AllocStateMachine();
	if (!state) {
		g_printerr ("Gammu GSM_AllocStateMachine failed\n");
		return FALSE;
	}
	
 	/* Find and read configuration file */
 	error = GSM_FindGammuRC (&cfg);
	if (error != ERR_NONE) {
		g_printerr ("Gammu error: %s\n", GSM_ErrorString (error));
		if (GSM_IsConnected (state)) {
			GSM_TerminateConnection (state);
		}
		return FALSE;
	}
	 
 	if (!GSM_ReadConfig (cfg, GSM_GetConfig(state, 0), 0)) {
		g_warning ("Could not read Gammu configuration\n");
		return FALSE;
	}
	
 	/* FIXME: the used configuration should be an option */
 	GSM_SetConfigNum(state, 1);
 	
	/* Connect to phone. May take up to 10 seconds */
 	error = GSM_InitConnection (state, 3); /* magic number */
	if (error != ERR_NONE) {
		g_warning ("Gammu: %s\n", GSM_ErrorString (error));
		if (GSM_IsConnected (state)) {
			GSM_TerminateConnection (state);
		}
		return FALSE;
	}
	
	error = GSM_GetNetworkInfo (state, &netinfo);
	if (error != ERR_NONE) {
		g_printerr ("Gammu error: %s\n", GSM_ErrorString (error));
		if (GSM_IsConnected (state)) {
			GSM_TerminateConnection (state);
		}
		return FALSE;
	}
	
	GSM_TerminateConnection(state);
	
	/* Turn LAC and CID into base ten */
	i_lac = 0;
	str_len = strlen ((char *)netinfo.LAC);
	for (i = 0; i < str_len; i++) {
		int shift = (str_len - i - 1) * 4;
		i_lac += (g_ascii_xdigit_value (netinfo.LAC[i]) << shift);
	}
	
	i_cid = 0;
	str_len = strlen ((char *)netinfo.CID);
	for (i = 0; i < str_len; i++) {
		int shift = (str_len - i - 1) * 4;
		i_cid += (g_ascii_xdigit_value (netinfo.CID[i]) << shift);
	}
	
	strings = g_strsplit (netinfo.NetworkCode, " ", 2);
	*mcc = strings[0];
	*mnc = strings[1];
	g_free (strings);
	
	*lac = g_strdup_printf ("%d", i_lac);
	*cid = g_strdup_printf ("%d", i_cid);
	
	/* TODO should make sure all return values have data? */
	return TRUE;
}

/* Position interface implementation */

static gboolean 
geoclue_gsmloc_get_position (GcIfacePosition        *iface,
                             GeocluePositionFields  *fields,
                             int                    *timestamp,
                             double                 *latitude,
                             double                 *longitude,
                             double                 *altitude,
                             GeoclueAccuracy       **accuracy,
                             GError                **error)
{
	GeoclueGsmloc *gsmloc;
	char *mcc, *mnc, *lac, *cid;
	
	gsmloc = (GEOCLUE_GSMLOC (iface));
	
	if (!geoclue_gsmloc_get_cell (gsmloc, &mcc, &mnc, &lac, &cid)) {
		/* Error */
		return FALSE;
	}
	
	*fields = GEOCLUE_POSITION_FIELDS_NONE;
	if (timestamp) {
		*timestamp = time (NULL);
	}
	
	if (!gc_web_service_query (gsmloc->web_service, 
	                           "action", "get",
	                           "format", "xml",
	                           "nid1", mcc,
	                           "nid2", mnc,
	                           "lac", lac,
	                           "cid", cid,
	                           NULL)) {
		g_set_error (error, GEOCLUE_ERROR, 
		             GEOCLUE_ERROR_NOT_AVAILABLE, 
		             "Web service query failed");
		
		g_free (mcc);
		g_free (mnc);
		g_free (lac);
		g_free (cid);
		return FALSE;
	}
	g_free (mcc);
	g_free (mnc);
	g_free (lac);
	g_free (cid);
	
	if (latitude && gc_web_service_get_double (gsmloc->web_service, 
	                                           latitude, "//Point/lat")) {
		*fields |= GEOCLUE_POSITION_FIELDS_LATITUDE;
	}
	if (longitude && gc_web_service_get_double (gsmloc->web_service, 
	                                            longitude, "//Point/lon")) {
		*fields |= GEOCLUE_POSITION_FIELDS_LONGITUDE;
	}
	
	if (accuracy) {
		if (*fields == GEOCLUE_POSITION_FIELDS_NONE) {
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_NONE,
							  0, 0);
		} else {
			/* Educated guess. */
			*accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_POSTALCODE,
							  0, 0);
		}
	}
	
	return TRUE;
}

static void
geoclue_gsmloc_finalize (GObject *obj)
{
	GeoclueGsmloc *gsmloc = GEOCLUE_GSMLOC (obj);
	
	g_object_unref (gsmloc->web_service);
	
	((GObjectClass *) geoclue_gsmloc_parent_class)->finalize (obj);
}


/* Initialization */

static void
geoclue_gsmloc_class_init (GeoclueGsmlocClass *klass)
{
	GcProviderClass *p_class = (GcProviderClass *)klass;
	GObjectClass *o_class = (GObjectClass *)klass;
	
	p_class->shutdown = shutdown;
	p_class->get_status = geoclue_gsmloc_get_status;
	
	o_class->finalize = geoclue_gsmloc_finalize;
}

static void
geoclue_gsmloc_init (GeoclueGsmloc *gsmloc)
{
	gc_provider_set_details (GC_PROVIDER (gsmloc), 
	                         GEOCLUE_DBUS_SERVICE_GSMLOC,
	                         GEOCLUE_DBUS_PATH_GSMLOC,
	                         "Gsmloc", "Gsmloc.org and Gammu -based provider");
	
	gsmloc->web_service = g_object_new (GC_TYPE_WEB_SERVICE, NULL);
	gc_web_service_set_base_url (gsmloc->web_service, GSMLOC_URL);
}

static void
geoclue_gsmloc_position_init (GcIfacePositionClass  *iface)
{
	iface->get_position = geoclue_gsmloc_get_position;
}

int 
main()
{
	g_type_init();
	
	GeoclueGsmloc *o = g_object_new (GEOCLUE_TYPE_GSMLOC, NULL);
	o->loop = g_main_loop_new (NULL, TRUE);
	
	g_main_loop_run (o->loop);
	
	g_main_loop_unref (o->loop);
	g_object_unref (o);
	
	return 0;
}
