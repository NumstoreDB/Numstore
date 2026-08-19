#ifndef NS_DARWIN_H
#define NS_DARWIN_H

#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "core/ns_error.h"

typedef struct i_net_darwin i_net_darwin;

struct i_net_darwin_vtable
{
  int (*i_socket) (void *self, int domain, int type, int protocol, error *e);

  err_t (*i_setsockopt) (
      void       *self,
      int         fd,
      int         level,
      int         optname,
      const void *optval,
      socklen_t   optlen,
      error      *e
  );

  err_t (*i_bind) (void *self, int fd, const struct sockaddr *addr, socklen_t addrlen, error *e);
  err_t (*i_listen) (void *self, int fd, int backlog, error *e);
  int (*i_fcntl_get) (void *self, int fd, int cmd, error *e);
  err_t (*i_fcntl_set) (void *self, int fd, int cmd, int flags, error *e);
  err_t (*i_kqueue) (void *self, int *out_kq, error *e);
  int (*i_kevent) (
      void                  *self,
      int                    kq,
      const struct kevent   *changelist,
      int                    nchanges,
      struct kevent         *eventlist,
      int                    nevents,
      const struct timespec *timeout,
      error                 *e
  );

  int (*i_accept) (void *self, int fd, struct sockaddr *addr, socklen_t *addrlen, error *e);
  ssize_t (*i_send) (void *self, int fd, const void *buf, size_t len, int flags, error *e);
  ssize_t (*i_recv) (void *self, int fd, void *buf, size_t len, int flags, error *e);
  err_t (*i_close) (void *self, int fd, error *e);
};

struct i_net_darwin
{
  const struct i_net_darwin_vtable *funcs;
  void                             *self;
};

// Default darwin
extern i_net_darwin net_darwin;

#ifdef TESTING

struct dst_net_darwin_params
{};

i_net_darwin build_dst_net_darwin (struct dst_net_darwin_params);

#endif

#endif
