The Error Model
===============

This page is about where an error lives once something fails, and who owns the
message. For the practical side - checking return values and reporting - see
[Handle errors](../howto/handle_errors.md).

Take an error
-------------

An error stays where it is until you take it. `ns_strerror` returns the message
of the pending error and clears it, so a second call returns `NULL`. Nothing
else clears it: a call that succeeds leaves an older error in place, and a call
that fails replaces it. The habit that keeps this simple is to call
`ns_strerror` after every call that fails.

That matters most for the one place where `NULL` is not a failure. `get if
exists` on a variable that isn't there returns `NULL` with no error, and the
way to tell is that `ns_strerror` is also `NULL` - which only holds if no
earlier error was left behind.

Where an error goes
-------------------

Every failed call records its error in two places:

* On the **database**. `ns_strerror (db, copy)` takes it. This is the error of
  the last call to fail on that handle, from any thread.
* On the **transaction** it ran in, if you passed one. `ns_txn_strerror (db,
  tx, copy)` takes it. A transaction belongs to one thread, so nobody else can
  replace this one.

A plan has no error of its own. `ns_plan_strerror (plan, copy)` is
`ns_strerror` on the database the plan belongs to. `ns_commit` and
`ns_rollback` end the transaction even when they fail, so their error is on the
database only.

Who owns the message
--------------------

The `copy` argument decides who owns the string:

* `false` returns a pointer into the database (or the transaction). It is valid
  until the next failure overwrites it. On a handle that only one thread uses
  that is fine, and it is what the examples in these docs do. It is **not
  thread safe** on a shared handle: another thread's failure can rewrite the
  string while you are reading it.
* `true` returns a new string that is yours. Release it with `free`. If there
  is no memory for the copy you get `NULL` and the error stays pending.

Errors in Python
----------------

Numstore reports failures as standard Python exceptions. There is no custom
exception hierarchy to import.

| Exception             | When                                                                                                                                                   |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `RuntimeError`        | the database or the query layer refused: a missing variable, a query handed to the wrong method, a syntax error, a closed database, plan or transaction |
| `TypeError`           | the data or buffer has the wrong dtype or the wrong Python type                                                                                        |
| `ValueError`          | the shape doesn't fit, a type is malformed, or a transaction is from another database                                                                  |
| `NotImplementedError` | the type has no numpy representation on this platform (`f128`, `cf256`)                                                                                |
