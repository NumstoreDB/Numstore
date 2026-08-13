#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT       8080
#define MAX_EVENTS 64
#define BUF_SIZE   4096

static void
set_nonblocking (int fd)
{
  int flags = fcntl (fd, F_GETFL, 0);
  fcntl (fd, F_SETFL, flags | O_NONBLOCK);
}

static int
make_listener (int port)
{
  /*
   * Open up a new ipv4 socket with stream mode
   */
  int fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
  {
    perror ("socket");
    exit (1);
  }

  int yes = 1;
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes));

  struct sockaddr_in addr = {0};
  addr.sin_family         = AF_INET;
  addr.sin_addr.s_addr    = INADDR_ANY;
  addr.sin_port           = htons (port);

  if (bind (fd, (struct sockaddr *)&addr, sizeof (addr)) == -1)
  {
    perror ("bind");
    exit (1);
  }
  if (listen (fd, SOMAXCONN) == -1)
  {
    perror ("listen");
    exit (1);
  }

  set_nonblocking (fd);
  return fd;
}

static void
kq_add (int kq, int fd, int filter)
{
  struct kevent ev;
  EV_SET (&ev, fd, filter, EV_ADD | EV_ENABLE, 0, 0, NULL);
  kevent (kq, &ev, 1, NULL, 0, NULL);
}

static void
kq_del (int kq, int fd, int filter)
{
  struct kevent ev;
  EV_SET (&ev, fd, filter, EV_DELETE, 0, 0, NULL);
  kevent (kq, &ev, 1, NULL, 0, NULL);
}

static void
close_conn (int kq, int fd)
{
  kq_del (kq, fd, EVFILT_READ);
  close (fd);
  printf ("closed fd %d\n", fd);
}

int
main (void)
{
  int listen_fd = make_listener (PORT);

  int kq = kqueue ();
  if (kq == -1)
  {
    perror ("kqueue");
    exit (1);
  }

  kq_add (kq, listen_fd, EVFILT_READ);

  struct kevent events[MAX_EVENTS];
  char          buf[BUF_SIZE];

  printf ("listening on port %d\n", PORT);

  for (;;)
  {
    int n = kevent (kq, NULL, 0, events, MAX_EVENTS, NULL);
    if (n == -1)
    {
      if (errno == EINTR)
      {
        continue;
      }
      perror ("kevent");
      break;
    }

    for (int i = 0; i < n; i++)
    {
      int fd = (int)events[i].ident;

      // Error / EOF conditions come back as flags on the event
      if (events[i].flags & EV_EOF)
      {
        close_conn (kq, fd);
        continue;
      }

      if (fd == listen_fd)
      {
        // Accept as many pending connections as are ready
        for (;;)
        {
          struct sockaddr_in client_addr;
          socklen_t          client_len = sizeof (client_addr);
          int client_fd = accept (listen_fd, (struct sockaddr *)&client_addr, &client_len);
          if (client_fd == -1)
          {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
              break;
            }
            perror ("accept");
            break;
          }
          set_nonblocking (client_fd);
          kq_add (kq, client_fd, EVFILT_READ);
          printf ("accepted fd %d from %s\n", client_fd, inet_ntoa (client_addr.sin_addr));
        }
      }
      else
      {
        // Readable client socket — echo back
        ssize_t r = read (fd, buf, sizeof (buf));
        if (r <= 0)
        {
          // r == 0: client closed. r < 0: real error (already
          // filtered EAGAIN wouldn't show up here since kqueue
          // only told us it's readable).
          close_conn (kq, fd);
          continue;
        }
        ssize_t written = write (fd, buf, r);
        if (written == -1)
        {
          perror ("write");
          close_conn (kq, fd);
        }
      }
    }
  }

  close (kq);
  close (listen_fd);
  return 0;
}
