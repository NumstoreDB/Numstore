# C: Read into Your Own Buffer

`ns_read` fills a buffer you already have. It never allocates, which makes it
the function for reading a large variable in fixed size chunks - the buffer size
caps each read, and the return value tells you how many elements arrived:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

int
main (void)
{
  ns_cleanup ("chunks.db");
  nsdb_t *db = ns_open ("chunks.db");
  ns_execute (db, NULL, "create stream u32");

  uint32_t src[100];
  for (int i = 0; i < 100; i++) {
    src[i] = (uint32_t)i;
  }
  ns_write (db, NULL, src, sizeof (src), "insert stream 0 100");

  // Walk the whole variable 16 elements at a time.
  uint32_t chunk[16];
  b_size   start = 0;
  b_size   total = 0;
  for (;;) {
    sb_size n = ns_read (
        db,
        NULL,
        chunk,
        sizeof (chunk),
        "read stream[%" PRIu64 ":%" PRIu64 "]",
        start,
        start + 16
    );
    if (n <= 0) {
      break;
    }
    total += (b_size)n;
    start += (b_size)n;
  }

  printf ("read %" PRIu64 " elements in chunks of 16\n", total);
  return ns_close (db);
}
```

```text
read 100 elements in chunks of 16
```
