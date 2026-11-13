Transactions
============

This tutorial teaches transactions in Python. Do the
[query language tutorial](language_tour.md) first.

Each example is a real Python session. Type the examples in order.

```python
>>> import pynumstore as ns
>>> db = ns.Database("tour.db")
```

Queries without a transaction
-----------------------------

In the query language tutorial, each operation ran by itself. When you do not
name a transaction, each query gets an automatic transaction. The query happens
fully or not at all. It is durable when the call returns.

Group queries
-------------

To make several operations into one atomic unit, open a transaction with
`db.begin()`. Run the queries on the transaction, not on `db`. A `Transaction`
has the same five query methods as a `Database`:

```python
>>> db.execute("create prices f64")
>>> txn = db.begin()
>>> txn
<Transaction open>
>>> txn.write("insert prices 0 3", [1.5, 2.25, 3.75])
3
>>> txn.read("read prices")
array([1.5 , 2.25, 3.75])
>>> txn.commit()
>>> db.read("read prices")
array([1.5 , 2.25, 3.75])
```

Roll back
---------

In the transaction, `txn.read` sees the work of the transaction that is not
committed. `rollback` discards all of that work:

```python
>>> txn = db.begin()
>>> txn.write("insert prices 3 2", [4.0, 5.0])
2
>>> txn.read("read prices")
array([1.5 , 2.25, 3.75, 4.  , 5.  ])
>>> txn.rollback()
>>> db.read("read prices")
array([1.5 , 2.25, 3.75])
```

A transaction ends when you commit it or roll it back. If you use it again, you
get an error. Numstore does not ignore the call:

```python
>>> txn
<Transaction closed>
>>> txn.commit()
Traceback (most recent call last):
  ...
RuntimeError: transaction was already committed or rolled back
```

Use the context manager
-----------------------

Usually, use the context manager. It commits when the block ends. It rolls back
if the block raises an error:

```python
>>> with db.begin() as txn:
...     txn.write("insert prices 3 2", [4.0, 5.0])
2
>>> db.read("read prices")
array([1.5 , 2.25, 3.75, 4.  , 5.  ])
```

The `2` is the REPL output of `txn.write` in the block. It is the number of
elements that the insert put in.

If the block raises an error, no data goes in:

```python
>>> try:
...     with db.begin() as txn:
...         txn.write("insert prices 0 2", [98.0, 99.0])
...         raise ValueError("something went wrong")
... except ValueError as exc:
...     print(exc)
2
something went wrong
>>> db.read("read prices")
array([1.5 , 2.25, 3.75, 4.  , 5.  ])
```

One transaction at a time
-------------------------

One transaction is open at a time. `db.begin()` waits until the open
transaction ends. So end each transaction before you start the next one. Then
read [Transactions and concurrency](../concepts/transactions_and_concurrency.md)
to learn why.

Next steps
----------

* [Crash recovery](crash_recovery.md) - see what a commit keeps after a crash.
* [Use transactions in C](../howto/use_transactions_in_c.md).
