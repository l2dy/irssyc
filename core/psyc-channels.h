#ifndef __PSYC_CHANNELS_H
#define __PSYC_CHANNELS_H

#include "chat-protocols.h"
#include "channels.h"

/* Returns PSYC_CHANNEL_REC if it's PSYC channel, NULL if it isn't. */
#define PSYC_CHANNEL(channel) \
	PROTO_CHECK_CAST(CHANNEL(channel), PSYC_CHANNEL_REC, chat_type, "PSYC")

#define IS_PSYC_CHANNEL(channel) \
	(PSYC_CHANNEL(channel) ? TRUE : FALSE)

#define STRUCT_SERVER_REC PSYC_SERVER_REC
struct _PSYC_CHANNEL_REC {
#include "channel-rec.h"
};

/* Create new PSYC channel record */
PSYC_CHANNEL_REC *psyc_channel_create(PSYC_SERVER_REC *server, const char *name, const char *visible_name, int automatic);

#endif
