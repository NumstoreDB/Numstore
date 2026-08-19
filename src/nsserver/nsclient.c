#include "core/ns_stdtypes.h"

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

#define err_check(condition, op) \
  if (!(condition)) {            \
    perror (op);                 \
    exit (-1);                   \
  }

void
recv_buf (int socket, void *dest, size_t max_size, int flags)
{
  // Read the length prefix
  u32     len    = 0;
  ssize_t result = recv (socket, &len, 4, 0);
  printf ("Recv (prefix): %ld\n", result);
  err_check (result == 4, "recv");
  len = ntohl (len);
  err_check (len > 4 && len <= max_size + 4, "len");

  // Read the rest
  result = recv (socket, dest, len - 4, flags);
  printf ("Recv %s: %ld\n", (char *)dest, result);
  err_check (result == (ssize_t)(len - 4), "recv");
}

void
send_str (int socket, const char *src, int flags)
{
  // Send the length prefix
  u32 len        = strlen (src) + 4;
  len            = htonl (len);
  ssize_t result = send (socket, &len, 4, flags);
  printf ("Sent (prefix): %ld\n", result);
  err_check (result == (ssize_t)4, "send");

  // Send the rest
  len    = ntohl (len) - 4;
  result = send (socket, src, len, flags);
  printf ("Sent %s: %ld\n", src, result);
  err_check (result == (ssize_t)(len), "send");
}

int
main ()
{
  // Open the socket
  int client = socket (PF_INET, SOCK_STREAM, 0);
  err_check (client >= 0, "socket");

  // Listening
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons (9001);
  addr.sin_addr.s_addr = INADDR_ANY;

  char recv_buffer[2048];

  int  result = connect (client, (struct sockaddr *)&addr, sizeof (addr));
  err_check (result >= 0, "connect");

  {
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  {
    send_str (client, "create a struct { a f32, b i32 };", 0);
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  {
    send_str (client, "insert a[0];", 0);
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  {
    send_str (client, "insert a[10];", 0);
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  {
    send_str (client, "read a[0:20];", 0);
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  {
    send_str (client, "remove a[0:20];", 0);
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  {
    send_str (client, "read a[0:20];", 0);
    recv_buf (client, recv_buffer, sizeof (recv_buffer), 0);
  }

  return 0;
}
