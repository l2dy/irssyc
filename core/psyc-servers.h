#ifndef __PSYC_SERVERS_H
#define __PSYC_SERVERS_H

#include "chat-protocols.h"
#include "servers.h"
#include "servers-setup.h"

/* returns PSYC_SERVER_REC if it's PSYC server, NULL if it isn't */
#define PSYC_SERVER(server) PROTO_CHECK_CAST(SERVER(server), PSYC_SERVER_REC, chat_type, "PSYC")
#define PSYC_SERVER_CONNECT(conn) PROTO_CHECK_CAST(SERVER_CONNECT(conn), PSYC_SERVER_CONNECT_REC, chat_type, "PSYC")
#define IS_PSYC_SERVER(server) (PSYC_SERVER(server) ? TRUE : FALSE)
#define IS_PSYC_SERVER_CONNECT(conn) (PSYC_SERVER_CONNECT(conn) ? TRUE : FALSE) 

struct _PSYC_SERVER_CONNECT_REC {
#include "server-connect-rec.h"
};
#define STRUCT_SERVER_CONNECT_REC PSYC_SERVER_CONNECT_REC

struct _PSYC_SERVER_REC {
#include "server-rec.h"
#if 0
  LmConnection   *lmconn;
  gchar          *ressource;
  gint           priority;
  gint           show;
  GSList         *roster;
#endif
};

#define PSYC_SERVER_SETUP(server) \
	PROTO_CHECK_CAST(SERVER_SETUP(server), PSYC_SERVER_SETUP_REC, \
			 chat_type, "PSYC")

#define IS_PSYC_SERVER_SETUP(server) \
	(PSYC_SERVER_SETUP(server) ? TRUE : FALSE)

typedef struct {
#include "server-setup-rec.h"
} PSYC_SERVER_SETUP_REC;


SERVER_REC *psyc_server_init_connect(SERVER_CONNECT_REC *conn);
void        psyc_server_connect(SERVER_REC *server);

void        psyc_servers_init(void);
void        psyc_servers_deinit(void);

#endif
