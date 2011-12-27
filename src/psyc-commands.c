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
cmd_hello (const char *msg, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    g_return_if_fail(msg != NULL);
    CMD_PSYC_SERVER(server);

    if (*msg == 0)
        psyc_client_hello_get(server->client);
    else
        psyc_client_hello_offer(server->client, msg, strlen(msg));
}

void
psyc_commands_init ()
{
    command_bind_psyc("hello", NULL, (SIGNAL_FUNC) cmd_hello);
}

void
psyc_commands_deinit ()
{
    command_unbind("hello", (SIGNAL_FUNC) cmd_hello);
}
