Crash Recovery
==============

This tutorial crashes a process on purpose. Then it shows what Numstore keeps.
[Durability and crash recovery](../concepts/durability.md) tells how recovery
works.

Crash from Python
-----------------

[`samples/sample4_crash.py`](../../samples/sample4_crash.py) uses real process
crashes. It forks one child for each phase. It stops the child with
`os._exit(1)`. So the library cannot flush or clean up.

Run it:

```sh
python samples/sample4_crash.py
```

```text
full contents: 'AAABBAAAAAAA'
phase 2 - commit + crash      (expect BB): 'BB'
phase 3 - no commit + crash   (expect AA): 'AA'
```

The three phases are:

1. Insert ten `A` bytes and close the database correctly. This is the start
   state.
2. Insert `BB` at offset 3, **commit**, then crash. The commit happened. So on
   the next open, Numstore replays the log, and `BB` is there.
3. Insert `CC` at offset 7 and crash **without a commit**. On the next open,
   Numstore undoes that work. So those bytes are still `AA`. The array has 12
   elements, not 14.

Crash from C
------------

A commit is durable when `ns_commit` returns. It does not matter if you call
`ns_close` or not.

This program shows that. It crashes on purpose. Each phase runs in a forked
child. The child calls `_Exit (1)`, not `ns_close`. So the library cannot flush
or clean up:

```c
#include "numstore.h"

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define PATH "durability.db"

// Commit, then die without closing: ARIES redoes the committed work on reopen.
static void
commit_then_crash (void)
{
  nsdb_t *db = ns_open (PATH);
  nsdb_txn_t  *tx = ns_begin (db);
  ns_write (db, tx, "BB", 2, "insert log 3 2");
  ns_commit (db, tx);
  _Exit (1); // no ns_close
}

// Die mid transaction: ARIES undoes the uncommitted work on reopen.
static void
crash_without_commit (void)
{
  nsdb_t *db = ns_open (PATH);
  nsdb_txn_t  *tx = ns_begin (db);
  ns_write (db, tx, "CC", 2, "insert log 7 2");
  _Exit (1); // no ns_commit, no ns_close
}

static void
in_child (void (*phase) (void))
{
  pid_t pid = fork ();
  if (pid == 0) {
    phase ();
    _Exit (0);
  }
  waitpid (pid, NULL, 0);
}

int
main (void)
{
  ns_cleanup (PATH);

  nsdb_t *db = ns_open (PATH);
  ns_execute (db, NULL, "create log u8");
  ns_write (db, NULL, "AAAAAAAAAA", 10, "insert log 0 10");
  ns_close (db);

  in_child (commit_then_crash);
  in_child (crash_without_commit);

  // Reopening replays the log: committed work is there, the rest is gone.
  db = ns_open (PATH);
  char    out[16] = {0};
  sb_size n = ns_read (db, NULL, out, sizeof (out) - 1, "read log[0:]");
  printf ("after recovery: %.*s\n", (int)n, out);
  return ns_close (db);
}
```

```text
after recovery: AAABBAAAAAAA
```

The committed `BB` is at offset 3, where the first child put it. The `CC` from
the child that did not commit is gone. The array has 12 elements, not 14. There
is no state between the two. You never see half of a transaction.

Crash on purpose with `ns_crash`
--------------------------------

`ns_crash (db)` does the same thing in a live process. It drops the handle and
does not flush. On the next open, Numstore rolls back any transaction that was
open. The test suite uses it to test recovery. Use it for the same purpose. Do
not use it as a fast way to close.

More samples
------------

Two longer samples are in the repository:

* [`samples/smfile_sample4_rollback_commit.c`](../../samples/smfile_sample4_rollback_commit.c)
  shows all five combinations of commit, crash and rollback.
* [`samples/sample4_crash.py`](../../samples/sample4_crash.py) shows the same
  idea in Python.
