Transactions and Concurrency
============================

Automatic transactions
----------------------

When you don't name a transaction, each query gets an automatic one: it either
happens completely or not at all, and it's durable by the time the call returns.
To group queries, open a transaction - see the
[transactions tutorial](../tutorials/transactions.md) and
[Use transactions in C](../howto/use_transactions_in_c.md).

One transaction at a time
-------------------------

Numstore serializes transactions strictly: **one transaction is open at a time
per handle**. `begin` blocks until the open one commits or rolls back, which is
what makes transactions strictly serializable - the schedule is the order they
commit in, with no interleaving to reason about.

What this means
---------------

What that means in practice:

* Finish each transaction before opening the next one on the same handle. Call
  `ns_begin` twice without committing and the second call blocks forever on a
  transaction only you can finish.
* `ns_close` blocks while a transaction is open, so commit or roll back first.
* Don't open the same file twice, in one process or across processes. Both calls
  succeed today, but the second handle does not see the first handle's commits.
  Treat a database file as owned by one handle at a time.

In Python
---------

The practical consequence today is that you must finish a transaction before
starting the next one on the same handle. Open a second one without closing the
first and `begin()` blocks forever waiting for a transaction only you can
finish:

```python
# Don't do this: the second begin() waits for the first, which never commits.
txn1 = db.begin()
txn2 = db.begin()   # blocks
```

The same applies to two threads sharing one `Database`: the waiting `begin()`
does not currently release the GIL, so the thread holding the open transaction
never gets to commit. One transaction at a time, start to finish, is the
supported pattern:

```python
# Fine: each transaction is finished before the next one starts.
for batch in batches:
    with db.begin() as txn:
        txn.write("insert stream 0 %d" % len(batch), batch)
```

Opening the same file twice is also not a way around this. Two handles on one
file both open, but the second does not see the first's commits:

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

Open one `Database` per file, per process, and use it serially.

What's next
-----------

Concurrent readers and writers across threads and processes are in development.
Until they land, the supported shape is one handle per file with one transaction
in flight at a time; a crash at any point is still safe, which is what
[recovery](durability.md) is for.
