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

#ifndef IRSSI_PSYC_CHANNELS_H
#define IRSSI_PSYC_CHANNELS_H

#include "module.h"
#include <core/channels.h>
#include <core/chat-protocols.h>
#include <core/channels.h>
#include <fe-common/core/printtext.h>
#include <fe-common/core/window-items.h>

/* Returns PSYC_CHANNEL_REC if it's PSYC channel, NULL if it isn't. */
#define PSYC_CHANNEL(channel) \
	PROTO_CHECK_CAST(CHANNEL(channel), PSYC_CHANNEL_REC, chat_type, "PSYC")

#define IS_PSYC_CHANNEL(channel) \
	(PSYC_CHANNEL(channel) ? TRUE : FALSE)

#undef STRUCT_SERVER_REC
#define STRUCT_SERVER_REC PSYC_SERVER_REC
struct _PSYC_CHANNEL_REC {
#include <core/channel-rec.h>
};

#define psyc_channel_find(server, name) \
	PSYC_CHANNEL(channel_find(SERVER(server), name))

#define printformat_channel(server, channel, level, formatnum...)       \
    G_STMT_START {                                                      \
        if (channel)                                                     \
            printformat_window(window_item_window(channel), level,     \
                               ##formatnum);                            \
        else                                                            \
            printformat(server, NULL, level, ##formatnum);              \
    } G_STMT_END

/* Create new PSYC channel record */
PSYC_CHANNEL_REC *
psyc_channel_create (PSYC_SERVER_REC *server, const char *name,
                     const char *visible_name, int automatic);

void
psyc_channel_receive (PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel,
		      Packet *p, uint8_t state_reset,
		      PsycUniform *src, uint8_t ownsrc,
		      const char *srcuni, size_t srcunilen,
		      const char *srcnick, size_t srcnicklen,
		      PsycMethod mc, PsycMethod mc_family, unsigned int mc_flag,
		      const char *method, size_t methodlen,
		      const char *data, size_t datalen);

void
psyc_channel_send_message (PSYC_SERVER_REC *server, const char *target,
                           const char *msg, int target_type);

void
psyc_channel_nick_change (PSYC_SERVER_REC *server, char *ctx, size_t ctxlen,
                          char *uni, size_t unilen, char *nick, size_t nicklen);

void
psyc_channel_descr_change (PSYC_SERVER_REC *server, char *ctx, size_t ctxlen,
                           char *topic, size_t topiclen);

void
psyc_channel_join (PSYC_SERVER_REC *server, const char *uni, int automatic);

void
psyc_channels_init ();

void
psyc_channels_deinit ();

#endif
