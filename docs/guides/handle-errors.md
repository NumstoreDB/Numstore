Handle errors
=============

This guide shows how to catch and report a failure, in C and in Python. For
where an error is stored and who owns the message, see
[the error model](../concepts/errors.md).

Handle errors in C
------------------

A bad query never stops your program. Functions that return a pointer return
`NULL`, functions that return `int` return non-zero, and functions that return
`sb_size` return a negative number. `ns_strerror` then describes what happened:

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

  ns_execute (db, NULL, "create counts u32");
  if (ns_execute (db, NULL, "insert counts 0 10") != 0) {
    printf ("wrong entry point: %s\n", ns_strerror (db, false));
  }

  double  dest[4];
  sb_size n = ns_read (db, NULL, dest, sizeof (dest), "get counts");
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

`ns_open` gives you no message when it fails, because there's no database
handle yet to ask. Use `ns_open_with_error` when you need to report why, and
zero the buffer first - the message is copied into it without a terminator.

### Continue after an error

A failed query leaves no partial work behind, because a query is atomic whether
or not you gave it a transaction. The handle still works afterwards, so you can
report the error and keep going.

### Take the error of a transaction

On a handle that other threads share, take your transaction's error with
`ns_txn_strerror`, ask for a copy, and release it with `free`:

```c
if (ns_write (db, tx, src, sizeof (src), "insert counts 0 4") < 0) {
  char *msg = (char *)ns_txn_strerror (db, tx, true);
  fprintf (stderr, "insert failed: %s\n", msg != NULL ? msg : "unknown");
  free (msg);
}
```

Handle errors in Python
-----------------------

Numstore raises standard Python exceptions.
[The error model](../concepts/errors.md#errors-in-python) lists which one you get
and when. The messages are meant for people, so logging the exception is usually
enough:

```python
>>> try:
...     db.write("insert counts 0 1", np.array([3.5]))
... except TypeError as exc:
...     print(f"refused: {exc}")
refused: cannot write float64 data to a uint32 variable
```

A failed query is rolled back on its own, so you can catch the error inside a
transaction and keep using the transaction. If you let the error escape the
`with` block, the whole transaction is rolled back:

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

Both examples assume `counts` is a `u32` variable that holds `[1, 2]`.
