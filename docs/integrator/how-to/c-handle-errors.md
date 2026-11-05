# C: Handle Errors

Numstore never aborts on a bad query. Functions returning a pointer return
`NULL`, functions returning `int` return non-zero, and functions returning
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
    printf ("execute failed: %s\n", ns_strerror (db));
  }

  if (ns_execute (db, NULL, "create bad q99") != 0) {
    printf ("bad type: %s\n", ns_strerror (db));
  }

  ns_execute (db, NULL, "create foo u32");
  if (ns_execute (db, NULL, "insert foo 0 10") != 0) {
    printf ("wrong entry point: %s\n", ns_strerror (db));
  }

  double  dest[4];
  sb_size n = ns_read (db, NULL, dest, sizeof (dest), "get foo");
  if (n < 0) {
    printf ("read of a get: %s\n", ns_strerror (db));
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

`ns_open` gives you no message when it fails, because there is no database
handle yet to ask. Use `ns_open_with_error` when you need to report why, and
zero the buffer first - the message is copied into it without a terminator.

A failed query leaves no partial work behind - a query is atomic whether or not
you handed it a transaction - and the handle stays usable afterwards, so you can
report the error and carry on. Plans report through `ns_plan_strerror (plan)`
rather than `ns_strerror (db)`.
