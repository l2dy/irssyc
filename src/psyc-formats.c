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
#include <fe-common/core/formats.h>

FORMAT_REC psyc_formats[] = {
    { MODULE_NAME, "PSYC", 0 },

    { NULL, "Message", 0 },

    { "packet",
      "$Z {line_start}\n$0", 1, {0} },

    { "default",
      "$0", 1, {0} },

    { "_notice",
      "$0", 1, {0} },

    { "_request",
      "$0", 1, {0} },

    { "_status",
      "$0", 1, {0} },

    { "_echo_message",
      "{ownmsgnick $2 {ownnick $0}}$1", 3, {0} },

    { "_echo_message__speakaction",
      "{ownmsgnick $2 {ownnick $0} {ownnick $4}}$1", 3, {0} },

    { "_message",
      "{pubmsgnick $2 {pubnick $0}}$1", 3, {0} },

    { "_message__hilight",
      "{pubmsghinick $2 $0}$1", 3, {0} },

    { "_message__speakaction",
      "{pubmsgnick $2 {pubnick $0} {pubnick_action $4}}$1", 5, {0} },

    { "_message__hilight_speakaction",
      "{pubmsghinick $2 $0 {pubnick_action $4}}$1", 3, {0} },

    { "_echo_message_action",
      "{ownaction $0}$1", 2, {0} },

    { "_message_action",
      "{pubaction $0}$1", 2, {0} },

    { "_message_action__hilight",
      "{pubaction $0%n}$1", 2, {0} },

    { "_notice_context_enter",
      "{channick_hilight $0} {chanhost_hilight $1} $2", 3, {0} },

    { "_notice_context_leave", 
      "{channick $0} {chanhost $1} $2", 3, {0} },

    { "_status_context_members",
      "$0%K$1%n$2", 3, {0} },

    { NULL, "Command", 0 },

    { "state_list_header",
      "Context state:", 0 },

    { "state_list_var",
      "$[20]0 $1", 2, {0} },

    { "state_list_footer",
      "Total of $0 variables", 1, {0} },

    { "state_var",
      "$[20]0 $1", 2, {0} },

    { "state_var_not_found",
      "$0 not found in context state", 1, {0} },

};
