Transactions and Concurrency
============================

Automatic transactions
----------------------

A query that has no transaction gets an automatic transaction. The query
happens fully or not at all. It is durable when the call returns. To group
queries, open a transaction. See the
[transactions tutorial](../tutorials/transactions.md) and
[Use transactions in C](../howto/use_transactions_in_c.md).

One transaction at a time
-------------------------

Numstore runs transactions strictly in series. **One transaction is open at a
time for each handle.** `begin` waits until the open transaction commits or
rolls back. That is why transactions are strictly serializable. The schedule is
the order in which transactions commit. There is no interleaving to think
about.

What this means
---------------

* End each transaction before you open the next one on the same handle. If you
  call `ns_begin` two times without a commit, the second call waits forever. It
  waits for a transaction that only you can end.
* `ns_close` waits while a transaction is open. So commit or roll back first.
* Do not open the same file two times, in one process or in two processes. The
  two calls work now, but the second handle does not see the commits of the
  first handle. One handle owns a database file at a time.

In Python
---------

You must end a transaction before you start the next one on the same handle. If
you open a second transaction before the first one closes, `begin()` waits
forever. It waits for a transaction that only you can end:

```python
# Don't do this: the second begin() waits for the first, which never commits.
txn1 = db.begin()
txn2 = db.begin()   # blocks
```

The same is true for two threads that share one `Database`. The `begin()` that
waits does not release the GIL now. So the thread that holds the open
transaction cannot commit it. The supported pattern is one transaction at a
time, from start to end:

```python
# Fine: each transaction is finished before the next one starts.
for batch in batches:
    with db.begin() as txn:
        txn.write("insert stream 0 %d" % len(batch), batch)
```

A second open of the same file does not help. Two handles on one file both
open, but the second handle does not see the commits of the first:

```python
>>> a = ns.Database("shared.db")
>>> a.execute("create v u32")
>>> a.write("insert v 0 2", [1, 2])
2
>>> b = ns.Database("shared.db")      # opens, but is not coherent with a
>>> a.write("insert v 2 1", [3])
1
>>> b.read("read v")                  # a's third element is missing
array([1, 2], dtype=uint32)
>>> a.close(); b.close()
```

Open one `Database` for each file, in each process, and use it in series.

What is next
------------

Readers and writers at the same time, across threads and processes, are in
development. Until then, use one handle for each file, with one transaction at
a time. A crash at any point is still safe. That is the purpose of
[crash recovery](durability.md).
