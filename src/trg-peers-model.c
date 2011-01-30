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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#if HAVE_GEOIP
#include <GeoIP.h>
#endif

#include "torrent.h"
#include "tpeer.h"
#include "trg-peers-model.h"
#include "trg-model.h"
#include "util.h"

G_DEFINE_TYPE(TrgPeersModel, trg_peers_model, GTK_TYPE_LIST_STORE)

static void trg_peers_model_class_init(TrgPeersModelClass * klass)
{
}

gboolean
find_existing_peer_item_foreachfunc(GtkTreeModel * model,
				    GtkTreePath * path, GtkTreeIter * iter,
				    gpointer data)
{
    struct peerAndIter *pi;
    gchar *ip;

    pi = (struct peerAndIter *) data;

    gtk_tree_model_get(model, iter, PEERSCOL_IP, &ip, -1);
    if (g_strcmp0(ip, pi->ip) == 0) {
	pi->iter = *iter;
	pi->found = TRUE;
    }
    g_free(ip);
    return pi->found;
}

gboolean
find_existing_peer_item(TrgPeersModel * model, JsonObject * p,
			GtkTreeIter * iter)
{
    struct peerAndIter pi;
    pi.ip = peer_get_address(p);
    pi.found = FALSE;

    gtk_tree_model_foreach(GTK_TREE_MODEL(model),
			   find_existing_peer_item_foreachfunc, &pi);

    if (pi.found == TRUE)
	*iter = pi.iter;

    return pi.found;
}

static void resolved_dns_cb(GObject * source_object,
			    GAsyncResult * res, gpointer data)
{
    GtkTreeRowReference *treeRef;
    GtkTreeModel *model;
    GtkTreePath *path;

    treeRef = (GtkTreeRowReference *) data;
    model = gtk_tree_row_reference_get_model(treeRef);
    path = gtk_tree_row_reference_get_path(treeRef);

    if (path != NULL) {
	gchar *rdns =
	    g_resolver_lookup_by_address_finish(G_RESOLVER(source_object),
						res, NULL);
	if (rdns != NULL) {
	    GtkTreeIter iter;
	    if (gtk_tree_model_get_iter(model, &iter, path) == TRUE) {
		gdk_threads_enter();
		gtk_list_store_set(GTK_LIST_STORE(model),
				   &iter, PEERSCOL_HOST, rdns, -1);
		gdk_threads_leave();
	    }
	    g_free(rdns);
	}
	gtk_tree_path_free(path);
    }

    gtk_tree_row_reference_free(treeRef);
}

void trg_peers_model_update(TrgPeersModel * model, gint64 updateSerial,
			    JsonObject * t, gboolean first)
{
    JsonArray *peers;
    GtkTreeIter peerIter;
    int j;
    gboolean isNew;

    peers = torrent_get_peers(t);

    if (first == TRUE)
	gtk_list_store_clear(GTK_LIST_STORE(model));

    for (j = 0; j < json_array_get_length(peers); j++) {
	JsonObject *peer;
	const gchar *address, *flagStr;
#if HAVE_GEOIP
	GeoIP *gi;
	const gchar *country = NULL;
#endif
	peer = json_node_get_object(json_array_get_element(peers, j));

	if (first == TRUE
	    || find_existing_peer_item(model, peer, &peerIter) == FALSE) {
	    gtk_list_store_append(GTK_LIST_STORE(model), &peerIter);
	    isNew = TRUE;
	} else {
	    isNew = FALSE;
	}

	address = peer_get_address(peer);
	flagStr = peer_get_flagstr(peer);
#if HAVE_GEOIP
	if ((gi = g_object_get_data(G_OBJECT(model), "geoip")) != NULL)
	    country = GeoIP_country_name_by_addr(gi, address);
#endif
	gtk_list_store_set(GTK_LIST_STORE(model), &peerIter,
			   PEERSCOL_ICON,
			   GTK_STOCK_NETWORK, PEERSCOL_IP, address,
#if HAVE_GEOIP
			   PEERSCOL_COUNTRY,
			   country != NULL ? country : "",
#endif
			   PEERSCOL_FLAGS, flagStr,
			   PEERSCOL_PROGRESS,
			   peer_get_progress(peer),
			   PEERSCOL_DOWNSPEED,
			   peer_get_rate_to_client(peer),
			   PEERSCOL_UPSPEED,
			   peer_get_rate_to_peer(peer),
			   PEERSCOL_UPDATESERIAL, updateSerial, -1);

	if (isNew == TRUE) {
	    GtkTreePath *path;
	    GtkTreeRowReference *treeRef;
	    GInetAddress *inetAddr;
	    GResolver *resolver;

	    path =
		gtk_tree_model_get_path(GTK_TREE_MODEL(model), &peerIter);
	    treeRef =
		gtk_tree_row_reference_new(GTK_TREE_MODEL(model), path);
	    gtk_tree_path_free(path);

	    inetAddr = g_inet_address_new_from_string(address);
	    resolver = g_resolver_get_default();
	    g_resolver_lookup_by_address_async(resolver,
					       inetAddr, NULL,
					       resolved_dns_cb, treeRef);
	    g_object_unref(resolver);
	    g_object_unref(inetAddr);
	}
    }

    if (first == FALSE)
	trg_model_remove_removed(GTK_LIST_STORE(model),
				 PEERSCOL_UPDATESERIAL, updateSerial);
}

static void trg_peers_model_init(TrgPeersModel * self)
{
    GType column_types[PEERSCOL_COLUMNS];

    column_types[PEERSCOL_ICON] = G_TYPE_STRING;
    column_types[PEERSCOL_IP] = G_TYPE_STRING;
#if HAVE_GEOIP
    column_types[PEERSCOL_COUNTRY] = G_TYPE_STRING;
#endif
    column_types[PEERSCOL_HOST] = G_TYPE_STRING;
    column_types[PEERSCOL_FLAGS] = G_TYPE_STRING;
    column_types[PEERSCOL_PROGRESS] = G_TYPE_DOUBLE;
    column_types[PEERSCOL_DOWNSPEED] = G_TYPE_INT64;
    column_types[PEERSCOL_UPSPEED] = G_TYPE_INT64;
    column_types[PEERSCOL_UPDATESERIAL] = G_TYPE_INT64;

    gtk_list_store_set_column_types(GTK_LIST_STORE(self),
				    PEERSCOL_COLUMNS, column_types);

#if HAVE_GEOIP
    if (g_file_test(TRG_GEOIP_DATABASE, G_FILE_TEST_EXISTS) == TRUE) {
	GeoIP *gi = GeoIP_open(TRG_GEOIP_DATABASE,
			       GEOIP_STANDARD | GEOIP_CHECK_CACHE);
	g_object_set_data(G_OBJECT(self), "geoip", gi);
    }
#endif
}


TrgPeersModel *trg_peers_model_new()
{
    GObject *obj = g_object_new(TRG_TYPE_PEERS_MODEL, NULL);

    return TRG_PEERS_MODEL(obj);
}