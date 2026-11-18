Read data in C
==============

There are two ways to read a variable in C. `ns_read` fills a buffer you
already have, and `ns_malloc` allocates one for you.

Read into your own buffer
-------------------------

To walk a large variable, read it in fixed size chunks into your own buffer.
The buffer size caps each read, and the return value tells you how many
elements arrived. `ns_read` never allocates:

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

Let Numstore allocate the buffer
--------------------------------

`ns_malloc` is `ns_read` without the buffer. It allocates one of the right
size, fills it, and writes the byte count (not the element count) through
`dlen`. The buffer comes from the system allocator, so release it with plain
`free`:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
  ns_cleanup ("malloc.db");
  nsdb_t *db = ns_open ("malloc.db");
  ns_execute (db, NULL, "create r u32");

  const uint32_t src[] = {1, 2, 3, 4, 5, 6};
  ns_write (db, NULL, src, sizeof (src), "insert r 0 6");

  b_size    nbytes = 0;
  uint32_t *data = ns_malloc (db, NULL, &nbytes, "read r[0:6:2]");
  if (data == NULL) {
    fprintf (stderr, "read failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }

  printf (
      "%" PRIu64 " bytes, %" PRIu64 " elements:",
      nbytes,
      nbytes / sizeof (uint32_t)
  );
  for (b_size i = 0; i < nbytes / sizeof (uint32_t); i++) {
    printf (" %u", data[i]);
  }
  printf ("\n");

  free (data); // system free, not an ns_ function
  return ns_close (db);
}
```

```text
12 bytes, 3 elements: 1 3 5
```

In Python, `db.read` always allocates and `db.read_into` reads into an array
you bring. See [Move data in and out of Python](move-data-in-python.md).
