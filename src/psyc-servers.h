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

#ifndef IRSSI_PSYC_SERVERS_H
#define IRSSI_PSYC_SERVERS_H

#include "module.h"

#include <core/chat-protocols.h>
#include <core/servers.h>
#include <core/servers-setup.h>

#include <psyc/client.h>

/* returns a PSYC_SERVER_REC if it's a PSYC server, or NULL otherwise */
#define PSYC_SERVER(server)                                             \
    PROTO_CHECK_CAST(SERVER(server), PSYC_SERVER_REC, chat_type, "PSYC")
#define PSYC_SERVER_CONNECT(conn)                                       \
    PROTO_CHECK_CAST(SERVER_CONNECT(conn), PSYC_SERVER_CONNECT_REC, chat_type, "PSYC")
#define IS_PSYC_SERVER(server)                  \
    (PSYC_SERVER(server) ? TRUE : FALSE)
#define IS_PSYC_SERVER_CONNECT(conn)            \
    (PSYC_SERVER_CONNECT(conn) ? TRUE : FALSE) 

#define STRUCT_SERVER_CONNECT_REC PSYC_SERVER_CONNECT_REC
struct _PSYC_SERVER_CONNECT_REC {
#include <core/server-connect-rec.h>
};

struct _PSYC_SERVER_REC {
#include <core/server-rec.h>
    PsycClient *client;
};

#define PSYC_SERVER_SETUP(server)                                       \
    PROTO_CHECK_CAST(SERVER_SETUP(server), PSYC_SERVER_SETUP_REC,       \
                     chat_type, "PSYC")

#define IS_PSYC_SERVER_SETUP(server)            \
    (PSYC_SERVER_SETUP(server) ? TRUE : FALSE)

typedef struct {
#include <core/server-setup-rec.h>
} PSYC_SERVER_SETUP_REC;

SERVER_REC *
psyc_server_init_connect (SERVER_CONNECT_REC *conn);

void
psyc_server_connect (SERVER_REC *server);

void
psyc_servers_init ();

void
psyc_servers_deinit ();

#endif
