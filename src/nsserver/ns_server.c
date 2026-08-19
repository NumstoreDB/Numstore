#include "nsserver/ns_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_utils.h"
#include "nsserver/ns_connection.h"
#include "nsserver/os/ns_net_darwin.h"

struct connection_frame
{
  struct connection conn;
  int               present;
};

#define KTYPE  int
#define VTYPE  u32
#define SUFFIX conn
#include "core/ns_robin_hood_ht.h"
#undef KTYPE
#undef VTYPE
#undef SUFFIX

struct ns_server
{
  hash_table_conn         socket_to_index;
  hentry_conn             _hdata[100];
  struct connection_frame conns[100];
  struct kevent           events[200];
  int                     fd;
  int                     kq;
};

struct ns_server *
server_create (error *e)
{
  struct ns_server *server = i_malloc (default_mem (), 1, sizeof *server, e);
  if (server == NULL)
  {
    return NULL;
  }

  // Open the socket
  int fd = net_darwin.funcs->i_socket (&net_darwin, PF_INET, SOCK_STREAM, 0, e);
  if (fd < 0)
  {
    i_free (default_mem (), server);
    return NULL;
  }

  // Listening
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons (9001);
  addr.sin_addr.s_addr = INADDR_ANY;

  // Allow reuse
  int result = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof (int));
  if (result < 0)
  {
    error_causef (e, ERR_IO, "setsockopt failed: %s", strerror (errno));
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  // Bind to socket
  result = bind (fd, (struct sockaddr *)&addr, sizeof (addr));
  if (result < 0)
  {
    error_causef (e, ERR_IO, "bind failed: %s", strerror (errno));
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  result = listen (fd, 1);
  if (result < 0)
  {
    error_causef (e, ERR_IO, "listen failed: %s", strerror (errno));
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  // Set to non blocking mode
  int flags = fcntl (fd, F_GETFL, 0);
  if (flags < 0)
  {
    error_causef (e, ERR_IO, "fcntl (F_GETFL) failed: %s", strerror (errno));
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  result = fcntl (fd, F_SETFL, flags | O_NONBLOCK);
  if (result < 0)
  {
    error_causef (e, ERR_IO, "fcntl (F_SETFL) failed: %s", strerror (errno));
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  // Create a kqueue
  int kq = kqueue ();
  if (kq < 0)
  {
    error_causef (e, ERR_IO, "kqueue failed: %s", strerror (errno));
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  // Append the server to the list
  struct kevent change;
  EV_SET (&change, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  result = kevent (kq, &change, 1, NULL, 0, NULL);
  if (result < 0)
  {
    error_causef (e, ERR_IO, "kevent failed: %s", strerror (errno));
    close (kq);
    close (fd);
    i_free (default_mem (), server);
    return NULL;
  }

  ht_init_conn (&server->socket_to_index, server->_hdata, arrlen (server->_hdata));

  server->fd = fd;
  server->kq = kq;

  return server;
}

err_t
nsserver_execute (struct ns_server *server, error *e)
{
  // Block on a new event
  int n = kevent (server->kq, NULL, 0, server->events, 200, NULL); // block until ready

  if (n < 0)
  {
  }
  err_check (n >= 0, "kevent");

  // Iterate through all events
  for (int i = 0; i < n; ++i)
  {
    // Server
    if (server->events[i].ident == (uintptr_t)server)
    {
      // Accept a connection
      int client = accept (server->fd, NULL, NULL);
      err_check (client >= 0, "accept");

      // Reserve space in the connection pool
      for (u32 k = 0; k < arrlen (server->conns); ++k)
      {
        if (!server->conns[k].present)
        {
          server->conns[k].present = true;

          ht_insert_expect_conn (
              &server->socket_to_index,
              (hdata_conn){
                  .key   = client,
                  .value = k,
              }
          );

          // Start in write mode
          u32 len = htonl (6);
          memcpy (&server->conns[k].conn.buffer, &len, 4);
          memcpy (&server->conns[k].conn.buffer + 4, "OK", 2);
          server->conns[k].conn.rlen = 6;

          // Add to list in write mode
          struct kevent change;
          EV_SET (&change, client, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
          kevent (server->kq, &change, 1, NULL, 0, NULL);

          break;
        }
      }
    }
    else
    {
      hdata_conn data;
      ht_get_expect_conn (&server->socket_to_index, &data, server->events[i].ident);
      struct connection *conn = &server->conns[data.value].conn;

      // Writing
      if (conn->rlen >= 4 && conn->rlen == conn_read_prefix (conn))
      {
        ASSERT (conn->rlen >= conn->wlen);
        if (conn->rlen == conn->wlen)
        {
          // Done writing - transition to reading
          conn->wlen = 0;
          conn->rlen = 0;
          // We'll do a read next

          // Write -> Read
          struct kevent changes[2];
          EV_SET (&changes[0], server->events[i].ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
          EV_SET (
              &changes[1],
              server->events[i].ident,
              EVFILT_READ,
              EV_ADD | EV_ENABLE,
              0,
              0,
              NULL
          );
          kevent (server->kq, changes, 2, NULL, 0, NULL);
        }
        else
        {
          ssize_t sent =
              send (server->events[i].ident, conn->buffer + conn->wlen, conn->rlen - conn->wlen, 0);
          printf ("Sent: %ld\n", sent);
          err_check (sent >= 0, "send");
          conn->wlen += sent;
        }
      }

      // Reading
      else if (conn->rlen < 4 || conn->rlen < conn_read_prefix (conn))
      {
        ASSERT (conn->wlen == 0);

        u32     len = conn_read_prefix (conn);
        ssize_t recvd =
            recv (server->events[i].ident, conn->buffer + conn->rlen, len - conn->rlen, 0);
        printf ("Recv: %ld\n", recvd);
        err_check (recvd >= 0, "recv");
        conn->rlen += recvd;
        ASSERT (conn->rlen <= len);

        // Done reading
        if (conn->rlen == len)
        {
          len = htonl (6);
          memcpy (&conn->buffer, &len, 4);
          memcpy (&conn->buffer + 4, "OK", 2);
          conn->rlen = 6;

          // Read -> Write
          struct kevent changes[2];
          EV_SET (&changes[0], server->events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
          EV_SET (
              &changes[1],
              server->events[i].ident,
              EVFILT_WRITE,
              EV_ADD | EV_ENABLE,
              0,
              0,
              NULL
          );
          kevent (server->kq, changes, 2, NULL, 0, NULL);
        }
      }
    }
  }
}
