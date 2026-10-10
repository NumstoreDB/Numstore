Numstore
========

**An embedded ACID database for arrays.**

Features:
---------
- **Single File.** The whole database lives in a single file. There is no
  server. `nsserver` is a work in progress network front end to numstore but
  numstore will always expose a simple embedded front end interface. See
  `numstore/include/*` 
- **ACID.** Every insert, write and remove either happens completely or not at
  all, even if the process crashes partway through. It uses ARIES for crash
  recovery and a unique rope variant of the B+Tree to index into arrays.
- **Arrays.** Numstore stores images, sensor streams, tick data and any fixed
  layout of numbers you can describe as a type.
- **No dependencies.** Plain C11, with first class Python bindings that return
  numpy arrays.

Pynumstore is the easiest way to get started:

```python
import numpy as np
import pynumstore as ns

with ns.Database("prices.db") as db:
    db.execute("create prices f64")
    db.write("insert prices 0 3", np.array([1.5, 2.25, 3.75]))

    with db.begin() as tx:              # commits on success, rolls back on error
        tx.write("insert prices 3 2", [4.0, 5.0])

    print(db.read("read prices[0:]"))   # [1.5  2.25 3.75 4.   5.  ]
    print(db.read("read prices[0::2]")) # [1.5  3.75 5.  ]
```

You'll need Python 3.9+, a C11 compiler and CMake 3.20+.

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
python3 -m venv .venv && . .venv/bin/activate # Optional
pip install ./bindings/python
python bindings/python/samples/sample1_basic.py
```

`pip install` compiles the C library and the bindings in one step. The sample
creates a database file in your current directory.

Prefer C? Jump to the [C Quick Start](#c-quick-start).

Why Numstore?
-------------

Relational databases store tables: rows of named, typed columns, like a `users`
table with a name and a date of birth. They are a poor fit for data where one
value is a large block of numbers:

* a 3x256x256 RGB image
* a stream of thousands of floating point stock prices

That data usually ends up in flat binary files or formats like HDF5, which are
fast but give you no transactions: a crash halfway through a write leaves the
file half written. Numstore stores the same data with database guarantees.

Core ideas
----------

A database holds **variables**. Each variable has:

* a **name**, like `prices`
* a **type**, the byte layout of one element, like `f64` 
* **data**, an array of elements of that type

You work with variables through short queries:

| Query                    | What it does                                     |
| ------------------------ | ------------------------------------------------ |
| `create prices f64`      | create a variable                                |
| `delete prices`          | delete a variable                                |
| `insert prices 0 3`      | insert 3 elements at position 0                  |
| `write prices[0:3]`      | overwrite existing elements                      |
| `read prices[0:10:2]`    | read elements, with numpy-style `start:stop:step`|
| `remove prices[0:2]`     | remove elements and return them                  |
| `get prices`             | look up a variable's type and length             |

### Types

**Primitives.** The number is the size in bits.

| Family                 | Types             | Size         |
| ---------------------- | ----------------- | -------------|
| unsigned int           | `u8` to `u64`     | 1-8 bytes    |
| signed int             | `i8` to `i64`     | 1-8 bytes    |
| float                  | `f16` to `f128`   | 2-16 bytes   |
| complex float          | `cf32` to `cf256` | 4-32 bytes   |
| complex signed int     | `ci16` to `ci128` | 2-16 bytes   |
| complex unsigned int   | `cu16` to `cu128` | 2-16 bytes   |

**Composites.** Build bigger types out of smaller ones:

* **Strict array:** a fixed size, multi-dimensional array. `[3][256][256] f32`
  is three 256x256 grids of floats, like an RGB image.
* **Struct:** fields stored one after another, with no padding. The size is the
  sum of the fields. `struct { a u32, b f16 }` is 4 + 2 = 6 bytes.
* **Union:** fields that overlap, so it holds one of them at a time. The size
  is that of the largest field. `union { a u32, b f16 }` is 4 bytes.

They nest freely: `struct { a u32, b struct { c f16, d [10]f32 } }` is 4 + 2 +
40 = 46 bytes.

More: [Documentation](docs/index.md) ·
[Python API](bindings/python/README.md)

Smartfiles: Numstore as an ACID file
------------------------------------
 
If you don't need variables or types, the core of Numstore also works as a
single file with transactions, called a smartfile.
 
Traditionally, a file is an array of bytes, and you read and write its interior
with two well known functions:
 
```c
// Open and close a file
int open(name)
close(fd)
 
read(fd, dest, count)   // Read from a file - maybe fail - into dest
write(fd, src, count)   // Overwrite bytes within a file
```
 
Both return the number of bytes transferred, and both can stop partway, reading
or writing only some of the bytes you asked for.
 
From the write man page:
> Note that a successful write() may transfer fewer than count bytes.  Such
> partial writes can occur for various reasons; for example, because there was
> insufficient space on the disk device to write all of the requested bytes, or
> because a blocked write() to a socket, pipe, or similar was interrupted by a
> signal handler after it had transferred some, but before it had transferred
> all of the requested bytes.  In the event of a partial write, the caller can
> make another write() call to transfer the remaining bytes.  The subsequent
> call will either transfer further bytes or may result in an error (e.g., if
> the disk is now full).
 
From the read man page:
> On success, the number of bytes read is returned (zero indicates end of
> file), and the file position is advanced by this number. It is not an error
> if this number is smaller than the number of bytes requested; this may happen
> for example because fewer bytes are actually available right now (maybe
> because we were close to end-of-file, or because we are reading from a pipe,
> or from a terminal), or because read() was interrupted by a signal.  See also
> NOTES.
 
A smartfile has no half writes or half reads: every operation either happens
completely or not at all. You can also insert into or remove from the middle of
the file, not just append:
 
```c
// Open and close a smartfile
nsdb_t *ns_smfile_open (path);
int ns_close (ns);
 
// Group operations into a transaction
txn_t *ns_begin (ns);
int ns_commit (ns, txn);
int ns_rollback (ns, txn);
 
// Size of the file
sb_size ns_smfile_size (smf, tx);
 
// Insert data into the middle of the file (the file grows)
sb_size ns_smfile_insert (smf, tx, src, bofst, slen);
 
// Overwrite data in place (the file length stays the same)
sb_size ns_smfile_write (smf, tx, src, size, bofst, stride, nelem);
 
// Read elements of a given size and stride
sb_size ns_smfile_read (smf, tx, dest, size, bofst, stride, nelem);
 
// Remove data from the file, copying what was removed into dest
sb_size ns_smfile_remove (smf, tx, dest, size, bofst, stride, nelem);
```

C Quick Start
-------------

From the repository root:

```sh
cmake -S numstore -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
./build/release/bin/ns_sample1_basic_crud
```

What you get:

* `numstore/include/numstore.h`: the only header you need
* `build/release/lib/libnumstore.a`: the whole library in one static archive
* `build/release/bin/*_sample*`: runnable samples, with source in
  `numstore/apps/samples/`

For a debug build, use `-B build/debug -DCMAKE_BUILD_TYPE=Debug` instead.

Developing the Python bindings
------------------------------

The bindings are a thin layer over the C library. All the C glue is in
`bindings/python/src/c/ns_pynumstore.c`.

```sh
pip install -e './bindings/python[test]'   # editable install, with pytest
python -m pytest bindings/python/tests
```

AI Usage Policy
---------------

I use AI the way I use a language server: as a tool, not a co-author. AI usage
is fine, but not for heavy lifting.

Things I ask AI to do:

- Add edge-case test scenarios to existing unit tests (reviewed before
  committing).
- Review an algorithm I've written and flag anything that looks wrong.
- Write formatting scripts, CI/CD glue, and other boilerplate I could write
  myself but would rather not.

Things I don't ask AI to do:

- Implement features.
- Delete or replace code I've written.
- Read a paper and implement the algorithm.

In practice, AI is useful for ideation, code review, and generating mundane
code I'll immediately refactor. Every algorithm in this codebase was written by
me.

Contributing
------------

Contributions are welcome, but I don't have the bandwidth to make the process
easy yet (something I'm working on). File a GitHub issue for bugs, feature
requests or questions. Issues that are easy to pick up are labeled `good first
issue`.

License
-------

Apache 2.0. See [LICENSE](LICENSE).
