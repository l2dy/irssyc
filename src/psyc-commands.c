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

// FRIEND
static void
cmd_friend (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_friend()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    command_runsub("friend", data, server, channel);
}

// FRIEND REQUEST <uniform>
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

// FRIEND APPROVE <uniform>
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
    command_bind_psyc("hello", NULL, (SIGNAL_FUNC) cmd_hello);
    command_bind_psyc("friend", NULL, (SIGNAL_FUNC) cmd_friend);
    command_bind_psyc("friend request", NULL, (SIGNAL_FUNC) cmd_friend_request);
    command_bind_psyc("friend approve", NULL, (SIGNAL_FUNC) cmd_friend_approve);
}

void
psyc_commands_deinit ()
{
    command_unbind("hello", (SIGNAL_FUNC) cmd_hello);
    command_unbind("friend", (SIGNAL_FUNC) cmd_friend);
    command_unbind("friend request", (SIGNAL_FUNC) cmd_friend_request);
    command_unbind("friend approve", (SIGNAL_FUNC) cmd_friend_approve);
}
