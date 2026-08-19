#ifndef NS_SERVER_H
#define NS_SERVER_H

#include "core/ns_error.h"

struct ns_server *server_create (error *e);
err_t             nsserver_close (struct ns_server *server, error *e);

err_t nsserver_execute (struct ns_server *server, error *e);

#endif
