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
#include <core/nicklist.h>
#include <core/recode.h>
#include <core/settings.h>
#include <core/signals.h>
#include <fe-common/core/window-items.h>
#include <fe-common/core/hilight-text.h>
#include <fe-common/core/printtext.h>
#include <fe-common/core/fe-messages.h>

#include <psyc/client.h>
#include <psyc/client/commands.h>

#include "psyc-servers.h"
#include "psyc-channels.h"
#include "psyc-formats.h"

PSYC_CHANNEL_REC *
psyc_channel_create (PSYC_SERVER_REC *server, const char *name,
                     const char *visible_name, int automatic)
{
    LOG_DEBUG(">> psyc_channel_create(%s, %s, %d)\n", name, visible_name, automatic);

    PSYC_CHANNEL_REC *rec;
    g_return_val_if_fail(server == NULL || IS_PSYC_SERVER(server), NULL);
    g_return_val_if_fail(name != NULL, NULL);

    rec = g_new0(PSYC_CHANNEL_REC, 1);
    rec->no_modes = TRUE;

    channel_init((CHANNEL_REC *)rec, (SERVER_REC *)server,
                 name, visible_name, automatic);
    return rec;
}

void
psyc_channel_join (PSYC_SERVER_REC *server, const char *uni, int automatic)
{
    LOG_DEBUG(">> psyc_channel_join(%s, %d)\n", uni, automatic);
    psyc_client_enter(server->client, uni, strlen(uni));
}

void
psyc_channel_receive (PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel, Packet *p,
                      PsycMethod mc, PsycMethod mc_family, unsigned int mc_flag,
                      char *uni, size_t unilen, char *nick, size_t nicklen,
                      char *method, size_t methodlen, char *data, size_t datalen)
{
    LOG_DEBUG(">> psyc_channel_receive(%u)\n", mc);

    int for_me;
    char *color, *mode, *msg = data, *msg1 = NULL, *msg2 = NULL;
    HILIGHT_REC *hilight;

    if (settings_get_bool("psyc_debug"))
        printformat_window(window_item_window(channel),
                           MSGLEVEL_CRAP | MSGLEVEL_NOHILIGHT | MSGLEVEL_NEVER,
                           PSYCTXT_PACKET, p->content.data);

    if (mc_flag & PSYC_METHOD_VISIBLE) {
        if (!nick)
            nick = uni;

        for_me = !settings_get_bool("hilight_nick_matches") ? FALSE :
            nick_match_msg(CHANNEL(channel), data, server->nick);
        hilight = for_me ? NULL :
            hilight_match_nick(SERVER(server), channel->name, nick, uni,
                               MSGLEVEL_PUBLIC, msg);
        color = (hilight == NULL) ? NULL : hilight_get_color(hilight);

        msg = msg1 = recode_in(SERVER(server), data, uni);

        if (settings_get_bool("emphasis"))
            msg = msg2 = expand_emphasis((WI_ITEM_REC *)channel, msg);

        mode = settings_get_bool("show_nickmode")
            ? settings_get_bool("show_nickmode_empty") ? " " : "" : "";

        switch (mc) {
        case PSYC_MC_MESSAGE:
            printformat_window(window_item_window(channel),
                               MSGLEVEL_PUBLIC,
                               PSYCTXT__MESSAGE, nick, msg, mode);
            break;
        case PSYC_MC_MESSAGE_ECHO:
            printformat_window(window_item_window(channel),
                               MSGLEVEL_PUBLIC,
                               PSYCTXT__MESSAGE_ECHO, nick, msg, mode);
            break;
        case PSYC_MC_MESSAGE_ACTION:
            printformat_window(window_item_window(channel),
                               MSGLEVEL_PUBLIC | MSGLEVEL_ACTIONS,
                               PSYCTXT__MESSAGE_ACTION, nick, msg, mode);
            break;
        case PSYC_MC_MESSAGE_ECHO_ACTION:
            printformat_window(window_item_window(channel),
                               MSGLEVEL_PUBLIC | MSGLEVEL_ACTIONS,
                               PSYCTXT__MESSAGE_ECHO_ACTION, nick, msg, mode);
            break;
        default:
            break;
        }

	g_free_not_null(msg1);
	g_free_not_null(msg2);
    }

    if (methodlen > 0)
        signal_emit(method, 5, server, msg, nick, uni, channel->name);
}

void
psyc_channels_init ()
{

}

void
psyc_channels_deinit ()
{

}
