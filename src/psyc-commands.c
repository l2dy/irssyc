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

#include <core/channels.h>
#include <core/levels.h>
#include <fe-common/core/window-items.h>
#include <fe-common/core/hilight-text.h>
#include <fe-common/core/printtext.h>

#include "psyc-servers.h"
#include "psyc-channels.h"
#include "psyc-commands.h"
#include "psyc-formats.h"

#include <psyc/client.h>
#include <psyc/client/commands.h>

// QUERY
QUERY_REC *
psyc_command_query_create (const char *server_tag, const char *uni, int automatic)
{
    PSYC_SERVER_REC *server = server_find_tag(server_tag);
    if (server && IS_PSYC_SERVER(server))
	psyc_client_query(server->client, uni, strlen(uni));

    return NULL;
}

// PSYC
static void
cmd_psyc (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_INFO(">> cmd_psyc()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    command_runsub("psyc", data, server, channel);
}

// PSYC ALIAS
static void
cmd_alias (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_INFO(">> cmd_alias()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    command_runsub("psyc alias", data, server, channel);
}

// PSYC ALIAS ADD <nick> <uniform>
static void
cmd_alias_add (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *nick, *uni;

    if (!cmd_get_params(data, &free_arg, 2, &nick, &uni))
        return;

    size_t nicklen = strlen(nick);
    size_t unilen = strlen(uni);

    if (!nicklen || !unilen)
        return;

    psyc_client_alias_add(server->client, nick, nicklen, uni, unilen);

    cmd_params_free(free_arg);
}

// PSYC ALIAS REMOVE <nick> | <uniform>
static void
cmd_alias_remove (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *uni;

    if (!cmd_get_params(data, &free_arg, 1, &uni))
        return;

    size_t unilen = strlen(uni);

    if (!unilen)
        return;

    psyc_client_alias_remove(server->client, uni, unilen);

    cmd_params_free(free_arg);
}

// PSYC ALIAS CHANGE <old nick> <new nick>
static void
cmd_alias_change (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *oldnick, *newnick;

    if (!cmd_get_params(data, &free_arg, 2, &oldnick, &newnick))
        return;

    size_t oldnicklen = strlen(oldnick);
    size_t newnicklen = strlen(newnick);

    if (!oldnicklen || !newnicklen)
        return;

    psyc_client_alias_change(server->client, oldnick, oldnicklen, newnick, newnicklen);

    cmd_params_free(free_arg);
}

// NICK <new nick>
static void
cmd_nick (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *nick;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    if (!cmd_get_params(data, &free_arg, 1, &nick))
        return;

    psyc_client_alias_add(server->client, nick, strlen(nick), NULL, 0);

    cmd_params_free(free_arg);
}

// PSYC HELLO [<hello msg>]
static void
cmd_hello (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *hello;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    if (!cmd_get_params(data, &free_arg, 1, &hello))
        return;

    size_t hellolen = strlen(hello);

    if (hellolen)
        psyc_client_hello_offer(server->client, hello, hellolen);
    else
        psyc_client_hello_get(server->client);

    cmd_params_free(free_arg);
}

// PSYC FRIEND
static void
cmd_friend (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_INFO(">> cmd_friend()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    command_runsub("psyc friend", data, server, channel);
}

// PSYC FRIEND REQUEST <uniform>
static void
cmd_friend_request (const char *data, PSYC_SERVER_REC *server,
                    PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *uni;

    if (!cmd_get_params(data, &free_arg, 1, &uni))
        return;

    size_t unilen = strlen(uni);
    if (!unilen)
        return;

    psyc_client_friend_request(server->client, uni, unilen);

    cmd_params_free(free_arg);
}

// PSYC FRIEND APPROVE <uniform>
static void
cmd_friend_approve (const char *data, PSYC_SERVER_REC *server,
                    PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *uni;

    if (!cmd_get_params(data, &free_arg, 1, &uni))
        return;

    size_t unilen = strlen(uni);
    if (!unilen)
        return;

    psyc_client_friend_approve(server->client, uni, unilen);

    cmd_params_free(free_arg);
}

void
psyc_commands_init ()
{
    command_bind_psyc("psyc", NULL, (SIGNAL_FUNC) cmd_psyc);
    command_bind_psyc("psyc alias", NULL, (SIGNAL_FUNC) cmd_alias);
    command_bind_psyc("psyc alias add", NULL, (SIGNAL_FUNC) cmd_alias_add);
    command_bind_psyc("psyc alias remove", NULL, (SIGNAL_FUNC) cmd_alias_remove);
    command_bind_psyc("psyc alias change", NULL, (SIGNAL_FUNC) cmd_alias_change);
    command_bind_psyc("nick", NULL, (SIGNAL_FUNC) cmd_nick);
    command_bind_psyc("psyc hello", NULL, (SIGNAL_FUNC) cmd_hello);
    command_bind_psyc("psyc friend", NULL, (SIGNAL_FUNC) cmd_friend);
    command_bind_psyc("psyc friend request", NULL, (SIGNAL_FUNC) cmd_friend_request);
    command_bind_psyc("psyc friend approve", NULL, (SIGNAL_FUNC) cmd_friend_approve);
}

void
psyc_commands_deinit ()
{
    command_unbind("psyc", (SIGNAL_FUNC) cmd_psyc);
    command_unbind("psyc alias", (SIGNAL_FUNC) cmd_alias);
    command_unbind("psyc alias add", (SIGNAL_FUNC) cmd_alias_add);
    command_unbind("psyc alias remove", (SIGNAL_FUNC) cmd_alias_remove);
    command_unbind("psyc alias change", (SIGNAL_FUNC) cmd_alias_change);
    command_unbind("nick", (SIGNAL_FUNC) cmd_nick);
    command_unbind("psyc hello", (SIGNAL_FUNC) cmd_hello);
    command_unbind("psyc friend", (SIGNAL_FUNC) cmd_friend);
    command_unbind("psyc friend request", (SIGNAL_FUNC) cmd_friend_request);
    command_unbind("psyc friend approve", (SIGNAL_FUNC) cmd_friend_approve);
}
