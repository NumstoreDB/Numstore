# Transactions

Every one of the operations above ran on its own. When you don't name a
transaction, each query gets an automatic one: it either happens completely or
not at all, and it's durable by the time the call returns.

To group several operations into one atomic unit, open a transaction with
`db.begin()` and run the queries on it instead of on `db`. A `Transaction` has
the same five query methods as a `Database`:

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

Inside the transaction, `txn.read` sees the transaction's own uncommitted work.
`rollback` throws all of it away:

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

A transaction is finished once you commit or roll it back, and using it again is
an error rather than a silent no-op:

```python
>>> txn
<Transaction closed>
>>> txn.commit()
Traceback (most recent call last):
  ...
RuntimeError: transaction was already committed or rolled back
```

In practice you want the context manager, which commits when the block finishes
and rolls back if it raises:

```python
>>> with db.begin() as txn:
...     txn.write("insert prices 3 2", [4.0, 5.0])
2
>>> db.read("read prices")
array([1.5 , 2.25, 3.75, 4.  , 5.  ])
```

That `2` is the REPL echoing the return value of `txn.write` from inside the
block - the two elements it inserted.

If the block raises, nothing lands:

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
