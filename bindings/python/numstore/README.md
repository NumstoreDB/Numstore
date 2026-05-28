# numstore — Basic Usage

## Opening a Database

Use `ns.open()` as a context manager (recommended), or `Database(path)` for manual lifecycle management.

```python
import numstore as ns

with ns.open("mydb") as db:
    ...

# Manual open/close
db = ns.Database("mydb")
db.close()
```

---

## Variables

A **Variable** is a named, typed array stream. Access one through `db["name"]`, or create it with `db.var()`.

```python
with ns.open("mydb") as db:
    # Open an existing variable
    v = db["temperature"]

    # Create a new variable (dtype required)
    v = db.var("temperature", dtype="f32", create=True)

    # Drop a variable and all its data
    db.delete("temperature")
```

Dtype may be a numstore string (`"f32"`, `"f64"`, `"i32"`, …) or any NumPy dtype.

---

## Reading and Writing

```python
with ns.open("mydb") as db:
    v = db["temperature"]

    val  = v[0]       # read one element
    vals = v[0:10]    # read a slice

    v[0]    = np.float32(99.0)                        # overwrite one element
    v[0:5]  = np.array([1, 2, 3, 4, 5], dtype="f32") # overwrite a slice
```

---

## Appending and Inserting

```python
with ns.open("mydb") as db:
    v = db["temperature"]

    idx = v.append(np.float32(22.5))  # append to end; returns new index
    v.insert(0, np.float32(0.0))      # insert at position, shifts right
```

---

## Removing Elements

```python
with ns.open("mydb") as db:
    v = db["temperature"]

    val  = v.remove(0)    # remove one element and return it
    vals = v.remove(0:5)  # remove a slice and return it
    del v[0]              # remove without returning
```

---

## Transactions

`txn["name"]` works just like `db["name"]` — it returns a Variable whose reads and writes are all part of that transaction. The context manager commits on clean exit and rolls back on any exception.

```python
with ns.open("mydb") as db:
    with db.begin_txn() as txn:
        txn["temperature"][0]    = np.float32(99.0)
        txn["humidity"][0:5]     = arr
        txn["pressure"].append(np.float32(1013.25))
    # committed here; rolled back automatically on exception
```

Creating and deleting variables works the same way inside a transaction:

```python
with db.begin_txn() as txn:
    v = txn.var("pressure", dtype="f32", create=True)
    v.append(np.float32(1013.25))
    txn.delete("old_var")
```

For manual control:

```python
txn = db.begin_txn()
try:
    txn["temperature"][0] = np.float32(99.0)
    txn.commit()
except Exception:
    txn.rollback()
    raise
```

---

## Quick Reference

| Operation | Database | Transaction |
|---|---|---|
| Get variable | `db["x"]` | `txn["x"]` |
| Create variable | `db.var("x", dtype=…, create=True)` | `txn.var("x", dtype=…, create=True)` |
| Delete variable | `db.delete("x")` | `txn.delete("x")` |
| Read element | `db["x"][i]` | `txn["x"][i]` |
| Read slice | `db["x"][i:j]` | `txn["x"][i:j]` |
| Write element | `db["x"][i] = val` | `txn["x"][i] = val` |
| Write slice | `db["x"][i:j] = arr` | `txn["x"][i:j] = arr` |
| Append | `db["x"].append(val)` | `txn["x"].append(val)` |
| Insert | `db["x"].insert(i, val)` | `txn["x"].insert(i, val)` |
| Remove | `db["x"].remove(i)` | `txn["x"].remove(i)` |
| Length | `len(db["x"])` | `len(txn["x"])` |
| Commit / rollback | — | `txn.commit()` / `txn.rollback()` |
