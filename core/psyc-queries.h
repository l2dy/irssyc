#ifndef __PSYC_QUERIES_H
#define __PSYC_QUERIES_H

#include "queries.h"
#include "psyc-servers.h"

/* Returns PSYC_QUERY_REC if it's PSYC query, NULL if it isn't. */
#define PSYC_QUERY(query)                                       \
  PROTO_CHECK_CAST(QUERY(query), QUERY_REC, chat_type, "PSYC")

#define IS_PSYC_QUERY(query) (PSYC_QUERY(query) ? TRUE : FALSE)

QUERY_REC *psyc_query_create(const char *server_tag, const char *nick, int automatic);

#define psyc_query_find(server, name) query_find(SERVER(server), name)

void psyc_queries_init(void);
void psyc_queries_deinit(void);

#endif
