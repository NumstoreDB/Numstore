# Python: Reuse a Query Plan

Every call above compiled its query string, ran it, and threw the compiled form
away. `db.prepare` keeps it, so a query you run in a loop is compiled once:

```python
>>> plan = db.prepare("read out[0:2]")
>>> plan
<Plan 'read out[0:2]'>
>>> plan.read()
array([1, 2], dtype=uint32)
>>> plan.read()
array([1, 2], dtype=uint32)
>>> plan.close()
```

A `Plan` has the same `execute` / `read` / `read_into` / `write` / `var` methods,
each taking an optional transaction as its last argument, and it's a context
manager like everything else:

```python
>>> with db.prepare("insert out 0 2") as plan:
...     with db.begin() as txn:
...         plan.write([10, 20], txn)
2
>>> db.read("read out")
array([10, 20,  1,  2,  3,  4], dtype=uint32)
```

When you're done with the database, close it. Any query after that is an error
rather than a crash:

```python
>>> db.close()
>>> db.closed
True
>>> db.read("read out")
Traceback (most recent call last):
  ...
RuntimeError: database is closed
```

## Plan reference

Every call above compiled its query string, ran it, and threw the compiled form
away. `db.prepare(query) -> Plan` keeps it, so a query you run in a loop is
compiled once:

```python
>>> plan = db.prepare("read prices[0:2]")
>>> plan
<Plan 'read prices[0:2]'>
>>> plan.query
'read prices[0:2]'
>>> plan.read()
array([0.5, 3.5])
>>> plan.read()
array([0.5, 3.5])
>>> plan.close()
>>> plan.closed
True
```

| Member                      | Returns        | What it does                              |
|-----------------------------|----------------|-------------------------------------------|
| `plan.query`                | `str`          | the query it was compiled from            |
| `plan.closed`               | `bool`         | `True` once `close()` has run             |
| `plan.execute(txn=None)`    | `None`         | like `db.execute`                         |
| `plan.read(txn=None)`       | `np.ndarray`   | like `db.read`                            |
| `plan.read_into(out, txn=None)` | `int`      | like `db.read_into`                       |
| `plan.write(data, txn=None)`| `int`          | like `db.write`                           |
| `plan.var(txn=None)`        | `Var`          | the variable the query names              |
| `plan.close()`              | `None`         | releases it; safe to call twice           |

Each method takes an optional transaction as its last argument, so one compiled
plan can run inside any transaction. `Plan` is a context manager too:

```python
>>> with db.prepare("insert stream 0 2") as insert:
...     with db.begin() as txn:
...         insert.write([100, 200], txn)
2
>>> db.read("read stream[0:4]")
array([100, 200,   0,   1], dtype=uint32)
```

Compiling checks the syntax of the query, so a bad query fails at `prepare`
rather than later:

```python
>>> db.prepare("not a query")
Traceback (most recent call last):
  ...
RuntimeError: Expected a valid operation at pos: 0
```

The variable, though, is resolved when the plan runs, not when it compiles. A
plan for a variable that doesn't exist yet compiles fine and fails on use, which
is what lets you prepare against a variable you are about to create:

```python
>>> later = db.prepare("read not_yet[0:]")
>>> later.read()
Traceback (most recent call last):
  ...
RuntimeError: Variable: not_yet doesn't exist
>>> later.close()
```

A plan belongs to the database that made it, and a transaction from a different
database is refused:

```python
>>> other = ns.Database("other.db")
>>> other_txn = other.begin()
>>> db.prepare("read prices").read(other_txn)
Traceback (most recent call last):
  ...
ValueError: transaction belongs to a different database
>>> other_txn.rollback()
>>> other.close()
```
