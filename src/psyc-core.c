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

#include <core/signals.h>
#include <core/chat-protocols.h>
#include <core/chatnets.h>
#include <core/servers-setup.h>
#include <core/channels-setup.h>
#include <core/settings.h>
#include <core/channels.h>

#include "psyc-servers.h"
#include "psyc-channels.h"
#include "psyc-commands.h"
#include "psyc-formats.h"

static CHATNET_REC *
create_chatnet ()
{
    return g_new0(CHATNET_REC, 1);
}

static SERVER_SETUP_REC *
create_server_setup ()
{
    return g_new0(SERVER_SETUP_REC, 1);
}

static SERVER_CONNECT_REC *
create_server_connect ()
{
    return (SERVER_CONNECT_REC *) g_new0(PSYC_SERVER_CONNECT_REC, 1);
}

static CHANNEL_SETUP_REC *
create_channel_setup ()
{
    return g_new0(CHANNEL_SETUP_REC, 1);
}

static void
destroy_server_connect (SERVER_CONNECT_REC *conn)
{

}

void
psyc_core_init ()
{
    CHAT_PROTOCOL_REC *rec;

    settings_add_bool("psyc", "psyc_debug", FALSE);
    settings_add_str("psyc", "psyc_log_level", "WARNING");

#ifdef DEBUG
    if (settings_get_bool("psyc_debug"))
	GNUNET_log_setup("irssyc", settings_get_str("psyc_log_level"), NULL);
#endif

    rec = g_new0(CHAT_PROTOCOL_REC, 1);
    rec->name = "PSYC";
    rec->fullname = "PSYC, Protocol for Synchronous Conferencing";
    rec->chatnet = "psycnet";

    rec->case_insensitive = TRUE;

    rec->create_chatnet = create_chatnet;
    rec->create_server_setup = create_server_setup;
    rec->create_server_connect = create_server_connect;
    rec->create_channel_setup = create_channel_setup;
    rec->destroy_server_connect = destroy_server_connect;

    rec->server_init_connect = psyc_server_init_connect;
    rec->server_connect = psyc_server_connect;
    rec->channel_create = psyc_channel_create;
    rec->query_create = psyc_command_query_create;

    chat_protocol_register(rec);
    g_free(rec);

    theme_register(psyc_formats);

    psyc_servers_init();
    psyc_channels_init();
    psyc_commands_init();

    module_register("psyc", "core");
}

void
psyc_core_deinit ()
{
    psyc_servers_deinit();
    psyc_channels_deinit();
    psyc_commands_deinit();

    signal_emit("chat protocol deinit", 1, chat_protocol_find("PSYC"));
    chat_protocol_unregister("PSYC");
}
