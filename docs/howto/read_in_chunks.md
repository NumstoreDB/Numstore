Read a Large Variable in Chunks
===============================

To read a large variable, read it in fixed size chunks into a buffer that you
own. The size of the buffer limits each read. The return value is the number
of elements that arrived.

In C
----

`ns_read` fills a buffer that you have. It never allocates:

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

In Python
---------

`read_into` fills a numpy array that you have. It never allocates:

```python
>>> db.execute("create stream u32")
>>> db.write("insert stream 0 10", np.arange(10, dtype=np.uint32))
10
>>> chunk = np.zeros(4, dtype=np.uint32)
>>> total, start = 0, 0
>>> while True:
...     n = db.read_into(f"read stream[{start}:{start + 4}]", chunk)
...     if n == 0:
...         break
...     total += n
...     start += n
>>> total
10
```

