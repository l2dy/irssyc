/*
  psyc-channels.c : irssi-psyc

  Copyright (C) 2008 Gabor Adam TOTH

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "module.h"
#include "psyc-servers.h"
#include "psyc-channels.h"

PSYC_CHANNEL_REC *psyc_channel_create(PSYC_SERVER_REC *server, const char *name, const char *visible_name, int automatic) {
  /*fprintf(stderr, "##psyc_channel_create");*/
  PSYC_CHANNEL_REC *rec;
  g_return_val_if_fail(server == NULL || IS_PSYC_SERVER(server), NULL);
  g_return_val_if_fail(name != NULL, NULL);

  rec = g_new0(PSYC_CHANNEL_REC, 1);
  if (*name == '+') rec->no_modes = TRUE;

  channel_init((CHANNEL_REC *) rec, (SERVER_REC *) server, name, visible_name, automatic);
  return rec;
}
