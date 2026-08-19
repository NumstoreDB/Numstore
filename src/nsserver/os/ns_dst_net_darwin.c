#include "nsserver/os/ns_net_darwin.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

struct dst_net_darwin
{
  struct dst_net_darwin_params params;
};

i_net_darwin build_dst_net_darwin (struct dst_net_darwin_params)
{}

static int
dst_socket (void *self, int domain, int type, int protocol, error *e)
{
  const int fd = socket (domain, type, protocol);
  if (unlikely (fd == -1)) {
    error_causef (e, ERR_IO, "socket: %s", strerror (errno));
    return error_trace (e);
  }
  return fd;
}

static err_t
dst_setsockopt (
    void       *self,
    int         fd,
    int         level,
    int         optname,
    const void *optval,
    socklen_t   optlen,
    error      *e
)
{
  if (unlikely (setsockopt (fd, level, optname, optval, optlen) == -1)) {
    error_causef (e, ERR_IO, "setsockopt: %s", strerror (errno));
    return error_trace (e);
  }
  return SUCCESS;
}

static err_t
dst_bind (void *self, int fd, const struct sockaddr *addr, socklen_t addrlen, error *e)
{
  if (unlikely (bind (fd, addr, addrlen) == -1)) {
    error_causef (e, ERR_IO, "bind: %s", strerror (errno));
    return error_trace (e);
  }
  return SUCCESS;
}

static err_t
dst_listen (void *self, int fd, int backlog, error *e)
{
  if (unlikely (listen (fd, backlog) == -1)) {
    error_causef (e, ERR_IO, "listen: %s", strerror (errno));
    return error_trace (e);
  }
  return SUCCESS;
}

static int
dst_fcntl_get (void *self, int fd, int cmd, error *e)
{
  const int flags = fcntl (fd, cmd);
  if (unlikely (flags == -1)) {
    error_causef (e, ERR_IO, "fcntl_get: %s", strerror (errno));
    return error_trace (e);
  }
  return flags;
}

static err_t
dst_fcntl_set (void *self, int fd, int cmd, int flags, error *e)
{
  if (unlikely (fcntl (fd, cmd, flags) == -1)) {
    error_causef (e, ERR_IO, "fcntl_set: %s", strerror (errno));
    return error_trace (e);
  }
  return SUCCESS;
}

static err_t
dst_kqueue (void *self, int *dest, error *e)
{
  const int kq = kqueue ();
  if (unlikely (kq == -1)) {
    error_causef (e, ERR_IO, "kqueue: %s", strerror (errno));
    return error_trace (e);
  }
  *dest = kq;
  return SUCCESS;
}

static int
dst_kevent (
    void                  *self,
    int                    kq,
    const struct kevent   *changelist,
    int                    nchanges,
    struct kevent         *eventlist,
    int                    nevents,
    const struct timespec *timeout,
    error                 *e
)
{
  const int n = kevent (kq, changelist, nchanges, eventlist, nevents, timeout);
  if (unlikely (n == -1)) {
    error_causef (e, ERR_IO, "kevent: %s", strerror (errno));
    return error_trace (e);
  }
  return n;
}

static int
dst_accept (void *self, int fd, struct sockaddr *addr, socklen_t *addrlen, error *e)
{
  const int client = accept (fd, addr, addrlen);
  if (unlikely (client == -1)) {
    error_causef (e, ERR_IO, "accept: %s", strerror (errno));
    return error_trace (e);
  }
  return client;
}

static ssize_t
dst_send (void *self, int fd, const void *buf, size_t len, int flags, error *e)
{
  const ssize_t sent = send (fd, buf, len, flags);
  if (unlikely (sent == -1)) {
    error_causef (e, ERR_IO, "send: %s", strerror (errno));
    return error_trace (e);
  }
  return sent;
}

static ssize_t
dst_recv (void *self, int fd, void *buf, size_t len, int flags, error *e)
{
  const ssize_t recvd = recv (fd, buf, len, flags);
  if (unlikely (recvd == -1)) {
    error_causef (e, ERR_IO, "recv: %s", strerror (errno));
    return error_trace (e);
  }
  return recvd;
}

static err_t
dst_close (void *self, int fd, error *e)
{
  if (unlikely (close (fd) == -1)) {
    error_causef (e, ERR_IO, "close: %s", strerror (errno));
    return error_trace (e);
  }
  return SUCCESS;
}

struct i_net_darwin_vtable dst_nvtable = {
    .i_socket     = dst_socket,
    .i_setsockopt = dst_setsockopt,
    .i_bind       = dst_bind,
    .i_listen     = dst_listen,
    .i_fcntl_get  = dst_fcntl_get,
    .i_fcntl_set  = dst_fcntl_set,
    .i_kqueue     = dst_kqueue,
    .i_kevent     = dst_kevent,
    .i_accept     = dst_accept,
    .i_send       = dst_send,
    .i_recv       = dst_recv,
    .i_close      = dst_close,
};
