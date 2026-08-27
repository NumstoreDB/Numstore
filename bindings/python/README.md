# pynumstore

Python bindings for [numstore](https://github.com/NumstoreDB/Numstore), an embedded
ACID database for arrays.

## Install

From a checkout of the numstore repo (this package compiles the C extension by
shelling out to the top-level `Makefile`, so it needs the full source tree, not
just `bindings/python`):

```
pip install ./bindings/python
```

For development, install it editable so edits to the wrapper are picked up
without reinstalling:

```
pip install -e ./bindings/python
```

## Usage

```python
import numpy as np
import pynumstore as ns

with ns.Database("mydb.nsdb") as db:
    db.execute("create foo u32")

    src = np.arange(5, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    dest = np.zeros(5, dtype=np.uint32)
    n = db.execute(f"read foo[0:] blimit {dest.nbytes}", dest)

    with db.begin() as txn:
        txn.execute("insert foo 5 3", np.array([10, 11, 12], dtype=np.uint32))
        # commits on a clean exit, rolls back if an exception is raised
```

`Database.execute` and `Transaction.execute` both take a fully-formed query
string plus an optional numpy array used as the data payload (source data for
`insert`/`write`, destination buffer for `read`/`remove`). See
[docs/index.md](../../docs/index.md) in the main repo for the query language.

`ns.to_dtype("u32")` converts a numstore type string to the matching numpy
`dtype`.

## Testing

```
pip install -e ./bindings/python[test]
pytest bindings/python/tests
```

## Building a wheel

```
make python-package
```

builds the extension and packages it into a wheel under `build/python/target/`.
This currently only builds for the host platform - cross-platform wheels are a
later step.
