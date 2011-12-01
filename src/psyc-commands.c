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

// NICK <new nick>
static void
cmd_nick (const char *data, PSYC_SERVER_REC *server, WI_ITEM_REC *witem)
{
    void *free_arg;
    char *nick;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    if (!cmd_get_params(data, &free_arg, 1, &nick))
        return;

    //psyc_client_set_nick(server->client, nick, strlen(nick));

    cmd_params_free(free_arg);
}

static void
state_list_var (PSYC_CHANNEL_REC *channel, Modifier *mod, PsycOperator oper,
                char *name, size_t namelen, char *value, size_t valuelen)
{
    LOG_DEBUG(">> state_list_var(%.*s, %.*s)\n",
              (int)namelen, name, (int)valuelen, value);

    printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                       PSYCTXT_STATE_LIST_VAR, name, value);
}

// STATE LIST
static void
cmd_state_list (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_list()\n");

    printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                       PSYCTXT_STATE_LIST_HEADER);

    ssize_t c = psyc_client_state_iterate(server->client,
                                         channel->name, strlen(channel->name),
                                         (PsycClientStateIterator)state_list_var,
                                         channel);
    char count[11];
    sprintf(count, "%ld", c);

    printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                       PSYCTXT_STATE_LIST_FOOTER, count);
}

// STATE
static void
cmd_state (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    CMD_PSYC_CHANNEL(channel);

    if (*data == '\0')
        cmd_state_list(data, server, channel);
    else
        command_runsub("state", data, server, channel);
}

// STATE GET <name>
static void
cmd_state_get (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_get(%s)\n", data);

    void *free_arg;
    char *name;

    if (!cmd_get_params(data, &free_arg, 1, &name))
        return;

    char *value = psyc_client_state_get(server->client,
                                        channel->name, strlen(channel->name),
                                        name, strlen(name), NULL);

    if (value)
        printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                           PSYCTXT_STATE_VAR, name, value);
    else
        printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                           PSYCTXT_STATE_VAR_NOT_FOUND);

    cmd_params_free(free_arg);
}

// STATE SET <name> <value>
static void
cmd_state_set (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_set(%s)\n", data);

    void *free_arg;
    char *name, *value;

    if (!cmd_get_params(data, &free_arg, 2, &name, &value))
        return;

    size_t namelen = strlen(name);
    if (namelen == 0)
        return;

    PsycOperator oper = PSYC_OPERATOR_ASSIGN;
    if (psyc_is_oper(*name)) {
        oper = *name;
        name++;
        namelen--;
    }
    if (namelen == 0)
        return; 

    psyc_client_state_set(server->client,
                          channel->name, strlen(channel->name),
                          oper, name, namelen,
                          value, strlen(value));

    cmd_params_free(free_arg);
}

// STATE RESET
static void
cmd_state_reset (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_reset(%s)\n", data);

    printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                       PSYCTXT_STATE_LIST_HEADER);

    psyc_client_state_reset(server->client, channel->name, strlen(channel->name));
}

// STATE RESYNC
static void
cmd_state_resync (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_resync(%s)\n", data);

    printformat_window(window_item_window(channel), MSGLEVEL_CLIENTCRAP,
                       PSYCTXT_STATE_LIST_HEADER);

    psyc_client_state_resync(server->client, channel->name, strlen(channel->name));
}

void
psyc_commands_init ()
{
    command_bind_psyc("nick", NULL, (SIGNAL_FUNC)cmd_nick);
    command_bind_psyc("state", NULL, (SIGNAL_FUNC)cmd_state);
    command_bind_psyc("state list", NULL, (SIGNAL_FUNC)cmd_state_list);
    command_bind_psyc("state get", NULL, (SIGNAL_FUNC)cmd_state_get);
    command_bind_psyc("state set", NULL, (SIGNAL_FUNC)cmd_state_set);
    command_bind_psyc("state reset", NULL, (SIGNAL_FUNC)cmd_state_reset);
    command_bind_psyc("state resync", NULL, (SIGNAL_FUNC)cmd_state_resync);
}

void
psyc_commands_deinit ()
{
    command_unbind("nick", (SIGNAL_FUNC)cmd_nick);
    command_unbind("state", (SIGNAL_FUNC)cmd_state);
    command_unbind("state list", (SIGNAL_FUNC)cmd_state_list);
    command_unbind("state get", (SIGNAL_FUNC)cmd_state_get);
    command_unbind("state set", (SIGNAL_FUNC)cmd_state_set);
    command_unbind("state reset", (SIGNAL_FUNC)cmd_state_reset);
    command_unbind("state resync", (SIGNAL_FUNC)cmd_state_resync);
}
