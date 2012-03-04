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
#include "psyc-commands.h"
#include "psyc-formats.h"

#define RECVBUFLEN PSYC_MAX_PACKET_SIZE

SERVER_REC *
psyc_server_init_connect (SERVER_CONNECT_REC *conn)
{
    LOG_INFO(">> psyc_server_init_connect()\n");

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
    LOG_INFO(">> is_channel(%s)\n", data);

    return TRUE;
}

static int
is_nick_flag (char flag)
{
    LOG_INFO(">> is_nick_flag(%c)\n", flag);

    return FALSE;
}

static const char *
get_nick_flags ()
{
    return "";
}

static PsycRC
transmit (PSYC_SERVER_REC *server, const char *data, size_t len)
{
    LOG_INFO(">> psyc_server:transmit(%lu)\n", len);

    if (!server || !IS_PSYC_SERVER(server)
        || server->disconnected || server->connection_lost || !server->handle)
        return PSYC_ERROR;

    if (net_sendbuffer_send(server->handle, data, len) == -1) {
        LOG_INFO("transmit error: %lu\n", len);
        server->connection_lost = TRUE;
    }

    return PSYC_OK;
}

static void
linked (PSYC_SERVER_REC *server)
{
    server->connected = TRUE;
    server->connect_time = time(NULL);
}

static void
unlinked (PSYC_SERVER_REC *server)
{
    server_disconnect((SERVER_REC*)server);
}

static void
alias_add (PSYC_SERVER_REC *server, uint8_t own,
           const char *uni, size_t unilen,
           const char *nick, size_t nicklen)
{
    LOG_INFO(">> psyc_server:alias_add(%.*s, %.*s, %d)\n",
              (int)unilen, uni, (int)nicklen, nick, own);

    if (own) {
        g_free(server->nick);
        server->nick = g_strdup(nick);
        signal_emit("server nick changed", 1, server);
    } else {
        /// @todo change nick in all channels
    }
}

static void
alias_remove (PSYC_SERVER_REC *server, uint8_t own,
              const char *uni, size_t unilen,
              const char *nick, size_t nicklen,
	      const char *newnick, size_t newnicklen)
{
    LOG_INFO(">> psyc_server:alias_remove(%.*s, %.*s, %d)\n",
              (int)unilen, uni, (int)nicklen, nick, own);

    if (own) {
        g_free(server->nick);
        server->nick = g_strdup(uni);
        signal_emit("server nick changed", 1, server);
    } else {
        /// @todo change nick in all channels
    }
}

static void
alias_change (PSYC_SERVER_REC *server, uint8_t own,
              const char *uni, size_t unilen,
              const char *oldnick, size_t oldnicklen,
              const char *newnick, size_t newnicklen)
{
    LOG_INFO(">> psyc_server:alias_change(%.*s, %.*s, %d)\n",
              (int)oldnicklen, oldnick, (int)newnicklen, newnick, own);

    if (own) {
        g_free(server->nick);
        server->nick = g_strdup(newnick);
        signal_emit("server nick changed", 1, server);
    } else {

    }
}

static void
receive (PSYC_SERVER_REC *server, Packet *p, uint8_t state_reset,
	 PsycUniform *ctx, uint8_t ownctx,
         const char *ctxuni, size_t ctxunilen,
         const char *ctxname, size_t ctxnamelen,
	 PsycUniform *src, uint8_t ownsrc,
         const char *srcuni, size_t srcunilen,
         const char *srcnick, size_t srcnicklen,
         PsycMethod mc, PsycMethod mc_family, unsigned int mc_flag,
         const char *method, size_t methodlen,
         const char *data, size_t datalen)
{
    LOG_INFO(">> psyc_server:receive(method=%.*s, mc=%u, mc_family=%u, mc_flag=%u, "
              "state_reset=%d, ownsrc=%d, ownctx=%d, "
              "ctxuni=%.*s, ctxname=%.*s, uni=%.*s, nick=%.*s, data=%.*s)\n",
              (int)S2ARG2(p->method), mc, mc_family, mc_flag,
              state_reset, ownsrc, ownctx,
              (int)ctxunilen, ctxuni, (int)ctxnamelen, ctxname,
              (int)srcunilen, srcuni, (int)srcnicklen, srcnick,
              (int)datalen, data);

    if (!server || !IS_PSYC_SERVER(server))
        return;

    if (ctxnamelen > 0) {
        PSYC_CHANNEL_REC *channel = psyc_channel_find(server, ctxuni);
        if (!channel)
            channel = psyc_channel_create(server, ctxuni, ctxname, 0);

        psyc_channel_receive(server, channel, p, state_reset,
                             src, ownsrc, srcuni, srcunilen, srcnick, srcnicklen,
			     mc, mc_family, mc_flag,
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
            signal_emit(method, 5, server, data, srcnick, srcuni, ctx);
    }
}

static void
receive_raw (PSYC_SERVER_REC *server)
{
    LOG_INFO(">> psyc_server:receive_raw()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    char buf[RECVBUFLEN];
    int len = net_receive(net_sendbuffer_handle(server->handle), buf, RECVBUFLEN);
    LOG_INFO("len: %d\n", len);

    if (len > 0) {
        psyc_client_receive(server->client, buf, len);
    } else if (len < 0) {
        LOG_INFO("receive error: %d\n", len);
        server->connection_lost = TRUE;
        server_disconnect((SERVER_REC*)server);
    }
}

PsycClientEvents client_events = {
    .receive = (PsycClientReceive) receive,
    .link = (PsycClientLink) linked,
    .unlink = (PsycClientUnlink) unlinked,
    .alias_add = (PsycClientAliasAdd) alias_add,
    .alias_remove = (PsycClientAliasRemove) alias_remove,
    .alias_change = (PsycClientAliasChange) alias_change,
};

static void
sig_server_connected (PSYC_SERVER_REC *server)
{
    LOG_INFO(">> psyc_server:sig_server_connected()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    server->channels_join = psyc_channel_join;
    server->ischannel = is_channel;
    server->isnickflag = (void *) is_nick_flag;
    server->get_nick_flags = (void *) get_nick_flags;
    server->send_message = psyc_channel_send_message;

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
    LOG_INFO(">> psyc_server:sig_server_disconnected()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    psyc_client_destroy(server->client);
}

static void
sig_server_quit (PSYC_SERVER_REC *server, const char *msg)
{
    LOG_INFO(">> psyc_server:sig_server_quit()\n");

    if (!server || !IS_PSYC_SERVER(server))
        return;

    psyc_client_unlink(server->client);
}

static void
sig_server_connect_copy (SERVER_CONNECT_REC **dest, PSYC_SERVER_CONNECT_REC *src)
{
    LOG_INFO(">> psyc_server:sig_server_connect_copy()\n");

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
