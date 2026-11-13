Durability and Crash Recovery
=============================

The commit is the durability point
----------------------------------

`commit()` is the durability point. When it returns, the data is on disk and a
crash cannot take it back. `db.close()` is still worth calling on a clean exit,
but it isn't what makes committed data durable.

Three facts the API guarantees:

* `commit()` is the durability point. When it returns, the data is on disk.
* A query with no transaction gets an automatic one, so it is durable by the
  time the call returns too.
* `close()` flushes and leaves nothing to recover, but it is **not** what makes
  committed data durable - `commit()` already did that. A process killed between
  `commit()` and `close()` loses nothing.

How recovery works
------------------

Numstore writes ahead to a log and recovers with ARIES on the next open: work
from a committed transaction is redone, work from a transaction that never
committed is undone. There is no third outcome, which is the whole point - no
state exists where half of a transaction is visible.

The same holds in C: a commit is durable the moment `ns_commit` returns, whether
or not you get to call `ns_close`.

Recovery is automatic: `ns.Database(path)` and `ns_open` replay the log before
your first query runs, and need no flag.

To watch it happen, crash a process in the
[crash recovery tutorial](../tutorials/crash_recovery.md). For what the files on
disk look like and how to copy them, see
[Back up a database](../howto/back_up_a_database.md).
