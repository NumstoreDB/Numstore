Getting Started
===============

Install Numstore and run your first queries, from Python or C. The queries are
identical in both - only the host language changes.

* [Python](#python)
* [C](#c)
* [Next steps](#next-steps)

Python
======

Install
-------

Install the bindings:

```sh
pip install pynumstore
```

Numpy is the only dependency. 

A first round trip
------------------

Create a variable, insert some data, read it back:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> with ns.Database("quickstart.db") as db:
...     db.execute("create prices f64")
...     db.write("insert prices 0 3", np.array([1.5, 2.25, 3.75]))
...     db.read("read prices[0:]")
3
array([1.5 , 2.25, 3.75])
```

Open a database
---------------

A Numstore database is a single file. Open one:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("tour.db")
>>> db
<Database 'tour.db'>
>>> db.path
'tour.db'
```

`Database` is a context manager, so in a script you'd normally write
`with ns.Database("tour.db") as db:` and let the block close it. In the REPL
it's easier to keep `db` around and call `db.close()` when you're done.

The [query language tutorial](tutorials/language_tour.md) picks up from here.

C
=

Build the library
-----------------

Build the library out of the repository:

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

To compile and link your program, see
[Build and link the C library](howto/build_and_link.md).

A first program
---------------

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
    fprintf (stderr, "create failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }

  const double in[] = {1.5, 2.25, 3.75};
  sb_size      wrote
      = ns_write (db, NULL, in, sizeof (in), "insert prices 0 3");
  if (wrote < 0) {
    fprintf (stderr, "insert failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }
  printf ("inserted %" PRId64 " elements\n", wrote);

  double  out[8];
  sb_size read = ns_read (db, NULL, out, sizeof (out), "read prices[0:]");
  if (read < 0) {
    fprintf (stderr, "read failed: %s\n", ns_strerror (db, false));
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

Three things carry across every C example
------------------------------------------

* **The `NULL` second argument is the transaction.** Pass `NULL` and the query
  gets an automatic transaction of its own - atomic and durable by the time the
  call returns. See [Use transactions in C](howto/use_transactions_in_c.md) to
  group queries instead.
* **Queries are `printf` format strings.** `ns_execute`, `ns_read`, `ns_write`,
  `ns_get_var` and `ns_plan_fcreate` are variadic, so an offset or a count can
  come from a variable:
  `ns_write (db, NULL, src, sizeof (src), "insert prices %d %d", offset, count)`.
* **Buffer sizes are bytes, query counts are elements.** `sizeof (out)` tells
  Numstore how much room it has; `read prices[0:]` says which elements you want.
  The return value is the number of **elements** moved, and reads stop when
  either the range or the buffer runs out.

Which function to call
----------------------

Which function you call depends on what has to cross the boundary - nothing,
metadata, data in, or data out:

| Function                                 | Queries it accepts         | Returns                                        |
| ---------------------------------------- | -------------------------- | ---------------------------------------------- |
| `ns_execute (db, tx, fmt, ...)`          | `CREATE` `DELETE` `REMOVE` | `int` - `0` on success                         |
| `ns_get_var (db, tx, fmt, ...)`          | anything naming a variable | `nsdb_var_t *`, `NULL` on failure              |
| `ns_write (db, tx, src, slen, fmt, ...)` | `INSERT` `WRITE`           | `sb_size` - elements written, `< 0` on failure |
| `ns_read (db, tx, dest, dlen, fmt, ...)` | `READ` `REMOVE`            | `sb_size` - elements read, `< 0` on failure    |
| `ns_malloc (db, tx, &dlen, fmt, ...)`    | `READ` `REMOVE`            | `void *`, `NULL` on failure                    |

Next steps
==========

* [The query language](tutorials/language_tour.md) - the seven queries, step
  by step.
* [Glossary](GLOSSARY.md) - the vocabulary used throughout the docs.
