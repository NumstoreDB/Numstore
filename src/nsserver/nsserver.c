#include "core/ns_csx_assert.h"
#include "core/ns_stdtypes.h"
#include "core/ns_utils.h"

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

#define MAX_EVENTS 64
#define BUF_SIZE   4096

#define err_check(condition, op) \
  if (!(condition)) {            \
    perror (op);                 \
    exit (-1);                   \
  }

struct connection
{
  u8  buffer[4096];

  u32 rlen;
  u32 wlen;
};

static inline u32
conn_read_prefix (struct connection *conn)
{
  u32 len;
  memcpy (&len, conn->buffer, 4);
  len = ntohl (len);
  return len;
}

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

struct conn_mgr
{
  hash_table_conn         socket_to_index;
  hentry_conn             _hdata[100];
  struct connection_frame conns[100];
};

int
main ()
{
  // Open the socket
  int server = socket (PF_INET, SOCK_STREAM, 0);
  err_check (server >= 0, "socket");

  // Listening
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons (9001);
  addr.sin_addr.s_addr = INADDR_ANY;

  // Allow reuse
  int result           = setsockopt (server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof (int));
  err_check (result >= 0, "setsockopt");

  // Bind to socket
  result = bind (server, (struct sockaddr *)&addr, sizeof (addr));
  err_check (result >= 0, "bind");

  result = listen (server, 1);
  err_check (result >= 0, "listen");

  // Set to non blocking mode
  int flags = fcntl (server, F_GETFL, 0);
  err_check (flags >= 0, "fcntl");
  result = fcntl (server, F_SETFL, flags | O_NONBLOCK);
  err_check (result >= 0, "fcntl");

  // Create a kqueue
  int kq = kqueue ();
  err_check (kq >= 0, "kqueue");

  // Append the server to the list
  struct kevent change;
  EV_SET (&change, server, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  kevent (kq, &change, 1, NULL, 0, NULL);

  struct kevent   events[200];
  struct conn_mgr mgr = {0};
  ht_init_conn (&mgr.socket_to_index, mgr._hdata, arrlen (mgr._hdata));

  while (1) {
    // Block on a new event
    int n = kevent (kq, NULL, 0, events, 200, NULL); // block until ready
    err_check (n >= 0, "kevent");

    // Iterate through all events
    for (int i = 0; i < n; ++i) {
      // Server
      if (events[i].ident == (uintptr_t)server) {
        // Accept a connection
        int client = accept (server, NULL, NULL);
        err_check (client >= 0, "accept");

        // Reserve space in the connection pool
        for (u32 k = 0; k < sizeof (mgr.conns); ++k) {
          if (!mgr.conns[k].present) {
            mgr.conns[k].present = true;

            ht_insert_expect_conn (
                &mgr.socket_to_index,
                (hdata_conn){
                    .key   = client,
                    .value = k,
                }
            );

            // Start in write mode
            u32 len = htonl (6);
            memcpy (&mgr.conns[k].conn.buffer, &len, 4);
            memcpy (&mgr.conns[k].conn.buffer + 4, "OK", 2);
            mgr.conns[k].conn.rlen = 6;

            // Add to list in write mode
            struct kevent change;
            EV_SET (&change, client, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
            kevent (kq, &change, 1, NULL, 0, NULL);

            break;
          }
        }
      } else {
        hdata_conn data;
        ht_get_expect_conn (&mgr.socket_to_index, &data, events[i].ident);
        struct connection *conn = &mgr.conns[data.value].conn;

        // Writing
        if (conn->rlen >= 4 && conn->rlen == conn_read_prefix (conn)) {
          ASSERT (conn->rlen >= conn->wlen);
          if (conn->rlen == conn->wlen) {
            // Done writing - transition to reading
            conn->wlen = 0;
            conn->rlen = 0;
            // We'll do a read next

            // Write -> Read
            struct kevent changes[2];
            EV_SET (&changes[0], events[i].ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
            EV_SET (&changes[1], events[i].ident, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
            kevent (kq, changes, 2, NULL, 0, NULL);
          } else {
            ssize_t sent =
                send (events[i].ident, conn->buffer + conn->wlen, conn->rlen - conn->wlen, 0);
            printf ("Sent: %ld\n", sent);
            err_check (sent >= 0, "send");
            conn->wlen += sent;
          }
        }

        // Reading
        else if (conn->rlen < 4 || conn->rlen < conn_read_prefix (conn)) {
          ASSERT (conn->wlen == 0);

          u32     len   = conn_read_prefix (conn);
          ssize_t recvd = recv (events[i].ident, conn->buffer + conn->rlen, len - conn->rlen, 0);
          printf ("Recv: %ld\n", recvd);
          err_check (recvd >= 0, "recv");
          conn->rlen += recvd;
          ASSERT (conn->rlen <= len);

          // Done reading
          if (conn->rlen == len) {
            len = htonl (6);
            memcpy (&conn->buffer, &len, 4);
            memcpy (&conn->buffer + 4, "OK", 2);
            conn->rlen = 6;

            // Read -> Write
            struct kevent changes[2];
            EV_SET (&changes[0], events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            EV_SET (&changes[1], events[i].ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
            kevent (kq, changes, 2, NULL, 0, NULL);
          }
        }
      }
    }
  }

  return 0;
}
