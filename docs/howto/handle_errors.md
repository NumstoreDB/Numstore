Handle Errors
=============

This guide shows how to find and report errors in C and in Python.
[The error model](../concepts/errors.md) tells where Numstore keeps an error and
who owns the message.

Handle errors in C
------------------

Numstore does not abort on a bad query. A function that returns a pointer
returns `NULL`. A function that returns `int` returns a value that is not zero.
A function that returns `sb_size` returns a negative number. Then call
`ns_strerror` to get the message:

```c
#include "numstore.h"

#include <stdio.h>

int
main (void)
{
  // ns_open_with_error copies its message without a terminator, so zero the
  // buffer. MAX_ERRSTR_LEN is the longest message Numstore produces.
  char    errstr[MAX_ERRSTR_LEN] = {0};
  nsdb_t *db = ns_open_with_error ("no/such/dir/x.db", errstr, sizeof (errstr));
  if (db == NULL) {
    printf ("open failed: %s\n", errstr);
  }

  ns_cleanup ("errors.db");
  db = ns_open ("errors.db");

  if (ns_execute (db, NULL, "not a real query") != 0) {
    printf ("execute failed: %s\n", ns_strerror (db, false));
  }

  if (ns_execute (db, NULL, "create bad q99") != 0) {
    printf ("bad type: %s\n", ns_strerror (db, false));
  }

  ns_execute (db, NULL, "create foo u32");
  if (ns_execute (db, NULL, "insert foo 0 10") != 0) {
    printf ("wrong entry point: %s\n", ns_strerror (db, false));
  }

  double  dest[4];
  sb_size n = ns_read (db, NULL, dest, sizeof (dest), "get foo");
  if (n < 0) {
    printf ("read of a get: %s\n", ns_strerror (db, false));
  }

  return ns_close (db);
}
```

```text
open failed: open_file_rw no/such/dir/x.db: No such file or directory
execute failed: Expected a valid operation at pos: 0
bad type: Expected type at position 2, got token type TT_IDENTIFIER
wrong entry point: Only supported exec commands are REMOVE/CREATE/DELETE
read of a get: Only supported read commands are READ/REMOVE
```

### Report a failed open

When `ns_open` fails, it gives no message. There is no database handle yet to
ask. To report the cause, use `ns_open_with_error`. Set the buffer to zero
first, because Numstore copies the message into it without a terminator.

### Continue after an error

A failed query leaves no partial work. A query is atomic, with or without a
transaction. The handle stays usable after the error. So you can report the
error and continue.

### Take the error of a transaction

On a handle that other threads also use, take the error of your transaction
with `ns_txn_strerror`. Set `copy` to `true` and release the string with `free`:

```c
if (ns_write (db, tx, src, sizeof (src), "insert foo 0 4") < 0) {
  char *msg = (char *)ns_txn_strerror (db, tx, true);
  fprintf (stderr, "insert failed: %s\n", msg != NULL ? msg : "unknown");
  free (msg);
}
```

Handle errors in Python
-----------------------

Numstore gives standard Python exceptions.
[The error model](../concepts/errors.md#errors-in-python) lists when each one
occurs. The messages are for people to read. So it is usually enough to log the
exception:

```python
>>> try:
...     db.write("insert counts 0 1", np.array([3.5]))
... except TypeError as exc:
...     print(f"refused: {exc}")
refused: cannot write float64 data to a uint32 variable
```

Numstore rolls back a failed query. So when you catch an error in a
transaction, the transaction stays usable. If the error goes out of the `with`
block, Numstore rolls back the whole transaction:

```python
>>> with db.begin() as txn:
...     try:
...         txn.write("insert counts 0 1", np.array([3.5]))
...     except TypeError:
...         txn.write("insert counts 0 1", np.array([3], dtype=np.uint32))
1
>>> db.read("read counts")
array([3, 1, 2], dtype=uint32)
```

In these two examples, `counts` is a `u32` variable that holds `[1, 2]`.
