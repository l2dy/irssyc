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

#ifndef IRSSI_PSYC_MODULE_H
#define IRSSI_PSYC_MODULE_H

#define MODULE_NAME "psyc/core"

#include <irssi/irssi-config.h>
#include <common.h>
#include <stdio.h>
#include <psyc.h>

typedef struct _PSYC_SERVER_CONNECT_REC PSYC_SERVER_CONNECT_REC;
typedef struct _PSYC_SERVER_REC PSYC_SERVER_REC;
typedef struct _PSYC_CHANNEL_REC PSYC_CHANNEL_REC;

#define PSYC_PROTOCOL (chat_protocol_lookup("PSYC"))

#define IRSSI_PSYC_PACKAGE "irssi-psyc"
#define IRSSI_PSYC_VERSION "0.2"

#ifdef DEBUG
# define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
# define LOG_INFO(...) fprintf(stderr, __VA_ARGS__)
# define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#else
# define LOG_DEBUG(...)
# define LOG_INFO(...)
# define LOG_ERRORc(...)
#endif

#define C2STR(str) (PsycString) {sizeof(str)-1, str}
#define C2STRI(str) {sizeof(str)-1, str}
#define C2ARG(str) str, sizeof(str)-1
#define C2ARG2(str) sizeof(str)-1, str
#define S2ARG(str) (str).data, (str).length
#define S2ARG2(str) (str).length, (str).data

#endif
