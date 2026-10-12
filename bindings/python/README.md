pynumstore
==========

Python bindings for [Numstore](https://github.com/NumstoreDB/Numstore), an
embedded ACID database for arrays. 

```python
import numpy as np
import pynumstore as ns

with ns.Database("prices.db") as db:
    db.execute("create prices f64")
    db.write("insert prices 0 3", np.array([1.5, 2.25, 3.75]))

    with db.begin() as tx: 
        tx.write("insert prices 3 2", [4.0, 5.0])

    print(db.read("read prices[0:]"))   # [1.5  2.25 3.75 4.   5.  ]
    print(db.read("read prices[0::2]")) # [1.5  3.75 5.  ]
```

Install
-------

You need Python 3.9+, a C11 compiler and CMake 3.20+. From the repo root:

```sh
python3 -m venv .venv && . .venv/bin/activate
pip install ./bindings/python
```

This compiles the C library and the extension in one step. There are no runtime
dependencies besides numpy.

How it works
------------

A database is a set of named **variables**. Each variable has a **type** (the
layout of one element) and holds a growable array of elements of that type. You
talk to it with short queries:

| Query                  | Python call              | What it does                       |
| ---------------------- | ------------------------ | ---------------------------------- |
| `create prices f64`    | `db.execute(q)`          | create a variable                  |
| `delete prices`        | `db.execute(q)`          | delete a variable                  |
| `insert prices 0 3`    | `db.write(q, data)`      | insert 3 elements at index 0       |
| `write prices[0:3]`    | `db.write(q, data)`      | overwrite elements in place        |
| `read prices[0:10:2]`  | `db.read(q)`             | read a slice (`start:stop:step`)   |
| `remove prices[0:2]`   | `db.read(q)`             | remove a slice and return it       |
| `get prices`           | `db.get(q)`              | look up type, length and shape     |

Only `insert` and `remove` change a variable's length. Slices follow numpy
rules, except negative steps aren't supported yet. 

Types
-----

Every Numstore type has a numpy equivalent, and `ns.to_dtype()` converts
between them:

| Numstore                       | numpy                                       |
| ------------------------------ | ------------------------------------------- |
| `u8` ... `u64`, `i8` ... `i64` | `np.uint8` ... `np.int64`                   |
| `f16`, `f32`, `f64`            | `np.float16`, `np.float32`, `np.float64`    |
| `cf64`, `cf128`                | `np.complex64`, `np.complex128`             |
| `cu16`, `ci32`, ...            | struct `[("re", ...), ("im", ...)]`         |
| `[10][20] f32`                 | `np.dtype((np.float32, (10, 20)))`          |
| `struct { a u8, b u32 }`       | `[("a", np.uint8), ("b", np.uint32)]`       |
| `union { a u8, b u32 }`        | struct dtype with every offset at 0         |

The number in a type name is its size in bits, so `cf64` is two `f32`s, the
same as numpy's `complex64`. numpy has no complex integers, so those become a
two-field struct.

Structs and unions are packed, with no padding. A struct's size is the sum of
its fields and a union's is its largest field. Types nest freely.

### Structured data

```python
point = "struct { x f32, y f32, z f32 }"

with ns.Database("points.db") as db:
    db.execute(f"create points {point}")

    pts = np.zeros(3, dtype=ns.to_dtype(point))
    pts["x"] = [1, 2, 3]
    db.write("insert points 0 3", pts)

    out = db.read("read points[0:]")
    print(out["x"])                     # [1. 2. 3.]
```

### Fixed-size arrays

An array type makes each element a whole block, such as an image:

```python
with ns.Database("frames.db") as db:
    db.execute("create frames [256][256] u8")
    db.write("insert frames 0 2", np.zeros((2, 256, 256), dtype=np.uint8))

    db.read("read frames[0:]").shape
```

Reads always return shape `(n, *dims)`, where `n` is the number of elements
read.

Transactions
------------

Every call outside a transaction is atomic on its own. To group calls together,
use `db.begin()`:

```python
with db.begin() as tx:
    tx.write("insert log 0 2", data)
    tx.execute("remove staging[0:]")
# committed here, or rolled back if the block raised
```

Committed writes are durable: they survive a crash right after `commit()`.
Uncommitted or rolled-back writes never reach the file.

Prepared queries
----------------

If you run the same query often, compile it once with `db.prepare()`:

```python
with db.prepare("read prices[0:]") as plan:
    for _ in range(1000):
        latest = plan.read()
```

Any plan method takes an optional `txn` argument to run it inside a
transaction.

API
---

### `Database(path)`

Opens `path` and creates it if needed. Works as a context manager.

| Method                     | Returns       | Notes                                             |
| -------------------------- | ------------- | ------------------------------------------------- |
| `execute(query)`           | `int`         | queries that take no data: `create`, `delete`, ...  |
| `write(query, data)`       | `int`         | `insert` / `write`; see casting below             |
| `read(query)`              | `ndarray`     | `read` / `remove`; returns a new array            |
| `read_into(query, out)`    | `int`         | same, into a preallocated C-contiguous array      |
| `get(query)`               | `Var`         | `.name .type .length .tsize .shape .dtype`        |
| `prepare(query)`           | `Plan`        | compiled query to run many times                  |
| `begin()`                  | `Transaction` |                                                   |
| `close()`                  |               | safe to call twice                                |

Examples
--------

[samples/](samples/) has runnable scripts, and `make samples` runs them all.

Development
-----------

```sh
make dev          # editable install with test extras
make test         # run pytest
make samples      # run every sample
make install      # build a wheel and install it
make sdist        # self-contained source distribution
make clean
make help         # everything else
```

You can override `PYTHON` (default `python3`), `BUILD_TYPE` (`Release`), `DIST`
(`dist`) and `WHEELHOUSE` (`wheelhouse`). For example,
`make install PYTHON=python3.12`.

The bindings are a thin layer over the C library. All the C glue is in
`src/c/ns_pynumstore.c`.

License
-------

Apache 2.0. See [LICENSE](../../LICENSE).
