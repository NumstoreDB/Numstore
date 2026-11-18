The error model
===============

This page covers where an error is stored after something fails, and who owns
the message string. For the practical side (checking return values and printing
errors), see [Handle errors](../guides/handle-errors.md).

Take an error
-------------

An error stays put until you take it. `ns_strerror` returns the message of the
pending error and clears it, so a second call returns `NULL`. Nothing else
clears it. A call that succeeds leaves an older error in place, and a call that
fails replaces it. A good habit is to call `ns_strerror` after every call that
fails.

This matters for `get if exists`, the one place where `NULL` isn't a failure.
On a variable that doesn't exist it returns `NULL` with no error, and you tell
the two cases apart by checking that `ns_strerror` is also `NULL`. That only
works if you didn't leave an earlier error behind.

Where an error goes
-------------------

Every failed call records its error in two places:

* On the database. `ns_strerror (db, copy)` takes it. This is the error from
  the last call that failed on that handle, from any thread.
* On the transaction it ran in, if you passed one. `ns_txn_strerror (db, tx,
  copy)` takes it. A transaction belongs to one thread, so no other thread can
  replace this one.

A plan has no error of its own. `ns_plan_strerror (plan, copy)` is
`ns_strerror` on the database the plan belongs to. `ns_commit` and
`ns_rollback` end the transaction even when they fail, so their error is on the
database only.

Who owns the message
--------------------

The `copy` argument decides who owns the string:

* `false` returns a pointer into the database (or the transaction). It stays
  valid until the next failure overwrites it. That's fine on a handle only one
  thread uses, and it's what the examples in these docs do. On a shared handle
  it isn't thread safe: another thread's failure can rewrite the string while
  you're reading it.
* `true` returns a new string that you own. Release it with `free`. If there's
  no memory for the copy, you get `NULL` and the error stays pending.

Errors in Python
----------------

Numstore raises standard Python exceptions, so there's no custom exception
hierarchy to import.

| Exception             | When                                                                                                                                                   |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `RuntimeError`        | the database or the query layer refused: a missing variable, a query handed to the wrong method, a syntax error, a closed database, plan or transaction |
| `TypeError`           | the data or buffer has the wrong dtype or the wrong Python type                                                                                        |
| `ValueError`          | the shape doesn't fit, a type is malformed, or a transaction is from another database                                                                  |
| `NotImplementedError` | the type has no NumPy representation on this platform (`f128`, `cf256`)                                                                                |
