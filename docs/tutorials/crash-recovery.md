Crash recovery
==============

This tutorial crashes a process on purpose and looks at what's left. For how
recovery works, see [Durability and crash recovery](../concepts/durability.md).

Crash from Python
-----------------

[`samples/sample4_crash.py`](../../samples/sample4_crash.py) shows this with
real process crashes. It forks a child for each phase and ends it with
`os._exit(1)`, which gives the library no chance to flush or clean up:

```sh
python samples/sample4_crash.py
```

```text
full contents: 'AAABBAAAAAAA'
phase 2 - commit + crash      (expect BB): 'BB'
phase 3 - no commit + crash   (expect AA): 'AA'
```

The three phases are:

1. Insert ten `A` bytes and close cleanly. This is the baseline.
2. Insert `BB` at offset 3, commit, then crash. The commit happened, so the log
   is replayed on the next open and `BB` is there.
3. Insert `CC` at offset 7 and crash without committing. That work is undone on
   the next open, so those bytes still read `AA` - and the array has 12
   elements, not 14.

Crash from C
------------

A commit is durable the moment `ns_commit` returns, even if your program never
calls `ns_close`.

This program crashes on purpose to show it. Each phase runs in a forked child
that calls `_Exit (1)` instead of `ns_close`, which gives the library no chance
to flush or clean up:

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
the child that never committed is gone, so the array has 12 elements instead of
14. You never see half a transaction.

Crash on purpose with `ns_crash`
--------------------------------

`ns_crash (db)` crashes on purpose from inside a live process. It drops the
handle without flushing, and any open transaction gets rolled back on the next
open. The test suite uses it to test recovery. Use it for that too, and not as a
faster way to close.
