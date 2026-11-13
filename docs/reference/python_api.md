Python API Reference
====================

This page tells about each public name in `pynumstore`. Each name has an
example that you can run. If Numstore is new to you, read
[Getting started](../GETTING_STARTED.md) first. Then read the
[query language tutorial](../tutorials/language_tour.md). For the C library, see
[the C API](c_api.md).

The examples on this page are one real Python session. Run them in order. They
use one database.

* [Install](#install)
* [Database](#database)
* [Queries](#queries)
* [Transactions](#transactions)
* [Concurrency and threading](#concurrency-and-threading)
* [Durability](#durability)
* [Plan](#plan)
* [Var](#var)
* [to_dtype](#to_dtype)
* [Errors](#errors)
* [Version](#version)

Install
-------

```sh
pip install pynumstore
```

Numpy is the only dependency. The wheel installs it. For a full round trip, see
[Getting started](../GETTING_STARTED.md).

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> with ns.Database("quickstart.db") as db:
...     db.execute("create prices f64")
...     db.write("insert prices 0 3", np.array([1.5, 2.25, 3.75]))
...     db.read("read prices[0:]")
3
array([1.5 , 2.25, 3.75])
```

Database
--------

`ns.Database(path)` opens the database file at `path`. If the file does not
exist, it makes the file. A database is one file. There is no server to start.

```python
>>> db = ns.Database("api.db")
>>> db
<Database 'api.db'>
>>> db.path
'api.db'
>>> db.closed
False
```

| Member                                        | Type                 | What it does                                            |
| --------------------------------------------- | -------------------- | ------------------------------------------------------- |
| `Database(path)`                              | `str \| os.PathLike` | opens (or makes) the database file                      |
| `db.path`                                     | `str`                | the path that opened it                                 |
| `db.closed`                                   | `bool`               | `True` after `close()` runs                             |
| `db.close()`                                  | `None`               | flushes and closes; you can call it two times           |
| `db.begin()`                                  | `Transaction`        | opens a transaction - see [Transactions](#transactions) |
| `db.prepare(query)`                           | `Plan`               | compiles a query to use again - see [Plan](#plan)       |
| `db.execute/get/get_all/read/read_into/write` |                      | the query methods - see [Queries](#queries)             |

`path` accepts all `os.PathLike` values. So `pathlib.Path` works:

```python
>>> from pathlib import Path
>>> tmp = ns.Database(Path("from_path.db"))
>>> tmp.path
'from_path.db'
>>> tmp.close()
```

`Database` is a context manager. Usually, open a database this way. The block
closes the database, also if the body raises an exception:

```python
>>> with ns.Database("scratch.db") as scratch:
...     scratch.execute("create tmp u8")
...     scratch.closed
False
>>> scratch.closed
True
```

A second close does nothing. A query on a closed database is an error. It does
not cause a crash:

```python
>>> scratch.close()
>>> scratch.read("read tmp")
Traceback (most recent call last):
  ...
RuntimeError: database is closed
```

Queries
-------

A query is a string. The method that you give it to depends on what goes across
the boundary: nothing, metadata, data in, or data out. `Database`,
`Transaction` and `Plan` all have the same set of methods.

| Method               | Queries it accepts         | Returns                        |
| -------------------- | -------------------------- | ------------------------------ |
| `db.execute(q)`      | `CREATE` `DELETE` `REMOVE` | `None`                         |
| `db.get(q)`          | each query with a variable | a [`Var`](#var)                |
| `db.get_all()`       | no query                   | `list[Var]`, one per variable  |
| `db.write(q, data)`  | `INSERT` `WRITE`           | `int` - elements written       |
| `db.read(q)`         | `READ` `REMOVE`            | a new `numpy` array            |
| `db.read_into(q, o)` | `READ` `REMOVE`            | `int` - elements read into `o` |

### execute

`execute(query) -> None` runs a query that moves no data. These are `CREATE`,
`DELETE`, and `REMOVE` when you do not want the removed data:

```python
>>> db.execute("create v u32")
>>> db.write("insert v 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.execute("remove v[0:2]")
>>> db.read("read v")
array([3, 4, 5, 6], dtype=uint32)
>>> db.execute("delete v")
```

A query that needs a buffer is an error:

```python
>>> db.execute("read v")
Traceback (most recent call last):
  ...
RuntimeError: Only supported exec commands are REMOVE/CREATE/DELETE
```

### write

`write(query, data) -> int` sends data in. It returns the number of elements
that it wrote. It accepts `INSERT` and `WRITE`. `data` is a value that numpy can
change into an array: a list, a tuple, or an array:

```python
>>> db.execute("create prices f64")
>>> db.write("insert prices 0 2", [1.5, 2.5])
2
>>> db.write("insert prices 2 2", (3.5, 4.5))
2
>>> db.write("insert prices 4 2", np.array([5.5, 6.5]))
2
>>> db.read("read prices")
array([1.5, 2.5, 3.5, 4.5, 5.5, 6.5])
```

The count in the query and the length of `data` can be different. The write goes
through the two together. It stops when one of them has no more elements. That
is why you must read the return value.

If you give a numpy array, Numstore checks its dtype. It permits only a cast of
the same kind. So it refuses a write that loses data. It does not truncate the
data silently:

```python
>>> db.execute("create counts u32")
>>> db.write("insert counts 0 2", np.array([1.5, 2.5]))
Traceback (most recent call last):
  ...
TypeError: cannot write float64 data to a uint32 variable
```

A plain list does not get this check. It goes through `np.asarray(data, dtype)`,
and numpy casts it as numpy decides. If the type is important, give an array:

```python
>>> db.write("insert counts 0 2", [1.5, 2.5])
2
>>> db.read("read counts")
array([1, 2], dtype=uint32)
```

### read

`read(query) -> np.ndarray` allocates memory. It gets the dtype and the shape of
the variable. It allocates exactly that, and gives the array to you. It accepts
`READ`. It also accepts `REMOVE` when you want the removed data:

```python
>>> db.read("read prices[0:3]")
array([1.5, 2.5, 3.5])
>>> db.read("remove prices[0:2]")
array([1.5, 2.5])
>>> db.read("read prices")
array([3.5, 4.5, 5.5, 6.5])
```

An empty variable gives an empty array. An empty *range* of a variable that is
not empty has nothing to allocate, so it raises an exception. If you want "the
data that is there, possibly nothing", use `read_into`:

```python
>>> db.execute("create empty u32")
>>> db.read("read empty")
array([], dtype=uint32)
>>> db.read("read prices[9:]")
Traceback (most recent call last):
  ...
RuntimeError: numstore operation failed
```

### read_into

`read_into(query, out) -> int` does not allocate memory. You give the buffer. It
fills as much of the buffer as it can. It returns the number of elements that it
wrote. Use it to read a large variable in chunks, or to read into memory that
you already own:

```python
>>> buf = np.zeros(2, dtype=np.float64)
>>> db.read_into("read prices", buf)
2
>>> buf
array([3.5, 4.5])
```

The buffer must have exactly the dtype of the variable. It must be C-contiguous
and writable. There is no conversion, because the data goes directly into your
memory:

```python
>>> db.read_into("read prices", np.zeros(4, dtype=np.uint32))
Traceback (most recent call last):
  ...
TypeError: out has dtype uint32, expected float64
>>> db.read_into("read prices", [0, 0, 0, 0])
Traceback (most recent call last):
  ...
TypeError: out must be a numpy array, not list
>>> db.read_into("read prices", np.zeros(8, dtype=np.float64)[::2])
Traceback (most recent call last):
  ...
ValueError: out must be C-contiguous and writable
```

This reads a full variable in chunks of the same size. For more about this
pattern, see [Read in chunks](../howto/read_in_chunks.md):

```python
>>> db.execute("create stream u32")
>>> db.write("insert stream 0 10", np.arange(10, dtype=np.uint32))
10
>>> chunk = np.zeros(4, dtype=np.uint32)
>>> total, start = 0, 0
>>> while True:
...     n = db.read_into(f"read stream[{start}:{start + 4}]", chunk)
...     if n == 0:
...         break
...     total += n
...     start += n
>>> total
10
```

### get and get_all

`get(query) -> Var` returns a snapshot of the metadata of a variable. Each query
that names a variable works. The query does not run:

```python
>>> db.get("get prices")
Var(name='prices', type='f64', length=4, tsize=8)
>>> db.get("remove prices[0:]")      # prices is not modified
Var(name='prices', type='f64', length=4, tsize=8)
```

A variable that does not exist is an error:

```python
>>> db.get("get nope")
Traceback (most recent call last):
  ...
RuntimeError: Variable: nope doesn't exist
```

`get_all() -> list[Var]` takes no query. It returns one `Var` for each variable
in the database. The order is not specified. If you need a stable list, sort it:

```python
>>> sorted(v.name for v in db.get_all())
['counts', 'empty', 'prices', 'stream']
>>> {v.name: v.length for v in db.get_all()}["stream"]
10
```

Transactions
------------

Each query above ran by itself. A query that has no transaction gets an
automatic transaction. It happens fully or not at all. It is durable when the
call returns.

`db.begin() -> Transaction` puts many queries into one atomic unit. A
`Transaction` has the same query methods as a `Database`. A read on it sees its
own work that is not committed:

```python
>>> txn = db.begin()
>>> txn
<Transaction open>
>>> txn.closed
False
>>> txn.write("insert prices 0 1", [0.5])
1
>>> txn.read("read prices")
array([0.5, 3.5, 4.5, 5.5, 6.5])
>>> txn.commit()
>>> db.read("read prices")
array([0.5, 3.5, 4.5, 5.5, 6.5])
```

| Member                                         | Returns | What it does                                    |
| ---------------------------------------------- | ------- | ----------------------------------------------- |
| `txn.commit()`                                 | `None`  | makes the work durable and closes the transaction |
| `txn.rollback()`                               | `None`  | discards the work and closes the transaction    |
| `txn.closed`                                   | `bool`  | `True` after commit or rollback                 |
| `txn.execute/get/get_all/read/read_into/write` |         | the same query methods, in the transaction      |

`rollback` undoes all of the work of the transaction:

```python
>>> txn = db.begin()
>>> txn.write("insert prices 0 2", [98.0, 99.0])
2
>>> txn.rollback()
>>> db.read("read prices")
array([0.5, 3.5, 4.5, 5.5, 6.5])
```

A transaction ends when you commit it or roll it back. To use it again is an
error. It does not silently do nothing:

```python
>>> txn
<Transaction closed>
>>> txn.commit()
Traceback (most recent call last):
  ...
RuntimeError: transaction was already committed or rolled back
```

Usually, use the context manager. It commits when the block ends. It rolls back
if the block raises an exception:

```python
>>> with db.begin() as txn:
...     txn.write("insert prices 5 1", [7.5])
1
>>> db.read("read prices")
array([0.5, 3.5, 4.5, 5.5, 6.5, 7.5])
>>> try:
...     with db.begin() as txn:
...         txn.execute("remove prices")
...         raise ValueError("changed my mind")
... except ValueError as exc:
...     print(exc)
changed my mind
>>> db.read("read prices")
array([0.5, 3.5, 4.5, 5.5, 6.5, 7.5])
```

For a step-by-step lesson, do the
[transactions tutorial](../tutorials/transactions.md).

Concurrency and threading
-------------------------

One transaction is open at a time for each database handle. `begin()` waits
until the open transaction ends. So end each transaction before you start the
next one on the same handle. Open one `Database` for each file, in each
process, and use it in series.

For the full rules and examples, see
[Transactions and concurrency](../concepts/transactions_and_concurrency.md).

Durability
----------

`commit()` is the durability point. When it returns, the data is on disk.
Recovery is automatic: `ns.Database(path)` replays the log before your first
query runs.

For the three guarantees and how recovery works, see
[Durability](../concepts/durability.md). To see a crash, do the
[crash recovery tutorial](../tutorials/crash_recovery.md). To learn which files
are on disk, see [Back up a database](../howto/back_up_a_database.md).

Plan
----

Each call above compiled its query string, ran it, and discarded the compiled
form. `db.prepare(query) -> Plan` keeps the compiled form. So a query that you
run in a loop compiles one time:

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

| Member                          | Returns      | What it does                                |
| ------------------------------- | ------------ | ------------------------------------------- |
| `plan.query`                    | `str`        | the query that it was compiled from         |
| `plan.closed`                   | `bool`       | `True` after `close()` runs                 |
| `plan.execute(txn=None)`        | `None`       | like `db.execute`                           |
| `plan.read(txn=None)`           | `np.ndarray` | like `db.read`                              |
| `plan.read_into(out, txn=None)` | `int`        | like `db.read_into`                         |
| `plan.write(data, txn=None)`    | `int`        | like `db.write`                             |
| `plan.var(txn=None)`            | `Var`        | the variable that the query names           |
| `plan.close()`                  | `None`       | releases it; you can call it two times      |

Each method takes an optional transaction as its last argument. So one compiled
plan can run in each transaction. `Plan` is also a context manager:

```python
>>> with db.prepare("insert stream 0 2") as insert:
...     with db.begin() as txn:
...         insert.write([100, 200], txn)
2
>>> db.read("read stream[0:4]")
array([100, 200,   0,   1], dtype=uint32)
```

The compile step checks the syntax of the query. So a bad query fails at
`prepare`, not later:

```python
>>> db.prepare("not a query")
Traceback (most recent call last):
  ...
RuntimeError: Expected a valid operation at pos: 0
```

But Numstore finds the variable when the plan runs, not when it compiles. A plan
for a variable that does not exist yet compiles. It fails when you use it. So
you can prepare a plan for a variable that you will make later:

```python
>>> later = db.prepare("read not_yet[0:]")
>>> later.read()
Traceback (most recent call last):
  ...
RuntimeError: Variable: not_yet doesn't exist
>>> later.close()
```

A plan belongs to the database that made it. It refuses a transaction from a
different database:

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

For when to use a plan, see [Reuse queries](../howto/reuse_queries.md).

Var
---

`Var` is a frozen dataclass. It is a snapshot of the metadata of a variable, at
the time that you asked. It continues to show those old values. So get it again
after you change the variable.

```python
>>> var = db.get("get prices")
>>> var.name, var.type
('prices', 'f64')
>>> var.length, var.tsize
(6, 8)
>>> var.dtype
dtype('float64')
>>> var.shape
(6,)
>>> len(var)
6
```

| Field / property | Type              | What it is                                       |
| ---------------- | ----------------- | ------------------------------------------------ |
| `var.name`       | `str`             | the name of the variable                         |
| `var.type`       | `str`             | its Numstore type, as text                       |
| `var.length`     | `int`             | number of elements when the snapshot was taken   |
| `var.tsize`      | `int`             | size of **one** element, in bytes                |
| `var.dtype`      | `np.dtype`        | the numpy dtype of one element                   |
| `var.shape`      | `tuple[int, ...]` | the shape of an array of the full variable       |
| `len(var)`       | `int`             | the same as `var.length`                         |

For a strict array type, `tsize` is the size of the full element. `shape` adds
the dimensions of the element at the end:

```python
>>> db.execute("create grid [2][3] u8")
>>> db.write("insert grid 0 4", np.zeros((4, 2, 3), dtype=np.uint8))
4
>>> grid = db.get("get grid")
>>> grid.tsize, grid.dtype, grid.shape
(6, dtype('uint8'), (4, 2, 3))
```

to_dtype
--------

`ns.to_dtype(type_str) -> np.dtype` gives the numpy dtype of one element of a
Numstore type. It does not use a database. It is the fastest way to find the
size of a type:

```python
>>> ns.to_dtype("u32")
dtype('uint32')
>>> ns.to_dtype("struct { a u16, b i64, c f32 }").itemsize
14
>>> ns.to_dtype("[10][20] u32").itemsize
800
>>> ns.to_dtype("union { a u32, b cf128 }")
dtype({'names': ['a', 'b'], 'formats': ['<u4', '<c16'], 'offsets': [0, 0], 'itemsize': 16})
```

If numpy cannot show a type, `to_dtype` raises an exception. It does not guess.
`f128` and `cf256` need a 16 byte float. Numpy has one only where the
`long double` of the platform is 16 bytes:

```python
>>> ns.to_dtype("struct { i u8, i u32 }")
Traceback (most recent call last):
  ...
ValueError: duplicate key: i
```

```python
>>> try:
...     ns.to_dtype("f128")
... except NotImplementedError as exc:
...     print("not representable here")
... else:
...     print("16 byte long double")  # doctest: +SKIP
not representable here
```

The database stores these types on all platforms. Only the numpy mapping
changes with the platform. So `get` works where `read` and `write` cannot:

```python
>>> db.execute("create wide f128")
>>> db.get("get wide").tsize
16
```

For the full type system, see [Types](../concepts/types.md).

Errors
------

Numstore gives failures as standard Python exceptions. There is no custom
exception hierarchy to import.

| Exception             | When                                                                                                                                                       |
| --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `RuntimeError`        | the database or the query layer refused: a missing variable, a query given to the wrong method, a syntax error, a closed database, plan or transaction |
| `TypeError`           | the data or the buffer has the wrong dtype or the wrong Python type                                                                                        |
| `ValueError`          | the shape does not fit, a type is not correct, or a transaction is from a different database                                                               |
| `NotImplementedError` | the type has no numpy form on this platform (`f128`, `cf256`)                                                                                              |

The messages are easy to read. So usually, it is sufficient to log the
exception:

```python
>>> try:
...     db.write("insert counts 0 1", np.array([3.5]))
... except TypeError as exc:
...     print(f"refused: {exc}")
refused: cannot write float64 data to a uint32 variable
```

Numstore rolls back a query that fails. So if you catch an error in a
transaction, you can continue to use the transaction. If the error goes out of
the `with` block, Numstore rolls back the full transaction:

```python
>>> with db.begin() as txn:
...     try:
...         txn.write("insert counts 0 1", np.array([3.5]))
...     except TypeError:
...         txn.write("insert counts 0 1", np.array([3], dtype=np.uint32))
1
>>> db.read("read counts")
array([3, 1, 2], dtype=uint32)
```

For the error model, see [Errors](../concepts/errors.md).

Version
-------

```python
>>> ns.__version__          # doctest: +SKIP
'1.5.0'
>>> isinstance(ns.__version__, str)
True
```

`pynumstore` has type hints (`py.typed`). So editors and type checkers see each
signature on this page. You do not need a stub package.

```python
>>> db.close()
```
