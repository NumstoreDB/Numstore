Pynumstore
==========

Python wrapper around the `numstore` C extension - a transactional store for
numpy arrays and structured types.

Quick start
-----------

```python
import numpy as np
import pynumstore as ns

with ns.Database("data.db") as db:
    db.execute("create prices f64")
    db.execute("insert prices 0 3", np.array([1.5, 2.25, 3.75]))
    dest = db.execute("read prices[0:]")
```

API
---

### `Database(path)`

- `.execute(query, data=None) -> int | ndarray` Runs a query. `data` is the
  source for `insert`, or the destination buffer for `read`/`remove`. Omit it
  on read/remove to auto-allocate and return an array; with `data` given, the
  element count is returned instead.
- `.begin() -> Transaction`
- `.close()`
- Context manager: closes on `__exit__`.

### `Transaction` (from `db.begin()`)

- `.execute(query, data=None)` - same semantics as `Database.execute`
- `.commit()`
- `.rollback()`
- Context manager: commits on clean exit, rolls back on exception. Double
  commit/rollback is a no-op. Using after close raises `RuntimeError`.

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
    txn.execute("insert log 0 2", data)
# committed automatically on clean exit, rolled back on exception
```

Committed writes are WAL-durable: they survive a crash immediately after
`commit()`. Uncommitted or rolled-back writes never persist, crash or not.
