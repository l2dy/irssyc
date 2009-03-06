/*
 * Copyright (C) 2008 Gabor Adam TOTH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "module.h"
#include "psyc-queries.h"
#include "psyc-servers.h"

QUERY_REC *psyc_query_create(const char *server_tag, const char *nick, int automatic) {
  QUERY_REC *rec;

  g_return_val_if_fail(nick != NULL, NULL);

  rec = g_new0(QUERY_REC, 1);
  rec->chat_type = PSYC_PROTOCOL;
  rec->name = g_strdup(nick);
  rec->server_tag = g_strdup(server_tag);

  query_init(rec, automatic);

  return rec;
}
