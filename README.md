<p align="center">
  <img src="docs/logo.png" alt="NumStore Logo" width="200"/>
</p>

# Numstore

**A database for bytes** 

---

## Overview

Numstore is organized into three layered components:

| Component | Description |
|---|---|
| **Smart Files** | A crash-safe file abstraction with transaction support, named sections, and O(log N) inner mutations |
| **Numstore** | Smart Files with a rich type system - a full database for typed arrays |
| **PyNumstore** | A Python wrapper around Numstore |

---

## PyNumstore

PyNumstore is not published to PyPI (yet). The package is self-contained under `libs/pynumstore` and uses [scikit-build-core](https://scikit-build-core.readthedocs.io/) - the C extension is compiled automatically as part of the install step.

> Note: Pynumstore is experimental - expect bugs around wrapping python and c - the core logic is found in numstore / nscore

### Quick Start

**1. Install**

```bash
pip install -e libs/pynumstore
```

This compiles the C extension and installs the package in editable mode. CMake and a C compiler must be available on your `PATH`.

---

**2. Run a sample**

```bash
python samples/pynumstore/sample1.py
```

All PyNumstore samples live in `samples/pynumstore/`.

---

### Opening a Database

Use `ns.open()` as a context manager (recommended), or `Database(path)` for manual lifecycle management.

```python
import numstore as ns

# Recommended: context manager
with ns.open("mydb") as db:
    ...

# Manual open/close
db = ns.Database("mydb")
db.close()
```

---

### Variables

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

Dtype may be a Numstore string (`"f32"`, `"f64"`, `"i32"`, …) or any NumPy dtype.

---

### Reading and Writing

```python
with ns.open("mydb") as db:
    v = db["temperature"]

    val  = v[0]       # read one element
    vals = v[0:10]    # read a slice

    v[0]    = np.float32(99.0)                        # overwrite one element
    v[0:5]  = np.array([1, 2, 3, 4, 5], dtype="f32") # overwrite a slice
```

---

### Appending and Inserting

```python
with ns.open("mydb") as db:
    v = db["temperature"]

    idx = v.append(np.float32(22.5))  # append to end; returns new index
    v.insert(0, np.float32(0.0))      # insert at position, shifts right
```

---

### Removing Elements

```python
with ns.open("mydb") as db:
    v = db["temperature"]

    val  = v.remove(0)    # remove one element and return it
    vals = v.remove(0:5)  # remove a slice and return it
    del v[0]              # remove without returning
```

---

### Transactions

`txn["name"]` works just like `db["name"]` - it returns a Variable whose reads and writes are all part of that transaction. The context manager commits on clean exit and rolls back on any exception.

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

### Quick Reference

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
| Commit / Rollback | - | `txn.commit()` / `txn.rollback()` |

---

---

## Smart Files

Smart Files is the storage layer underlying Numstore. Files have had the same definition for 50 years: an array of bytes that grows, shrinks, and seeks. Smart Files extends that model with crash safety, inner mutations, and more.

### Motivation

Standard files have two fundamental problems:

1. **No crash safety.** `fwrite` does not guarantee bytes hit disk. A crash mid-write leaves the file in an unknown state with no path to recovery.
2. **No inner mutations.** There is no standard way to insert or remove bytes in the middle of a file without rewriting everything that follows.

### What Smart Files Adds

1. **Transactions** - Modifications go through a write-ahead log. Every write commits fully or rolls back cleanly; a crash mid-write leaves nothing corrupt.
2. **Inner mutations** - Insert or remove bytes anywhere in the stream in O(log N) time.
3. **Stride access** - Read, write, and remove at regular intervals without manual offset arithmetic.
4. **Multiple named streams** - Store more than one named byte stream per file - no separate file handles, no embedded database.

Written in C with no dependencies. Developed on POSIX; built (but not yet optimized) for Windows.

---

### Quick Start

```bash
git clone https://github.com/lincketheo/smartfiles.git
cd smartfiles
git submodule update --init
cmake --preset release
cmake --build --preset release
./build/release/samples/sample1_simple   # run a sample
sudo cmake --install ./build/release
```

---

### Sample Application

```c
#include "smfile.h"
#include <stddef.h>
#include <stdint.h>

void print_array(const char* prefix, uint32_t* arr, int len)
{
  printf("%s\n", prefix);
  printf("     ");
  for (int i = 0; i < len; ++i) {
    printf("%d ", arr[i]);
  }
  printf("\n");
}

int main()
{
  // NULL second parameter selects the default stream.
  // Pass any const char* to name it.
  smfile_t* f = smfile_open("example");

  uint32_t data[200000];
  for (int i = 0; i < 200000; ++i) {
    data[i] = i;
  }

  // Write the array at byte offset 0
  smfile_pinsert(f, NULL, data, 0, sizeof(data));

  // Insert another chunk at byte offset 40 (element index 10)
  smfile_pinsert(f, NULL, data, 10 * sizeof(uint32_t), 200000);

  // Read every 2nd uint32_t starting at offset 0
  uint32_t read_data[200];
  smfile_pread(f, NULL, read_data, sizeof(uint32_t), 0, 2, 200);
  print_array("Expect: [0, 2, 4, 6, ...]", read_data, 20);

  // Remove every 3rd element; removed values are copied into read_data
  smfile_premove(f, NULL, read_data, sizeof(uint32_t), 0, 3, 200);
  print_array("Expect: [0, 3, 6, 9, ...]", read_data, 20);

  // Same strided read - stream has changed shape after removal
  smfile_pread(f, NULL, read_data, sizeof(uint32_t), 0, 2, 200);
  print_array("Expect: [1, 4, 7, 0, ...]", read_data, 20);

  // Overwrite every 2nd element with values from data
  smfile_pwrite(f, NULL, data, sizeof(uint32_t), 0, 2, 200);

  // Final read - even-indexed slots now hold a clean sequence
  smfile_pread(f, NULL, read_data, sizeof(uint32_t), 0, 2, 200);
  print_array("Expect: [0, 1, 2, 3, ...]", read_data, 20);

  smfile_close(f);
  return 0;
}
```

**Compile and run:**

```bash
gcc main.c -o main -lsmartfiles -L/usr/local/lib
./main
```

**Expected output:**

```
Expect: [0, 2, 4, 6, ...]
     0 2 4 6 8 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28
Expect: [0, 3, 6, 9, ...]
     0 3 6 9 2 5 8 11 14 17 20 23 26 29 32 35 38 41 44 47
Expect: [1, 4, 7, 0, ...]
     1 4 7 0 3 6 9 12 15 18 21 24 27 30 33 36 39 42 45 48
Expect: [0, 1, 2, 3, ...]
     0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
```

---

### Step-by-Step Walkthrough

**Step 1 - Insert at position 0.**
Write the full 200,000-element array at byte offset 0. The stream now holds `[0, 1, 2, 3, ...]`.

**Step 2 - Insert in the middle.**
Insert another 200,000-byte chunk at byte offset 40 (element index 10). Everything from index 10 onwards shifts right, producing `[0, 1, …, 9, 0, 1, 2, …, 9, 10, 11, …]` at the join.

**Step 3 - Strided read (stride = 2).**
`smfile_pread` reads every 2nd `uint32_t` from offset 0, returning `[0, 2, 4, 6, 8, 0, 2, …]`. The `0` at index 5 marks where the inserted block begins.

**Step 4 - Strided remove (stride = 3).**
`smfile_premove` pulls every 3rd `uint32_t` and copies the removed values into `read_data`. Gaps close; remaining elements shift down.

**Step 5 - Strided read again (stride = 2).**
Same read as Step 3, but the stream is shorter. After removal the sequence starts `[1, 4, 7, 0, …]`.

**Step 6 - Strided write (stride = 2).**
`smfile_pwrite` overwrites every 2nd element with values from `data` (0, 1, 2, 3, …).

**Step 7 - Final read.**
The last strided read returns `[0, 1, 2, 3, …]` - even-indexed slots hold the clean sequence written in Step 6.

---

## Project Structure

Public headers live in `include/`. Source code lives in `lib/`. The `thirdparty/c_specx` directory holds reusable C utilities shared across projects - all original code, no external dependencies.

`lib/` is organized by subsystem:

| Directory | Responsibility |
|---|---|
| `algorithms` | Rope algorithms and database traversal |
| `aries` | Rollback and crash recovery logic |
| `dpgt` | Dirty page table |
| `lockt` | Lock table |
| `os_pager` | Single-file pager for reading pages from disk |
| `pager` | Buffer pool initialization, page reads, and WAL entry writes |
| `pages` | Page type definitions |
| `testing` | Test-specific utilities |
| `txns` | Transaction table and transaction logic |
| `wal` | Write-ahead log |

---

## How I Use AI

I use AI the way I use a language server: as a tool, not a co-author.

**Things I ask AI to do:**
- Add edge-case test scenarios to existing unit tests (reviewed before committing)
- Review an algorithm I've written and flag anything that looks wrong
- Write formatting scripts, CI/CD glue, and other boilerplate I could write myself but would rather not

**Things I don't ask AI to do:**
- Implement features
- Delete or replace code I've written
- Read a paper and implement the algorithm

In practice, AI is useful for ideation, code review, and generating mundane code I'll immediately refactor. Every algorithm in this codebase was written by me.

The `CLAUDE.md` file ensures that if AI-assisted code ever does land here, it follows consistent standards. It was inspired by [Karpathy Inspired Claude Code](https://github.com/forrestchang/andrej-karpathy-skills).

If you're skeptical that a database engine can be written without leaning on AI - fair. Read through the code and make up your own mind.

---

## Contributing

File a ticket on GitHub for bugs, feature requests, or questions. Pull requests are welcome - see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. Windows CI/CD support is an open and approachable first contribution.

---

## License

Apache 2.0. See [LICENSE](LICENSE).
