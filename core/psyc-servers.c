/*
 * Copyright (C) 2008 Gabor Adam TOTH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "module.h"
#include "signals.h"
#include "psyc-servers.h"

SERVER_REC * psyc_server_init_connect(SERVER_CONNECT_REC *conn) {
  PSYC_SERVER_REC *server;
  gchar *str;

  g_return_val_if_fail(IS_PSYC_SERVER_CONNECT(conn), NULL);
  if (conn->address == NULL || *conn->address == '\0') return NULL;
  if (conn->nick == NULL || *conn->nick == '\0') return NULL;

  server = g_new0(PSYC_SERVER_REC, 1);
  server->chat_type = PSYC_PROTOCOL;

  server->connrec = (PSYC_SERVER_CONNECT_REC *) conn;
  server_connect_ref(SERVER_CONNECT(conn));

  /* Don't use irssi's sockets */
  server->connrec->no_connect = TRUE;

  server_connect_init((SERVER_REC *) server);

  signal_emit("psyc server init connect", 1, (SERVER_REC *)server);
  return (SERVER_REC *) server;
}

void psyc_server_connect(SERVER_REC *server) {
  signal_emit("server looking", 1, server);
}

static void psyc_channels_join(SERVER_REC *server, const char *channel, int automatic) {
}

static int ischannel_func(SERVER_REC *server, const char *data) {
  /*fprintf(stderr, "##psyc_ischannel(%s)\n", data);*/
  return FALSE;
}

static int isnickflag_func(char flag) {
  /*fprintf(stderr, "##psyc_isnickflag(%s)\n", flag);*/
  return FALSE;
}

static const char *get_nick_flags(void) {
  return "";
}

static void send_message(SERVER_REC *server, const char *target, const char *msg, int target_type) {
  /*fprintf(stderr, "##psyc_sendmessage(%s, %s)\n", target, msg);*/
}

static void sig_server_connected(PSYC_SERVER_REC *server) {
  if (!server || !IS_PSYC_SERVER(server)) return;

  server->channels_join = psyc_channels_join;
  server->ischannel = ischannel_func;
  server->isnickflag = (void *) isnickflag_func;
  server->get_nick_flags = (void *) get_nick_flags;
  server->send_message = send_message;
    
  /* Connection to server finished, fill the rest of the fields */
  server->connected = TRUE;
  server->connect_time = time(NULL);

  servers = g_slist_append(servers, server);
  signal_emit("event connected", 1, server);
}

static void sig_server_connect_copy(SERVER_CONNECT_REC **dest, PSYC_SERVER_CONNECT_REC *src) {
  PSYC_SERVER_CONNECT_REC *rec;

  g_return_if_fail(dest != NULL);
  if (!IS_PSYC_SERVER_CONNECT(src)) return;

  rec = g_new0(PSYC_SERVER_CONNECT_REC, 1);
  rec->chat_type = PSYC_PROTOCOL;
  *dest = (SERVER_CONNECT_REC *) rec;
}

void psyc_servers_init() {
  signal_add_first("server connected", (SIGNAL_FUNC) sig_server_connected);
  signal_add("server connect copy", (SIGNAL_FUNC) sig_server_connect_copy);
}

void psyc_servers_deinit() {
  signal_remove("server connected", (SIGNAL_FUNC) sig_server_connected);
  signal_remove("server connect copy", (SIGNAL_FUNC) sig_server_connect_copy);
}
