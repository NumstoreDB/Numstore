# C: Your First Database

From the repository root:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
./build/release/bin/ns_sample1_basic_crud
```

What you get:

* `include/numstore.h`: the only header you need
* `build/release/lib/libnumstore.a`: the whole library in one static archive
* `build/release/bin/`: one binary per C sample in `samples/`, plus `ns_simtest`

To compile a program of your own against it:

```sh
cc -std=c11 -I include myprog.c build/release/lib/libnumstore.a -o myprog
```

On Linux add `-pthread -lm`. For a debug build, use
`-B build/debug -DCMAKE_BUILD_TYPE=Debug` instead.

The [C API Reference](../reference/c-api.md) has the rest: linking from CMake, and a
complete program for queries, transactions, crash recovery, packed structs,
prepared plans and smart files.

## A first program

Open a database, create a variable, insert data, read it back. A Numstore
database is a single file and there is no server to start:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

int
main (void)
{
  ns_cleanup ("prices.db");

  nsdb_t *db = ns_open ("prices.db");
  if (db == NULL) {
    fprintf (stderr, "could not open prices.db\n");
    return 1;
  }

  if (ns_execute (db, NULL, "create prices f64") != 0) {
    fprintf (stderr, "create failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }

  const double in[] = {1.5, 2.25, 3.75};
  sb_size      wrote = ns_write (db, NULL, in, sizeof (in), "insert prices 0 3");
  if (wrote < 0) {
    fprintf (stderr, "insert failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }
  printf ("inserted %" PRId64 " elements\n", wrote);

  double  out[8];
  sb_size read = ns_read (db, NULL, out, sizeof (out), "read prices[0:]");
  if (read < 0) {
    fprintf (stderr, "read failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }

  printf ("read %" PRId64 " elements:", read);
  for (sb_size i = 0; i < read; i++) {
    printf (" %g", out[i]);
  }
  printf ("\n");

  return ns_close (db);
}
```

```text
inserted 3 elements
read 3 elements: 1.5 2.25 3.75
```

Three things carry across every example below:

* **The `NULL` second argument is the transaction.** Pass `NULL` and the query
  gets an automatic transaction of its own - atomic and durable by the time the
  call returns. See [Transactions](../reference/c-api.md) to group queries instead.
* **Queries are `printf` format strings.** `ns_execute`, `ns_read`, `ns_write`,
  `ns_get_var` and `ns_plan_fcreate` are variadic, so an offset or a count can
  come from a variable:
  `ns_write (db, NULL, src, sizeof (src), "insert prices %d %d", offset, count)`.
* **Buffer sizes are bytes, query counts are elements.** `sizeof (out)` tells
  Numstore how much room it has; `read prices[0:]` says which elements you want.
  The return value is the number of **elements** moved, and reads stop when
  either the range or the buffer runs out.

Which function you call depends on what has to cross the boundary - nothing,
metadata, data in, or data out:

| Function                                | Queries it accepts         | Returns                       |
|-----------------------------------------|----------------------------|-------------------------------|
| `ns_execute (db, tx, fmt, ...)`         | `CREATE` `DELETE` `REMOVE` | `int` - `0` on success        |
| `ns_get_var (db, tx, fmt, ...)`         | anything naming a variable | `nsdb_var_t *`, `NULL` on failure |
| `ns_write (db, tx, src, slen, fmt, ...)`| `INSERT` `WRITE`           | `sb_size` - elements written, `< 0` on failure |
| `ns_read (db, tx, dest, dlen, fmt, ...)`| `READ` `REMOVE`            | `sb_size` - elements read, `< 0` on failure |
| `ns_malloc (db, tx, &dlen, fmt, ...)`   | `READ` `REMOVE`            | `void *`, `NULL` on failure   |
