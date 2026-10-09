Pynumstore
==========

Python wrapper around the `numstore` C extension - a transactional store for
numpy arrays and structured types.

Quick start
-----------

From the repo root:

```sh
python3 -m venv .venv && . .venv/bin/activate
pip install build
make -C bindings/python install
```

```python
import numpy as np
import pynumstore as ns

with ns.Database("data.db") as db:
    db.execute("create prices f64")
    db.write("insert prices 0 3", np.array([1.5, 2.25, 3.75]))
    dest = db.read("read prices[0:]")
```

More examples in [samples/](samples/).

Development
-----------

`make help` lists the targets. The common ones:

```sh
make install      # build a wheel and install it
make dev          # editable install with test extras
make test         # pytest
make samples      # run every sample
make sdist        # self-contained source distribution
make clean
```

Variables: `PYTHON` (default `python3`), `BUILD_TYPE` (`Release`), `DIST`
(`dist`), `WHEELHOUSE` (`wheelhouse`), e.g. `make install PYTHON=python3.12`.

API
---

### `Database(path)`

- `.execute(query) -> int` Runs a query that takes no data (`create`,
  `delete`, `remove`, ...).
- `.write(query, data) -> int` Runs an `insert`/`write` query with `data` as
  the source. Lists and scalars are converted to the variable's dtype; arrays
  are cast if that keeps their kind (float64 -> float32 is fine, float -> int
  raises `TypeError`).
- `.read(query) -> ndarray` Runs a `read`/`remove` query and returns a new
  array of shape `(n, *dims)`.
- `.read_into(query, out) -> int` Same, into a preallocated, C-contiguous
  array.
- `.get(query) -> Var` Looks up a variable's metadata (`name`, `type`,
  `length`, `tsize`, `shape`, `dtype`).
- `.prepare(query) -> Plan` Compiles a query once to run many times.
- `.begin() -> Transaction`
- `.close()` Safe to call twice.
- Context manager: closes on `__exit__`.

### `Transaction` (from `db.begin()`)

- `.execute`, `.write`, `.read`, `.read_into`, `.get` - same as `Database`
- `.commit()`
- `.rollback()`
- Context manager: commits on clean exit, rolls back on exception. Using it
  after it finished raises `RuntimeError`.

### `Plan` (from `db.prepare(query)`)

- `.execute(txn=None)`, `.write(data, txn=None)`, `.read(txn=None)`,
  `.read_into(out, txn=None)`, `.var(txn=None)` - run the compiled query,
  optionally inside a transaction.
- `.close()`, context manager.

### `to_dtype(type_str) -> np.dtype`

Converts a numstore type string to a numpy dtype. Raises `ValueError` on an
unknown type.

## Query language

```
create <name> <type>              # define a variable
insert <name> <offset> <count>    # write count elements from data
read <name>[start:end]            # read into data, or return new array
remove <name>[start:end]          # delete range; returns removed elements
```

Slices support a step: `name[start:end:step]`.

### Types

Scalars: `u8 i8 u32 i32 f64 ...` (numstore integer/float types)

Composite:

```
struct {
    id     u32,
    name   [10]u8,
    height u32
}
```

Structured data maps directly onto `numpy` structured arrays - build with
`to_dtype()` and construct via `np.array([...], dtype=record_dtype)`.

## Transactions

```python
with db.begin() as txn:
    txn.write("insert log 0 2", data)
# committed automatically on clean exit, rolled back on exception
```

Committed writes are WAL-durable: they survive a crash immediately after
`commit()`. Uncommitted or rolled-back writes never persist, crash or not.
