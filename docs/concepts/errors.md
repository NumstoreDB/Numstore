The Error Model
===============

This page tells where Numstore keeps an error, and who owns the message. To
find and report errors, see [Handle errors](../howto/handle_errors.md).

Take an error
-------------

An error stays until you take it. `ns_strerror` returns the message of the
pending error and clears it. So a second call returns `NULL`. No other call
clears it. A call that works does not remove an older error. A call that fails
replaces it. To keep this simple, call `ns_strerror` after each call that fails.

This is most important in the one case where `NULL` is not a failure.
`get if exists` on a variable that does not exist returns `NULL` with no error.
To find this case, check that `ns_strerror` is also `NULL`. This is only correct
if no older error stays.

Where an error goes
-------------------

Each failed call puts its error in two places:

* On the **database**. `ns_strerror (db, copy)` takes it. This is the error of
  the last call that failed on that handle, from any thread.
* On the **transaction** that it ran in, if you gave one.
  `ns_txn_strerror (db, tx, copy)` takes it. A transaction belongs to one
  thread. So no other thread can replace this error.

A plan has no error of its own. `ns_plan_strerror (plan, copy)` is
`ns_strerror` on the database of the plan. `ns_commit` and `ns_rollback` end
the transaction, also when they fail. So their error is on the database only.

Who owns the message
--------------------

The `copy` argument sets who owns the string:

* `false` returns a pointer into the database (or the transaction). It stays
  correct until the next failure replaces it. On a handle that only one thread
  uses, this is correct. The examples in these documents use it. It is **not
  thread safe** on a shared handle. The failure of another thread can change
  the string while you read it.
* `true` returns a new string that is yours. Release it with `free`. If there is
  no memory for the copy, you get `NULL` and the error stays.

Errors in Python
----------------

In Python, Numstore gives standard exceptions. There is no custom exception
hierarchy to import.

| Exception             | When                                                                                                                                                   |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `RuntimeError`        | the database or the query layer refused: a missing variable, a query given to the wrong method, a syntax error, a closed database, plan or transaction |
| `TypeError`           | the data or the buffer has the wrong dtype or the wrong Python type                                                                                    |
| `ValueError`          | the shape does not fit, a type is not correct, or a transaction is from a different database                                                           |
| `NotImplementedError` | the type has no numpy form on this platform (`f128`, `cf256`)                                                                                          |
