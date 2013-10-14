/* vim: set et ts=8 sw=8: */
/* geoip-update.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2013 Satabdi Das
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Satabdi Das <satabdidas@gmail.com>
 *          Bastien Nocera <hadess@hadess.net>
 *          Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <libsoup/soup.h>

static struct {
        const char *uri;
        const char *db_gz_name;
        const char *db_name;
} db_info_map[] = {
        {"http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz",
         "GeoLiteCity.dat.gz",
         "GeoLiteCity.dat"
        },
        {"http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz",
         "GeoIP.dat.gz",
         "GeoIP.dat"
        }
};

/* local_db_needs_update function returns TRUE on success and FALSE on failure.
 * It sets the parameter needs_update to TRUE if the local database needs
 * to be updated.
 */
static gboolean
local_db_needs_update (SoupSession *session,
                       const char  *db_uri,
                       GFile       *db_local,
                       gboolean    *needs_update,
                       GError     **error)
{
        GFileInfo *db_local_info;
        SoupMessage *msg;
        SoupDate *date;
        const gchar *db_time_str;
        guint64 db_time;
        guint64 db_local_time;
        guint status_code;

        if (g_file_query_exists (db_local, NULL) == FALSE) {
                *needs_update = TRUE;
                return TRUE;
        }

        msg = soup_message_new ("HEAD", db_uri);
        status_code = soup_session_send_message (session, msg);
        if (status_code != SOUP_STATUS_OK) {
                g_set_error_literal (error,
                                     SOUP_HTTP_ERROR,
                                     status_code,
                                     msg->reason_phrase);
                return FALSE;
        }

        db_time_str = soup_message_headers_get_one (msg->response_headers, "Last-Modified");
        date = soup_date_new_from_string (db_time_str);
        db_time = (guint64) soup_date_to_time_t (date);
        soup_date_free (date);
        g_object_unref (msg);

        db_local_info = g_file_query_info (db_local,
                                           "time::modified",
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL,
                                           error);
        if (!db_local_info)
                return FALSE;

        db_local_time = g_file_info_get_attribute_uint64 (db_local_info, "time::modified");
        if (db_time <= db_local_time)
                *needs_update = FALSE;
        else
                *needs_update = TRUE;

        g_object_unref (db_local_info);

        return TRUE;
}

static void
decompress_db (GFile *db, const char *path)
{
        GFile *db_decomp;
        GFile *db_decomp_tmp;
        GError *error = NULL;
        GZlibDecompressor *conv;
        GInputStream *instream;
        GInputStream *instream_conv;
        GOutputStream *outstream;
        char *tmp_db_path;

        instream = (GInputStream *) g_file_read (db, NULL, &error);
        if (!instream) {
                g_print ("Error opening file: %s", error->message);
                g_error_free (error);
                return;
        }

        conv = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
        instream_conv = (GInputStream *) g_converter_input_stream_new (instream, G_CONVERTER (conv));
        g_object_unref (instream);

        tmp_db_path = g_strdup_printf ("%s.%s", path, "tmp");
        db_decomp_tmp =  g_file_new_for_path (tmp_db_path);
        g_free (tmp_db_path);
        outstream = (GOutputStream *) g_file_replace (db_decomp_tmp,
                                                      NULL,
                                                      FALSE,
                                                      G_FILE_CREATE_NONE,
                                                      NULL,
                                                      &error);
        if (!outstream) {
                g_print ("Error creating file: %s\n", error->message);
                g_error_free (error);
                goto end;
        }

        if (g_output_stream_splice (outstream,
                                    instream_conv,
                                    0,
                                    NULL,
                                    &error) == -1) {
                g_print ("Error decompressing the database: %s\n", error->message);
                g_object_unref (outstream);
                g_error_free (error);
                goto end;
        }

        if (g_output_stream_close (outstream,
                                   NULL,
                                   &error) == FALSE) {
                g_print ("Error decompressing the database: %s\n", error->message);
                g_error_free (error);
        } else {
                db_decomp = g_file_new_for_path (path);
                if (g_file_move (db_decomp_tmp,
                                 db_decomp,
                                 G_FILE_COPY_OVERWRITE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &error) == FALSE) {
                        g_print ("Error moving the temporary database file to the" \
                                 " original database file: %s\n", error->message);
                        g_error_free (error);
                } else {
                        g_print ("Database updated\n");
                }

                g_object_unref (db_decomp);
        }
        g_object_unref (outstream);

end:
        g_object_unref (db_decomp_tmp);
        g_object_unref (conv);
        g_object_unref (instream_conv);
}

int
main (int argc, char **argv)
{
        SoupSession *session;
        GError *error = NULL;
        const char *path, *dbpath = NULL;
        guint i;
        GOptionContext *context;
        const GOptionEntry entries[] = {
                { "dbpath", 0, 0, G_OPTION_ARG_STRING, &dbpath, "The directory containing the databases", NULL },
                { NULL }
        };

#if (!GLIB_CHECK_VERSION (2, 36, 0))
        g_type_init ();
#endif

        context = g_option_context_new ("- updates the city and country databases from Maxmind.");
        g_option_context_add_main_entries (context, entries, NULL);
        if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
                g_print ("Option parsing failed: %s\n", error->message);
                return 1;
        }

        path = g_getenv ("GEOIP_DATABASE_PATH");
        if (!path) {
                if (dbpath)
                        path = dbpath;
                else
                        path = GEOIP_DATABASE_PATH;
        }

        session = soup_session_new ();

        for (i = 0; i < G_N_ELEMENTS (db_info_map); i++) {
                SoupMessage *msg = NULL;
                const char*db_uri;
                GFile *db_local;
                char *db_path;
                char *db_decompressed_path;
                gboolean needs_update;
                guint status_code;

                g_print ("Updating %s database\n", db_info_map[i].db_name);
                db_uri = db_info_map[i].uri;

                db_path = g_build_filename (path, db_info_map[i].db_gz_name, NULL);
                db_local = g_file_new_for_path (db_path);
                g_free (db_path);

                if (local_db_needs_update (session, db_uri, db_local, &needs_update, &error) == FALSE) {
                        g_print ("Could not update the database: %s\n", error->message);
                        g_error_free (error);
                        goto end_loop;
                }
                if (!needs_update) {
                        g_print ("Database '%s' up to date\n", db_info_map[i].db_name);
                        goto end_loop;
                }

                msg = soup_message_new ("GET", db_uri);
                status_code = soup_session_send_message (session, msg);
                if (status_code != SOUP_STATUS_OK) {
                        g_set_error (&error,
                                     SOUP_HTTP_ERROR,
                                     status_code,
                                     "%s", msg->reason_phrase);
                        g_print ("Could not download the database: %s\n", msg->reason_phrase);
                        goto end_loop;
                }

                if (g_file_replace_contents (db_local,
                                             msg->response_body->data,
                                             msg->response_body->length,
                                             NULL,
                                             FALSE,
                                             0,
                                             NULL,
                                             NULL,
                                             &error) == FALSE) {
                        g_print ("Could not download the database: %s\n", error->message);
                        goto end_loop;
                }

                db_decompressed_path = g_build_filename (path, db_info_map[i].db_name, NULL);
                decompress_db (db_local, db_decompressed_path);
                g_free (db_decompressed_path);

end_loop:
                g_object_unref (db_local);
                g_clear_object (&msg);
                error = NULL;
        }

        g_object_unref (session);

        return 0;
}
