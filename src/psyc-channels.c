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
#include "psyc-commands.h"
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
    rec->joined = TRUE;
    rec->synced = TRUE;

    channel_init((CHANNEL_REC *)rec, (SERVER_REC *)server,
                 name, visible_name, automatic);

    signal_emit("channel joined", 1, rec);
    signal_emit("channel sync", 1, rec);

    return rec;
}

void
psyc_channel_receive (PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel, Packet *p,
                      uint8_t state_reset, uint8_t ownsrc, uint8_t ownctx,
                      PsycMethod mc, PsycMethod mc_family, unsigned int mc_flag,
                      const char *srcuni, size_t srcunilen,
                      const char *srcnick, size_t srcnicklen,
                      const char *method, size_t methodlen,
                      const char *data, size_t datalen)
{
    LOG_DEBUG(">> psyc_channel_receive(%u)\n", mc);

    int for_me = 0;
    char *color, *mode, *msg = data, *msg1 = NULL, *msg2 = NULL, *value;
    HILIGHT_REC *hilight = NULL;
    NICK_REC *nrec = NULL;
    WINDOW_REC *win = window_item_window(channel);

    if (settings_get_bool("psyc_debug"))
        printformat_window(win,
                           MSGLEVEL_CRAP | MSGLEVEL_NOHILIGHT | MSGLEVEL_NEVER,
                           PSYCTXT_PACKET, p->content.data);

    switch (mc) {
    case PSYC_MC_NOTICE_CONTEXT_ENTER:
        nrec = nicklist_find_mask(CHANNEL(channel), srcuni);
        if (nrec || !srcunilen)
            break;
        nrec = g_new0(NICK_REC, 1);
        nrec->nick = g_strdup(srcnick ? srcnick : srcuni);
        nrec->host = g_strdup(srcuni);
        nicklist_insert(CHANNEL(channel), nrec);
        break;
    case PSYC_MC_ECHO_CONTEXT_ENTER:
        break;
    case PSYC_MC_NOTICE_CONTEXT_LEAVE:
	if (srcunilen) {
	    nrec = nicklist_find_mask(CHANNEL(channel), srcuni);
	    if (nrec)
		nicklist_remove(CHANNEL(channel), nrec);
	    break;
	}
	// fall thru
    case PSYC_MC_ECHO_CONTEXT_LEAVE:
        signal_emit(method, 5, server, msg, srcnick, srcuni, channel->name);
        channel_destroy(CHANNEL(channel));
        methodlen = 0;
        break;
    default:
        break;
    }

    if (mc == PSYC_MC_NOTICE_SET || state_reset) {
        value = psyc_client_state_get(server->client,
                                      channel->name, strlen(channel->name),
                                      PSYC_C2ARG("_description"), NULL);
        if (value) {
            if (channel->topic)
                g_free(channel->topic);
            channel->topic = g_strdup(value);
            //channel->topic_by = ;
            //channel->topic_time = ;
            signal_emit("channel topic changed", 1, channel);
        }
    }

    // print packet if needed
    if (mc_flag & PSYC_METHOD_VISIBLE) {
        if (!srcnick)
            srcnick = srcuni;

        if (data) {
            for_me = !settings_get_bool("hilight_nick_matches") ? FALSE :
                nick_match_msg(CHANNEL(channel), msg, server->nick);
            hilight = for_me ? NULL :
                hilight_match_nick(SERVER(server), channel->name, srcnick, srcuni,
                               MSGLEVEL_PUBLIC, msg);
            color = (hilight == NULL) ? NULL : hilight_get_color(hilight);

            msg = msg1 = recode_in(SERVER(server), data, srcuni);

            if (settings_get_bool("emphasis"))
                msg = msg2 = expand_emphasis((WI_ITEM_REC *)channel, msg);
        }

        mode = settings_get_bool("show_nickmode")
            ? settings_get_bool("show_nickmode_empty") ? " " : "" : "";

        switch (mc_family) {
        case PSYC_MC_DATA:
            printformat_window(win, MSGLEVEL_CRAP, PSYCTXT__DATA, msg);
            break;
        case PSYC_MC_ECHO:
            printformat_window(win, MSGLEVEL_CRAP, PSYCTXT__ECHO, msg);
            break;
        case PSYC_MC_ERROR:
            printformat_window(win, MSGLEVEL_SNOTES, PSYCTXT__ERROR, msg);
            break;
        case PSYC_MC_FAILURE:
            printformat_window(win, MSGLEVEL_SNOTES, PSYCTXT__FAILURE, msg);
            break;
        case PSYC_MC_INFO:
            printformat_window(win, MSGLEVEL_CRAP, PSYCTXT__INFO, msg);
            break;
        case PSYC_MC_MESSAGE:
            switch (mc) {
            case PSYC_MC_MESSAGE_ACTION:
                if (ownsrc)
                    printformat_window(win,
                                       MSGLEVEL_PUBLIC | MSGLEVEL_ACTIONS |
                                       MSGLEVEL_NOHILIGHT | MSGLEVEL_NO_ACT,
                                       PSYCTXT__MESSAGE_ECHO_ACTION,
                                       server->nick, msg, mode);
                else
                    printformat_window(win,
                                       MSGLEVEL_PUBLIC | MSGLEVEL_ACTIONS,
                                       PSYCTXT__MESSAGE_ACTION,
                                       srcnick, msg, mode);
                break;
            default:
                if (ownsrc)
                    printformat_window(win, MSGLEVEL_PUBLIC |
                                       MSGLEVEL_NOHILIGHT | MSGLEVEL_NO_ACT,
                                       PSYCTXT__MESSAGE_ECHO, server->nick, msg, mode);
                else
                    printformat_window(win, MSGLEVEL_PUBLIC,
                                       PSYCTXT__MESSAGE, srcnick, msg, mode);
                break;
            }
            break;
        case PSYC_MC_NOTICE:
            printformat_window(win, MSGLEVEL_NOTICES, PSYCTXT__NOTICE, msg);
            break;
        case PSYC_MC_REQUEST:
            printformat_window(win, MSGLEVEL_INVITES | MSGLEVEL_HILIGHT,
                               PSYCTXT__WARNING, msg);
            break;
        case PSYC_MC_STATUS:
            printformat_window(win, MSGLEVEL_CRAP, PSYCTXT__STATUS, msg);
            break;
        case PSYC_MC_WARNING:
            printformat_window(win, MSGLEVEL_CRAP, PSYCTXT__WARNING, msg);
            break;
        default:
            printformat_window(win, MSGLEVEL_CRAP, PSYCTXT_DEFAULT, msg);
            break;
        }

	g_free_not_null(msg1);
	g_free_not_null(msg2);
    }

    if (methodlen > 0)
        signal_emit(method, 5, server, msg, srcnick, srcuni, channel->name);
}

// JOIN <channel>
void
psyc_channel_join (PSYC_SERVER_REC *server, const char *uni, int automatic)
{
    LOG_DEBUG(">> psyc_channel_join(%s, %d)\n", uni, automatic);

    psyc_client_enter(server->client, uni, strlen(uni));
}

// PART
static void
cmd_part (const char *uni, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    CMD_PSYC_SERVER(server);
    CMD_PSYC_CHANNEL(channel);

    psyc_client_leave(server->client, channel->name, strlen(channel->name));
}

// SHARE <filename>
static void
cmd_share (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *fn;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    CMD_PSYC_CHANNEL(channel);

    if (!cmd_get_params(data, &free_arg, 1, &fn))
        return;

    psyc_client_share(server->client, channel->name, strlen(channel->name),
                      fn, strlen(fn));
}

// MEMBER
static void
cmd_member (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_member()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);

    command_runsub("member", data, server, channel);
}

// MEMBER ADD <uniform|nick>
static void
cmd_member_add (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *uni;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    CMD_PSYC_CHANNEL(channel);

    if (!cmd_get_params(data, &free_arg, 1, &uni))
        return;

    psyc_client_member_add(server->client, channel->name, strlen(channel->name),
			   uni, strlen(uni));
}

// MEMBER REMOVE <uniform|nick>
static void
cmd_member_remove (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *uni;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    CMD_PSYC_CHANNEL(channel);

    if (!cmd_get_params(data, &free_arg, 1, &uni))
        return;

    psyc_client_member_remove(server->client, channel->name, strlen(channel->name),
			      uni, strlen(uni));
}

void
psyc_channel_send_message (PSYC_SERVER_REC *server, const char *target,
                           const char *msg, int target_type)
{
    LOG_DEBUG(">> psyc_channel_send_message(%s, %s, %d)\n", target, msg, target_type);

    psyc_client_message(server->client, target, strlen(target), msg, strlen(msg));
}

// ME <action>
static void
cmd_me (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    void *free_arg;
    char *msg;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    CMD_PSYC_CHANNEL(channel);

    if (!cmd_get_params(data, &free_arg, 1, &msg))
        return;

    psyc_client_message_action(server->client, channel->name, strlen(channel->name),
                               msg, strlen(msg));
}

// TOPIC <new topic>
static void
cmd_topic (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    GHashTable *optlist;
    void *free_arg = NULL;
    char *topic = data;

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    if (channel != NULL && !IS_PSYC_CHANNEL(channel))
        return;

    if (!cmd_get_params(data, &free_arg, 1 | PARAM_FLAG_OPTIONS | PARAM_FLAG_GETREST,
			"topic", &optlist, &topic))
        return;

    if (*topic == 0)
        return;

    if (g_hash_table_lookup(optlist, "delete") != NULL)
        topic = "";

    char *ctx = NULL;
    size_t ctxlen = 0;
    if (channel) {
        ctx = channel->name;
        ctxlen = strlen(channel->name);
    }

    psyc_client_state_set(server->client, ctx, ctxlen,
                          '=', PSYC_C2ARG("_description"), topic, strlen(topic));

    cmd_params_free(free_arg);
}

struct srvchan {
    PSYC_SERVER_REC *server;
    PSYC_CHANNEL_REC *channel;
};

static PsycRC
state_list_var (struct srvchan *sc, Modifier *mod, PsycOperator oper,
                char *name, size_t namelen, char *value, size_t valuelen)
{
    LOG_DEBUG(">> state_list_var(%.*s, %.*s)\n",
              (int)namelen, name, (int)valuelen, value);

    printformat_channel(sc->server, sc->channel, MSGLEVEL_CLIENTCRAP,
                        PSYCTXT_STATE_LIST_VAR, name, value);
    return PSYC_OK;
}

// STATE LIST
static void
cmd_state_list (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_list()\n");

    printformat_channel(server, channel, MSGLEVEL_CLIENTCRAP,
                        PSYCTXT_STATE_LIST_HEADER);

    char *ctx = NULL;
    size_t ctxlen = 0;
    if (channel) {
        ctx = channel->name;
        ctxlen = strlen(channel->name);
    }

    struct srvchan sc = {server, channel};
    ssize_t c = psyc_client_state_iterate(server->client, ctx, ctxlen,
                                          (PsycClientStateIterator)state_list_var, &sc);
    char count[11];
    sprintf(count, "%ld", c);

    printformat_channel(server, channel, MSGLEVEL_CLIENTCRAP,
                       PSYCTXT_STATE_LIST_FOOTER, count);
}

// STATE
static void
cmd_state (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state()\n");

    g_return_if_fail(data != NULL);
    CMD_PSYC_SERVER(server);
    if (channel != NULL && !IS_PSYC_CHANNEL(channel))
        return;

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

    size_t namelen = strlen(name);
    if (!namelen)
        return;

    char *ctx = NULL;
    size_t ctxlen = 0;
    if (channel) {
        ctx = channel->name;
        ctxlen = strlen(channel->name);
    }
    char *value = psyc_client_state_get(server->client, ctx, ctxlen,
                                        name, strlen(name), NULL);

    if (value)
        printformat_channel(server, channel, MSGLEVEL_CLIENTCRAP,
                         PSYCTXT_STATE_VAR, name, value);
    else
        printformat_channel(server, channel, MSGLEVEL_CLIENTCRAP,
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

    if (!cmd_get_params(data, &free_arg, 2 | PARAM_FLAG_GETREST, &name, &value))
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

    char *ctx = NULL;
    size_t ctxlen = 0;
    if (channel) {
        ctx = channel->name;
        ctxlen = strlen(channel->name);
    }

    psyc_client_state_set(server->client, ctx, ctxlen,
                          oper, name, namelen,
                          value, strlen(value));

    cmd_params_free(free_arg);
}

// STATE RESET
static void
cmd_state_reset (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_reset(%s)\n", data);

    char *ctx = NULL;
    size_t ctxlen = 0;
    if (channel) {
        ctx = channel->name;
        ctxlen = strlen(channel->name);
    }

    printformat_channel(server, channel, MSGLEVEL_CLIENTCRAP,
                        PSYCTXT_STATE_LIST_HEADER);
    psyc_client_state_reset(server->client, ctx, ctxlen);
}

// STATE RESYNC
static void
cmd_state_resync (const char *data, PSYC_SERVER_REC *server, PSYC_CHANNEL_REC *channel)
{
    LOG_DEBUG(">> cmd_state_resync(%s)\n", data);

    char *ctx = NULL;
    size_t ctxlen = 0;
    if (channel) {
        ctx = channel->name;
        ctxlen = strlen(channel->name);
    }

    printformat_channel(server, channel, MSGLEVEL_CLIENTCRAP,
                        PSYCTXT_STATE_LIST_HEADER);
    psyc_client_state_resync(server->client, ctx, ctxlen);
}

static void
sig_message_own_public (PSYC_SERVER_REC *server, const char *msg, const char *target)
{
    // don't print own messages, wait for echo
    signal_stop();
}

void
psyc_channels_init ()
{
    command_bind_psyc("me", NULL, (SIGNAL_FUNC) cmd_me);
    command_bind_psyc("topic", NULL, (SIGNAL_FUNC) cmd_topic);
    command_bind_psyc("part", NULL, (SIGNAL_FUNC) cmd_part);
    command_bind_psyc("member", NULL, (SIGNAL_FUNC) cmd_member);
    command_bind_psyc("member add", NULL, (SIGNAL_FUNC) cmd_member_add);
    command_bind_psyc("member remove", NULL, (SIGNAL_FUNC) cmd_member_remove);
    command_bind_psyc("share", NULL, (SIGNAL_FUNC) cmd_share);
    command_bind_psyc("state", NULL, (SIGNAL_FUNC) cmd_state);
    command_bind_psyc("state list", NULL, (SIGNAL_FUNC) cmd_state_list);
    command_bind_psyc("state get", NULL, (SIGNAL_FUNC) cmd_state_get);
    command_bind_psyc("state set", NULL, (SIGNAL_FUNC) cmd_state_set);
    command_bind_psyc("state reset", NULL, (SIGNAL_FUNC) cmd_state_reset);
    command_bind_psyc("state resync", NULL, (SIGNAL_FUNC) cmd_state_resync);

    signal_add_last("message own_public", (SIGNAL_FUNC) sig_message_own_public);
}

void
psyc_channels_deinit ()
{
    command_unbind("me", (SIGNAL_FUNC) cmd_me);
    command_unbind("topic", (SIGNAL_FUNC) cmd_topic);
    command_unbind("part", (SIGNAL_FUNC) cmd_part);
    command_unbind("member", (SIGNAL_FUNC) cmd_member);
    command_unbind("member add", (SIGNAL_FUNC) cmd_member_add);
    command_unbind("member remove", (SIGNAL_FUNC) cmd_member_remove);
    command_unbind("share", (SIGNAL_FUNC) cmd_share);
    command_unbind("state", (SIGNAL_FUNC) cmd_state);
    command_unbind("state list", (SIGNAL_FUNC) cmd_state_list);
    command_unbind("state get", (SIGNAL_FUNC) cmd_state_get);
    command_unbind("state set", (SIGNAL_FUNC) cmd_state_set);
    command_unbind("state reset", (SIGNAL_FUNC) cmd_state_reset);
    command_unbind("state resync", (SIGNAL_FUNC) cmd_state_resync);
}
