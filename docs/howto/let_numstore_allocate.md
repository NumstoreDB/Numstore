Let Numstore Allocate the Buffer
================================

`ns_malloc` is `ns_read` without a buffer. It allocates a buffer of the correct
size and fills it. It writes the **byte** count through `dlen`. The buffer
comes from the system allocator. So release it with `free`:

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

In Python, `db.read` always allocates. See
[Move data in and out from Python](move_data_in_python.md#get-data-out).
