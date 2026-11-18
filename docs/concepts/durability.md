Durability and crash recovery
=============================

The commit is the durability point
----------------------------------

After `commit()` returns, your data is on disk and a crash can't take it back.
A query you run without a transaction gets an automatic one, so it's durable by
the time the call returns too.

Call `db.close()` on a clean exit. It flushes and leaves nothing to recover on
the next open. Closing isn't what makes committed data durable - `commit()`
already did that. If a process dies between
`commit()` and `close()`, it loses nothing.

How recovery works
------------------

Numstore writes ahead to a log, and on the next open it recovers with ARIES.
Work from committed transactions is redone and work from transactions that
never committed is undone. Those are the only two outcomes, so you never see
half of a transaction.

C works the same way: a commit is durable the moment `ns_commit` returns, even
if your program never calls `ns_close`.

You don't have to turn recovery on. `ns.Database(path)` and `ns_open` replay the
log before your first query runs.

To watch it happen, crash a process in the
[crash recovery tutorial](../tutorials/crash-recovery.md). For what the files on
disk look like and how to copy them, see
[Back up a database](../guides/back-up-a-database.md).
