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

#ifndef PSYC_COMMANDS_H
#define PSYC_COMMANDS_H

#include <core/commands.h>

#define command_bind_psyc(cmd, section, signal) \
    command_bind_proto(cmd, PSYC_PROTOCOL, section, signal)
#define command_bind_psyc_first(cmd, section, signal) \
    command_bind_proto_first(cmd, PSYC_PROTOCOL, section, signal)
#define command_bind_psyc_last(cmd, section, signal) \
    command_bind_proto_last(cmd, PSYC_PROTOCOL, section, signal)

/* Simply returns if server isn't for PSYC protocol. Prints ERR_NOT_CONNECTED
   error if there's no server or server isn't connected yet */
#define CMD_PSYC_SERVER(server)                 \
    G_STMT_START {                                       \
        if (server != NULL && !IS_PSYC_SERVER(server))   \
            return;                                      \
        if (server == NULL || !(server)->connected)      \
            cmd_return_error(CMDERR_NOT_CONNECTED);      \
    } G_STMT_END

#define CMD_PSYC_CHANNEL(channel)                               \
    G_STMT_START {                                              \
        if (channel == NULL || !IS_PSYC_CHANNEL(channel))       \
            return;                                             \
    } G_STMT_END

void
psyc_commands_init ();

void
psyc_commands_deinit ();

QUERY_REC *
psyc_command_query_create (const char *server_tag, const char *uni, int automatic);

void
send_message (PSYC_SERVER_REC *server, const char *target,
              const char *msg, int target_type);

#endif
