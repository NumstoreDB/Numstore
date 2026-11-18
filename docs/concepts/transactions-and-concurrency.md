Transactions and concurrency
============================

Automatic transactions
----------------------

When you don't name a transaction, each query gets an automatic one: it either
happens completely or not at all, and it's durable by the time the call returns.
To group queries, open a transaction - see the
[transactions tutorial](../tutorials/transactions.md) and
[Use transactions in C](../guides/transactions-in-c.md).

One transaction at a time
-------------------------

Each handle has at most one open transaction. `begin` blocks until the open one
commits or rolls back. That makes transactions strictly serializable: they run
in the order they commit, with no interleaving to reason about.

What this means
---------------

* Finish each transaction before you open the next one on the same handle. If
  you call `ns_begin` twice without committing, the second call blocks forever
  waiting on a transaction only you can finish.
* `ns_close` blocks while a transaction is open, so commit or roll back first.
* Don't open the same file twice, in one process or across processes. Both
  opens succeed, but the second handle doesn't see the first handle's
  commits. Treat a database file as owned by one handle at a time.

In Python
---------

Python has the same rule. If you open a second transaction without finishing the
first, `begin()` blocks forever:

```python
# Don't do this: the second begin() waits for the first, which never commits.
txn1 = db.begin()
txn2 = db.begin()   # blocks
```

Two threads sharing one `Database` run into the same thing. The waiting
`begin()` doesn't release the GIL, so the thread holding the open
transaction never gets to commit. Run one transaction at a time, start to
finish:

```python
# Fine: each transaction is finished before the next one starts.
for batch in batches:
    with db.begin() as txn:
        txn.write("insert stream 0 %d" % len(batch), batch)
```

Opening the same file twice doesn't get around this either. Both handles open,
but the second one doesn't see the first one's commits:

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

Limits
------

Numstore doesn't support concurrent readers and writers across threads or
processes. Use one handle per file with one transaction at a time. A crash at
any point is still safe - that's what [recovery](durability.md) is for.
