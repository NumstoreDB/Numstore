Durability and Crash Recovery
=============================

The commit is the durability point
----------------------------------

`commit()` is the point where data becomes durable. When it returns, the data is
on disk. A crash cannot undo it. It is good to call `db.close()` on a correct
exit, but `close()` does not make committed data durable.

The API gives three guarantees:

* `commit()` is the durability point. When it returns, the data is on disk.
* A query with no transaction gets an automatic transaction. So it is durable
  when the call returns too.
* `close()` flushes and leaves nothing to recover. But `close()` is **not**
  what makes committed data durable. `commit()` already did that. If a process
  stops between `commit()` and `close()`, it loses nothing.

How recovery works
------------------

Numstore writes each change to a log first. On the next open, it recovers with
ARIES:

* It redoes the work of a transaction that committed.
* It undoes the work of a transaction that did not commit.

There is no third result. This is the main point: no state shows half of a
transaction.

A commit is durable when `ns_commit` returns. It does not matter if you call
`ns_close` or not.

Recovery is automatic. `ns.Database(path)` and `ns_open` replay the log before
your first query runs. You do not need a flag.

To see recovery work, do the [crash recovery tutorial](../tutorials/crash_recovery.md).
To learn which files are on disk, and how to copy them, see
[Back up a database](../howto/back_up_a_database.md).
