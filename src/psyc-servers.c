/*
  This file is part of irssi-psyc.
  Copyright (C) 2011 Gabor Adam Toth

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "module.h"

#include <core/servers.h>
#include <core/network.h>
#include <core/net-sendbuffer.h>
#include <core/levels.h>
#include <core/signals.h>
#include <core/settings.h>
#include <fe-common/core/window-items.h>
#include <fe-common/core/printtext.h>

#include <psyc.h>
#include <psyc/client.h>

#include "psyc-servers.h"
#include "psyc-channels.h"
#include "psyc-formats.h"

#define RECVBUFLEN PSYC_MAX_PACKET_SIZE

SERVER_REC *
psyc_server_init_connect (SERVER_CONNECT_REC *conn)
{
    LOG_DEBUG(">> psyc_server_init_connect()\n");

    PSYC_SERVER_REC *server;

    g_return_val_if_fail(IS_PSYC_SERVER_CONNECT(conn), NULL);
    if (conn->address == NULL || *conn->address == '\0')
        return NULL;
    if (conn->nick == NULL || *conn->nick == '\0')
        return NULL;

    server = g_new0(PSYC_SERVER_REC, 1);
    server->chat_type = PSYC_PROTOCOL;

    server->connrec = (PSYC_SERVER_CONNECT_REC *) conn;
    server_connect_ref(SERVER_CONNECT(conn));

    server_connect_init((SERVER_REC *) server);
    return (SERVER_REC *) server;
}

void
psyc_server_connect (SERVER_REC *server)
{
    if (!server_start_connect(server)) {
        server_connect_unref(server->connrec);
        g_free(server);
    }
}

static int
is_channel (SERVER_REC *server, const char *data)
{
    LOG_DEBUG(">> is_channel(%s)\n", data);

    return TRUE;
}

static int
is_nick_flag (char flag)
{
    LOG_DEBUG(">> is_nick_flag(%c)\n", flag);

    return FALSE;
}

static const char *
get_nick_flags ()
{
    return "";
}

static void
send_message (PSYC_SERVER_REC *server, const char *target,
              const char *msg, int target_type)
{
    LOG_DEBUG(">> psyc_server:send_message(%s, %s, %d)\n", target, msg, target_type);
    psyc_client_message(server->client, target, strlen(target), msg, strlen(msg));
}

static void
transmit (PSYC_SERVER_REC *server, const char *data, size_t len)
{
    LOG_DEBUG(">> psyc_server:transmit(%lu)\n", len);

    if (!server || !IS_PSYC_SERVER(server)
        || server->disconnected || server->connection_lost || !server->handle)
        return;

    if (net_sendbuffer_send(server->handle, data, len) == -1) {
        LOG_DEBUG("transmit error: %lu\n", len);
        server->connection_lost = TRUE;
    }
}

static void
linked (PSYC_SERVER_REC *server, Packet *p)
{
    LOG_DEBUG(">> psyc_server:linked()\n");

    server->connected = TRUE;
    server->connect_time = time(NULL);
}

static void
unlinked (PSYC_SERVER_REC *server, Packet *p)
{
    LOG_DEBUG(">> psyc_server:linked()\n");

    server_disconnect((SERVER_REC*)server);
}

static void
nick_changed (PSYC_SERVER_REC *server, char *nick, size_t nicklen)
{
    LOG_DEBUG(">> psyc_server:nick_changed()\n");

    g_free(server->nick);
    server->nick = g_strdup(nick);
}

static void
receive (PSYC_SERVER_REC *server, Packet *p,
         PsycMethod mc, PsycMethod mc_family, unsigned int mc_flag,
         char *ctx, size_t ctxlen, char *ctxname, size_t ctxnamelen,
         char *uni, size_t unilen, char *nick, size_t nicklen,
         char *method, size_t methodlen, char *data, size_t datalen)
{
    LOG_DEBUG(">> psyc_server:receive(%.*s, %u, %u, %u, "
              "%.*s, %.*s, %.*s, %.*s, %.*s)\n",
              (int)S2ARG2(p->method), mc, mc_family, mc_flag,
              (int)ctxlen, ctx, (int)ctxnamelen, ctxname,
              (int)unilen, uni, (int)nicklen, nick, (int)datalen, data);

    if (!server || !IS_PSYC_SERVER(server))
        return;

    if (ctxlen > 0) {
        PSYC_CHANNEL_REC *channel = psyc_channel_find(server, ctx);
        if (!channel)
            channel = psyc_channel_create(server, ctx, ctxname, 0);

        psyc_channel_receive(server, channel, p, mc, mc_family, mc_flag,
                             uni, unilen, nick, nicklen,
                             method, methodlen, data, datalen);
    } else {
        if (settings_get_bool("psyc_debug"))
            printformat(server, NULL,
                        MSGLEVEL_CRAP | MSGLEVEL_NOHILIGHT | MSGLEVEL_NEVER,
                        PSYCTXT_PACKET, p->content.data);

        if (mc_flag & PSYC_METHOD_VISIBLE) {
            switch (mc_family) {
            case PSYC_MC_NOTICE:
                printformat(server, NULL, MSGLEVEL_NOTICES, PSYCTXT__NOTICE, data);
                break;
            case PSYC_MC_REQUEST:
                printformat(server, NULL, MSGLEVEL_INVITES | MSGLEVEL_HILIGHT,
                            PSYCTXT__REQUEST, data);
                break;
            case PSYC_MC_STATUS:
                printformat(server, NULL, MSGLEVEL_NOTICES, PSYCTXT__STATUS, data);
                break;
            default:
                printformat(server, NULL, MSGLEVEL_CRAP, PSYCTXT_DEFAULT, data);
                break;
            }
        }
        if (methodlen > 0)
            signal_emit(method, 5, server, data, nick, uni, ctx);
    }
}

static void
receive_raw (PSYC_SERVER_REC *server)
{
    LOG_DEBUG(">> psyc_server:receive_raw()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    char buf[RECVBUFLEN];
    int len = net_receive(net_sendbuffer_handle(server->handle), buf, RECVBUFLEN);
    LOG_DEBUG("len: %d\n", len);

    if (len > 0) {
        psyc_client_receive(server->client, buf, len);
    } else if (len < 0) {
        LOG_DEBUG("receive error: %d\n", len);
        server->connection_lost = TRUE;
        server_disconnect((SERVER_REC*)server);
    }
}

PsycClientEvents client_events = {
    .receive = (PsycClientReceive) receive,
    .linked = (PsycClientLink) linked,
    .unlinked = (PsycClientLink) unlinked,
};

static void
sig_server_connected (PSYC_SERVER_REC *server)
{
    LOG_DEBUG(">> psyc_server:sig_server_connected()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    server->channels_join = psyc_channel_join;
    server->ischannel = is_channel;
    server->isnickflag = (void *) is_nick_flag;
    server->get_nick_flags = (void *) get_nick_flags;
    server->send_message = send_message;

    server->client = psyc_client_create(server->nick, strlen(server->nick),
                                        &client_events, server,
                                        (PsycClientSend) transmit, server);
    if (!server->client) {
        server_disconnect((SERVER_REC*)server);
        return;
    }

    server->readtag =
        g_input_add(net_sendbuffer_handle(server->handle), G_INPUT_READ,
                    (GInputFunction) receive_raw, server);

    psyc_client_link(server->client);
}

static void
sig_server_disconnected (PSYC_SERVER_REC *server)
{
    LOG_DEBUG(">> psyc_server:sig_server_disconnected()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    psyc_client_destroy(server->client);
}

static void
sig_server_quit (PSYC_SERVER_REC *server, const char *msg)
{
    LOG_DEBUG(">> psyc_server:sig_server_quit()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    psyc_client_unlink(server->client);
}

static void
sig_server_connect_copy (SERVER_CONNECT_REC **dest, PSYC_SERVER_CONNECT_REC *src)
{
    LOG_DEBUG(">> psyc_server:sig_server_connect_copy()\n");

    PSYC_SERVER_CONNECT_REC *rec;

    g_return_if_fail(dest != NULL);
    if (!IS_PSYC_SERVER_CONNECT(src))
        return;

    rec = g_new0(PSYC_SERVER_CONNECT_REC, 1);
    rec->chat_type = PSYC_PROTOCOL;
    *dest = (SERVER_CONNECT_REC *) rec;
}

void psyc_servers_init() {
    signal_add_first("server connected", (SIGNAL_FUNC) sig_server_connected);
    signal_add_last("server disconnected", (SIGNAL_FUNC) sig_server_disconnected);
    signal_add_last("server quit", (SIGNAL_FUNC) sig_server_quit);
    signal_add("server connect copy", (SIGNAL_FUNC) sig_server_connect_copy);
}

void psyc_servers_deinit() {
    signal_remove("server connected", (SIGNAL_FUNC) sig_server_connected);
    signal_remove("server disconnected", (SIGNAL_FUNC) sig_server_disconnected);
    signal_remove("server quit", (SIGNAL_FUNC) sig_server_quit);
    signal_remove("server connect copy", (SIGNAL_FUNC) sig_server_connect_copy);
}
