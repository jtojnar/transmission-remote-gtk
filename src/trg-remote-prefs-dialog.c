/*
 * transmission-remote-gtk - Transmission RPC client for GTK
 * Copyright (C) 2011  Alan Fitton

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

#include "trg-main-window.h"
#include "trg-remote-prefs-dialog.h"
#include "hig.h"
#include "dispatch.h"
#include "requests.h"
#include "json.h"
#include "trg-json-widgets.h"
#include "session-get.h"

G_DEFINE_TYPE(TrgRemotePrefsDialog, trg_remote_prefs_dialog,
	      GTK_TYPE_DIALOG)
#define TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRG_TYPE_REMOTE_PREFS_DIALOG, TrgRemotePrefsDialogPrivate))
enum {
    PROP_0,
    PROP_PARENT,
    PROP_CLIENT
};

typedef struct _TrgRemotePrefsDialogPrivate TrgRemotePrefsDialogPrivate;

struct _TrgRemotePrefsDialogPrivate {
    trg_client *client;
    TrgMainWindow *parent;

    GtkWidget *done_script_entry;
    GtkWidget *done_script_enabled_check;
    GtkWidget *pex_enabled_check;
    GtkWidget *lpd_enabled_check;
    GtkWidget *download_dir_entry;
    GtkWidget *peer_port_random_check;
    GtkWidget *peer_port_spin;
    GtkWidget *peer_limit_global_spin;
    GtkWidget *peer_limit_per_torrent_spin;
    GtkWidget *peer_port_forwarding_check;
    GtkWidget *blocklist_url_entry;
    GtkWidget *blocklist_check;
    GtkWidget *rename_partial_files_check;
    GtkWidget *encryption_combo;
    GtkWidget *incomplete_dir_entry;
    GtkWidget *incomplete_dir_check;
    GtkWidget *seed_ratio_limit_check;
    GtkWidget *seed_ratio_limit_spin;
    GtkWidget *cache_size_mb_spin;
    GtkWidget *start_added_torrent_check;
    GtkWidget *trash_original_torrent_files_check;
    GtkWidget *speed_limit_down_check;
    GtkWidget *speed_limit_down_spin;
    GtkWidget *speed_limit_up_check;
    GtkWidget *speed_limit_up_spin;
};

static GObject *instance = NULL;

static void update_session(GtkDialog * dlg)
{
    TrgRemotePrefsDialogPrivate *priv =
	TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(dlg);
    JsonNode *request = session_set();
    JsonObject *args = node_get_arguments(request);
    gchar *encryption;

    /* General */

    gtk_entry_json_output(GTK_ENTRY(priv->download_dir_entry), args);
    gtk_entry_json_output(GTK_ENTRY(priv->incomplete_dir_entry), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->done_script_enabled_check), args);
    gtk_entry_json_output(GTK_ENTRY(priv->done_script_entry), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->rename_partial_files_check), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->incomplete_dir_check), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->trash_original_torrent_files_check),
			       args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->start_added_torrent_check), args);
    gtk_spin_button_json_int_out(GTK_SPIN_BUTTON(priv->cache_size_mb_spin),
				 args);

    /* Connection */

    encryption =
	g_ascii_strdown(gtk_combo_box_get_active_text
			(GTK_COMBO_BOX(priv->encryption_combo)), -1);
    json_object_set_string_member(args, SGET_ENCRYPTION, encryption);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->peer_port_random_check), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->peer_port_forwarding_check), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->pex_enabled_check), args);
    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->lpd_enabled_check), args);
    gtk_spin_button_json_int_out(GTK_SPIN_BUTTON(priv->peer_port_spin),
				 args);

    /* Limits */

    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->seed_ratio_limit_check), args);
    gtk_spin_button_json_double_out(GTK_SPIN_BUTTON
				    (priv->seed_ratio_limit_spin), args);

    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->speed_limit_down_check), args);
    gtk_spin_button_json_int_out(GTK_SPIN_BUTTON
				 (priv->speed_limit_down_spin), args);

    gtk_toggle_button_json_out(GTK_TOGGLE_BUTTON
			       (priv->speed_limit_up_check), args);
    gtk_spin_button_json_int_out(GTK_SPIN_BUTTON
				 (priv->speed_limit_up_spin), args);

    gtk_spin_button_json_int_out(GTK_SPIN_BUTTON
				 (priv->peer_limit_global_spin), args);
    gtk_spin_button_json_int_out(GTK_SPIN_BUTTON
				 (priv->peer_limit_per_torrent_spin),
				 args);

    g_free(encryption);

    dispatch_async(priv->client, request, on_session_set, priv->parent);
}

static void
trg_remote_prefs_response_cb(GtkDialog * dlg, gint res_id, gpointer data)
{
    if (res_id == GTK_RESPONSE_ACCEPT) {
	update_session(dlg);
    }
    gtk_widget_destroy(GTK_WIDGET(dlg));
    instance = NULL;
}

static void
trg_remote_prefs_dialog_get_property(GObject * object, guint property_id,
				     GValue * value, GParamSpec * pspec)
{
    TrgRemotePrefsDialogPrivate *priv =
	TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_PARENT:
	g_value_set_object(value, priv->parent);
	break;
    case PROP_CLIENT:
	g_value_set_pointer(value, priv->client);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void
trg_remote_prefs_dialog_set_property(GObject * object, guint property_id,
				     const GValue * value,
				     GParamSpec * pspec)
{
    TrgRemotePrefsDialogPrivate *priv =
	TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_PARENT:
	priv->parent = g_value_get_object(value);
	break;
    case PROP_CLIENT:
	priv->client = g_value_get_pointer(value);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static GtkWidget *trg_rprefs_limitsPage(TrgRemotePrefsDialog * win,
					JsonObject * json)
{
    TrgRemotePrefsDialogPrivate *priv =
	TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(win);
    GtkWidget *w, *tb, *t;
    gint row = 0;

    t = hig_workarea_create();

    hig_workarea_add_section_title(t, &row, "Bandwidth");

    tb = priv->speed_limit_down_check = gtk_check_button_new_with_mnemonic
	("Limit download speed (KB/s)");
    widget_set_json_key(tb, SGET_SPEED_LIMIT_DOWN_ENABLED);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb),
				 session_get_speed_limit_down_enabled
				 (json));

    w = priv->speed_limit_down_spin =
	gtk_spin_button_new_with_range(0, INT_MAX, 5);
    widget_set_json_key(w, SGET_SPEED_LIMIT_DOWN);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_speed_limit_down(json));
    g_signal_connect(G_OBJECT(tb), "toggled",
		     G_CALLBACK(toggle_active_arg_is_sensitive), w);
    gtk_widget_set_sensitive(w,
			     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
							  (tb)));
    hig_workarea_add_row_w(t, &row, tb, w, NULL);

    tb = priv->speed_limit_up_check = gtk_check_button_new_with_mnemonic
	("Limit upload speed (KB/s)");
    widget_set_json_key(tb, SGET_SPEED_LIMIT_UP_ENABLED);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb),
				 session_get_speed_limit_up_enabled(json));

    w = priv->speed_limit_up_spin =
	gtk_spin_button_new_with_range(0, INT_MAX, 5);
    widget_set_json_key(w, SGET_SPEED_LIMIT_UP);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_speed_limit_up(json));
    g_signal_connect(G_OBJECT(tb), "toggled",
		     G_CALLBACK(toggle_active_arg_is_sensitive), w);
    gtk_widget_set_sensitive(w,
			     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
							  (tb)));
    hig_workarea_add_row_w(t, &row, tb, w, NULL);

    hig_workarea_add_section_title(t, &row, "Seeding");

    tb = priv->seed_ratio_limit_check = gtk_check_button_new_with_mnemonic
	("Seed ratio limit");
    widget_set_json_key(tb, SGET_SEED_RATIO_LIMITED);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb),
				 session_get_seed_ratio_limited(json));

    w = priv->seed_ratio_limit_spin =
	gtk_spin_button_new_with_range(0, INT_MAX, 0.1);
    widget_set_json_key(w, SGET_SEED_RATIO_LIMIT);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_seed_ratio_limit(json));
    gtk_widget_set_sensitive(w,
			     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
							  (tb)));
    g_signal_connect(G_OBJECT(tb), "toggled",
		     G_CALLBACK(toggle_active_arg_is_sensitive), w);
    hig_workarea_add_row_w(t, &row, tb, w, NULL);

    hig_workarea_add_section_title(t, &row, "Peers");

    w = priv->peer_limit_global_spin =
	gtk_spin_button_new_with_range(0, INT_MAX, 5);
    widget_set_json_key(GTK_WIDGET(w), SGET_PEER_LIMIT_GLOBAL);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_peer_limit_global(json));
    hig_workarea_add_row(t, &row, "Global peer limit", w, w);

    w = priv->peer_limit_per_torrent_spin =
	gtk_spin_button_new_with_range(0, INT_MAX, 5);
    widget_set_json_key(GTK_WIDGET(w), SGET_PEER_LIMIT_PER_TORRENT);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_peer_limit_per_torrent(json));
    hig_workarea_add_row(t, &row, "Per torrent peer limit", w, w);

    return t;
}

static GtkWidget *trg_rprefs_connPage(TrgRemotePrefsDialog * win,
				      JsonObject * s)
{
    TrgRemotePrefsDialogPrivate *priv =
	TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(win);

    GtkWidget *w, *t;
    const gchar *encryption;
    gint row = 0;

    t = hig_workarea_create();

    w = priv->encryption_combo = gtk_combo_box_new_text();
    widget_set_json_key(GTK_WIDGET(w), SGET_ENCRYPTION);
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), "Required");
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), "Preferred");
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), "Tolerated");
    encryption = session_get_encryption(s);
    if (g_strcmp0(encryption, "required") == 0) {
	gtk_combo_box_set_active(GTK_COMBO_BOX(w), 0);
    } else if (g_strcmp0(encryption, "tolerated") == 0) {
	gtk_combo_box_set_active(GTK_COMBO_BOX(w), 2);
    } else {
	gtk_combo_box_set_active(GTK_COMBO_BOX(w), 1);
    }
    hig_workarea_add_row(t, &row, "Encryption", w, NULL);

    w = priv->peer_port_spin = gtk_spin_button_new_with_range(0, 65535, 1);
    widget_set_json_key(w, SGET_PEER_PORT);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_peer_port(s));
    hig_workarea_add_row(t, &row, "Peer port", w, w);

    w = priv->peer_port_random_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Random peer port on start",
					  session_get_peer_port_random(s));
    widget_set_json_key(w, SGET_PEER_PORT_RANDOM_ON_START);

    w = priv->peer_port_forwarding_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Peer port forwarding",
					  session_get_port_forwarding_enabled
					  (s));
    widget_set_json_key(w, SGET_PORT_FORWARDING_ENABLED);

    w = priv->pex_enabled_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Peer exchange (PEX)",
					  session_get_pex_enabled(s));
    widget_set_json_key(w, SGET_PEX_ENABLED);

    w = priv->lpd_enabled_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Local peer discovery",
					  session_get_lpd_enabled(s));
    widget_set_json_key(w, SGET_LPD_ENABLED);

    return t;
}

static GtkWidget *trg_rprefs_generalPage(TrgRemotePrefsDialog * win,
					 JsonObject * s)
{
    TrgRemotePrefsDialogPrivate *priv =
	TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(win);

    GtkWidget *w, *tb, *t;
    gint row = 0;

    t = hig_workarea_create();

    w = priv->download_dir_entry = gtk_entry_new();
    widget_set_json_key(w, SGET_DOWNLOAD_DIR);
    gtk_entry_set_text(GTK_ENTRY(w), session_get_download_dir(s));
    hig_workarea_add_row(t, &row, "Download directory", w, NULL);

    tb = priv->incomplete_dir_check =
	gtk_check_button_new_with_mnemonic("Incomplete download dir");
    widget_set_json_key(tb, SGET_INCOMPLETE_DIR_ENABLED);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb),
				 session_get_incomplete_dir_enabled(s));

    w = priv->incomplete_dir_entry = gtk_entry_new();
    widget_set_json_key(w, SGET_INCOMPLETE_DIR);
    gtk_entry_set_text(GTK_ENTRY(w), session_get_incomplete_dir(s));
    gtk_widget_set_sensitive(w,
			     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
							  (tb)));
    g_signal_connect(G_OBJECT(tb), "toggled",
		     G_CALLBACK(toggle_active_arg_is_sensitive), w);
    hig_workarea_add_row_w(t, &row, tb, w, NULL);

    tb = priv->done_script_enabled_check =
	gtk_check_button_new_with_mnemonic("Torrent done script");
    widget_set_json_key(tb, SGET_SCRIPT_TORRENT_DONE_ENABLED);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb),
				 session_get_torrent_done_enabled(s));

    w = priv->done_script_entry = gtk_entry_new();
    widget_set_json_key(w, SGET_SCRIPT_TORRENT_DONE_FILENAME);
    gtk_entry_set_text(GTK_ENTRY(w), session_get_torrent_done_filename(s));
    gtk_widget_set_sensitive(w,
			     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
							  (tb)));
    g_signal_connect(G_OBJECT(tb), "toggled",
		     G_CALLBACK(toggle_active_arg_is_sensitive), w);
    hig_workarea_add_row_w(t, &row, tb, w, NULL);

    w = priv->cache_size_mb_spin =
	gtk_spin_button_new_with_range(0, INT_MAX, 1);
    widget_set_json_key(w, SGET_CACHE_SIZE_MB);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
			      session_get_cache_size_mb(s));
    hig_workarea_add_row(t, &row, "Cache size (MB)", w, w);

    w = priv->rename_partial_files_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Rename partial files",
					  session_get_rename_partial_files
					  (s));
    widget_set_json_key(w, SGET_RENAME_PARTIAL_FILES);

    w = priv->trash_original_torrent_files_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Trash original torrent files",
					  session_get_trash_original_torrent_files
					  (s));
    widget_set_json_key(w, SGET_TRASH_ORIGINAL_TORRENT_FILES);

    w = priv->start_added_torrent_check =
	hig_workarea_add_wide_checkbutton(t, &row,
					  "Start added torrents",
					  session_get_start_added_torrents
					  (s));
    widget_set_json_key(w, SGET_START_ADDED_TORRENTS);

    return t;
}

static GObject *trg_remote_prefs_dialog_constructor(GType type,
						    guint
						    n_construct_properties,
						    GObjectConstructParam
						    * construct_params)
{
    GObject *object;
    TrgRemotePrefsDialogPrivate *priv;
    JsonObject *session;
    GtkWidget *notebook;

    object = G_OBJECT_CLASS
	(trg_remote_prefs_dialog_parent_class)->constructor(type,
							    n_construct_properties,
							    construct_params);
    priv = TRG_REMOTE_PREFS_DIALOG_GET_PRIVATE(object);
    session = priv->client->session;

    gtk_window_set_title(GTK_WINDOW(object), "Remote Preferences");
    gtk_window_set_transient_for(GTK_WINDOW(object),
				 GTK_WINDOW(priv->parent));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(object), TRUE);

    gtk_dialog_add_button(GTK_DIALOG(object), GTK_STOCK_CLOSE,
			  GTK_RESPONSE_CLOSE);
    gtk_dialog_add_button(GTK_DIALOG(object), GTK_STOCK_APPLY,
			  GTK_RESPONSE_ACCEPT);

    gtk_container_set_border_width(GTK_CONTAINER(object), GUI_PAD);

    gtk_dialog_set_default_response(GTK_DIALOG(object),
				    GTK_RESPONSE_CLOSE);

    gtk_dialog_set_alternative_button_order(GTK_DIALOG(object),
					    GTK_RESPONSE_ACCEPT,
					    GTK_RESPONSE_CLOSE, -1);

    g_signal_connect(G_OBJECT(object),
		     "response",
		     G_CALLBACK(trg_remote_prefs_response_cb), NULL);

    notebook = gtk_notebook_new();

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			     trg_rprefs_generalPage
			     (TRG_REMOTE_PREFS_DIALOG(object),
			      session), gtk_label_new("General"));

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			     trg_rprefs_connPage
			     (TRG_REMOTE_PREFS_DIALOG(object),
			      session), gtk_label_new("Connections"));

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			     trg_rprefs_limitsPage
			     (TRG_REMOTE_PREFS_DIALOG(object),
			      session), gtk_label_new("Limits"));

    gtk_container_set_border_width(GTK_CONTAINER(notebook), GUI_PAD);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(object)->vbox), notebook,
		       TRUE, TRUE, 0);

    return object;
}

static void
trg_remote_prefs_dialog_class_init(TrgRemotePrefsDialogClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(klass, sizeof(TrgRemotePrefsDialogPrivate));

    object_class->constructor = trg_remote_prefs_dialog_constructor;
    object_class->get_property = trg_remote_prefs_dialog_get_property;
    object_class->set_property = trg_remote_prefs_dialog_set_property;

    g_object_class_install_property(object_class,
				    PROP_CLIENT,
				    g_param_spec_pointer
				    ("trg-client", "TClient",
				     "Client",
				     G_PARAM_READWRITE |
				     G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_NAME |
				     G_PARAM_STATIC_NICK |
				     G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
				    PROP_PARENT,
				    g_param_spec_object
				    ("parent-window", "Parent window",
				     "Parent window",
				     TRG_TYPE_MAIN_WINDOW,
				     G_PARAM_READWRITE |
				     G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_NAME |
				     G_PARAM_STATIC_NICK |
				     G_PARAM_STATIC_BLURB));
}

static void trg_remote_prefs_dialog_init(TrgRemotePrefsDialog * self)
{
}

TrgRemotePrefsDialog *trg_remote_prefs_dialog_get_instance(TrgMainWindow *
							   parent,
							   trg_client *
							   client)
{
    if (instance == NULL) {
	instance = g_object_new(TRG_TYPE_REMOTE_PREFS_DIALOG,
				"parent-window", parent,
				"trg-client", client, NULL);
    }

    return TRG_REMOTE_PREFS_DIALOG(instance);
}