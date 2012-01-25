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

#include <fe-common/core/formats.h>

enum {
    PSYCTXT_MODULE_NAME,

    PSYCTXT_CAT_MESSAGE,

    PSYCTXT_PACKET,
    PSYCTXT_DEFAULT,
    PSYCTXT__DATA,
    PSYCTXT__ECHO,
    PSYCTXT__ERROR,
    PSYCTXT__FAILURE,
    PSYCTXT__INFO,
    PSYCTXT__NOTICE,
    PSYCTXT__REQUEST,
    PSYCTXT__STATUS,
    PSYCTXT__WARNING,
    PSYCTXT__MESSAGE_ECHO,
    PSYCTXT__MESSAGE_ECHO__SPEAKACTION,
    PSYCTXT__MESSAGE,
    PSYCTXT__MESSAGE__HILIGHT,
    PSYCTXT__MESSAGE__SPEAKACTION,
    PSYCTXT__MESSAGE__HILIGHT_SPEAKACTION,
    PSYCTXT__MESSAGE_ECHO_ACTION,
    PSYCTXT__MESSAGE_ACTION,
    PSYCTXT__MESSAGE_ACTION__HILIGHT,
    PSYCTXT__NOTICE_CONTEXT_ENTER,
    PSYCTXT__NOTICE_CONTEXT_LEAVE,
    PSYCTXT__STATUS_CONTEXT_MEMBERS,

    PSYCTXT_CAT_COMMAND,

    PSYCTXT_STATE_LIST_HEADER,
    PSYCTXT_STATE_LIST_VAR,
    PSYCTXT_STATE_LIST_FOOTER,

    PSYCTXT_STATE_VAR,
    PSYCTXT_STATE_VAR_NOT_FOUND,
};

extern FORMAT_REC psyc_formats[];
