Numstore
========

**A database for arrays**

Numstore is a single-file, embedded, ACID database built for arrays, written
entirely in C with no dependencies.

Numstore has first class python bindings:

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

To run this yourself, see the [Quick Start](#quick-start).

What is Numstore?
-----------------

There's an untapped type of data that isn't natively supported in most modern
reliable fault tolerant databases today: Array data. Traditional relational
databases store "tabular data". Each column has a "name" and "data type". A SQL
database stores e.g. a "User" table, which has a name and a date of birth.

An array is a type of data where there's a lot of information packed into one
type:

* A 3x256x256 RGB image 
* A stream of thousands of floating point stock ticker data 

Traditionally, this type of data tends to be stored in a flat binary file
format or specialized non database file formats like HDF5.

Numstore is a database for that type of data. In numstore, a single database has:
* Multiple _variables_ which each have a name, and type 
* _types_ represent the byte layout of the variable 
* _data_ is an array of bytes that represent the content of the variable

Types are byte layouts of a variable:

* A "primitive" is a scalar - signed or unsigned ints, floats and complex
  floats. The digit represents how many bits they take up:
    - `u8-u64` are 1-8 byte unsigned ints
    - `i8-i64` are 1-8 byte signed ints
    - `f16-f128` are 2-16 byte floats
    - `cf32-cf256` are 4-32 byte complex floats
    - `ci16-ci128` are 2-16 byte complex signed ints
    - `cu16-cu128` are 2-16 byte complex unsigned ints

* A "struct" is a stacked combination of two sub types. The size of a struct is
  the sum of all the sub types:
    - A struct: `struct { a u32, b f16 }` is a 6 byte type (`sizeof(u32) +
      sizeof(f16)`) which represents an unsigned 4 byte int and a 2 byte float
      stacked on top of each other.
    - A nested struct: `struct { a u32, b struct { c f16, d [10]f32 } }` is a
      46 byte type `10 * sizeof(f32) + sizeof(f16) + sizeof(u32)`

* A "union" is a type where all of its sub types overlap at index 0,
  representing an "either or" relationship:
    - A union: `union { a u32, b f16 }` is a 4 byte structure which either
      represents a u32 or an f16
    - A nested union: `union { a u32, b struct { c f16, d [10]f32 } }` is a 42
      byte `Max(10 * sizeof(f32) + sizeof(f16), sizeof(u32))`

* A "strict array" is a fixed size, multi dimensional array of a sub type:
    - A simple strict array `[3][256][256] f32` is a rank 3 array of floats
      with 256 columns, 256 rows and 3 "cubes"

More info: [Documentation](docs/index.md)

What are Smartfiles?
--------------------

Numstore was originally written to be an array database, but I found it useful
to think of it as a single ACID file. If your use case is simpler than a
database for arrays, numstore core also doubles as a simple ACID file with
first class interior mutations.

Traditionally, a "file" is an array of bytes. You read and write to the
interior using two well known functions:

```
// Open and close a file
int open(name)
close(fd)

read(fd, dest, count) // Read a file - maybe fail - into dest
write(fd, src, count) // Overwrite bytes within a file
```

Both *read* and *write* return the number of bytes read or written. Both of
them can fail half way or read / write only a subset of the bytes in the file.

From the write man pages:
> Note that a successful write() may transfer fewer than count bytes.  Such
> partial writes can occur for various reasons; for example, because there was
> insufficient space on the disk device to write all of the requested bytes, or
> because a blocked write() to a socket, pipe, or similar was interrupted by a
> signal handler after it had transferred some, but before it had transferred
> all of the requested bytes.  In the event of a partial write, the caller can
> make another write() call to transfer the remaining bytes.  The subsequent
> call will either transfer further bytes or may result in an error (e.g., if
> the disk is now full).

From the read man pages:
> On success, the number of bytes read is returned (zero indicates end of
> file), and the file position is advanced by this number. It is not an error
> if this number is smaller than the number of bytes requested; this may happen
> for example because fewer bytes are actually available right now (maybe
> because we were close to end-of-file, or because we are reading from a pipe,
> or from a terminal), or because read() was interrupted by a signal.  See also
> NOTES.

```
// Open and close a smart file
nsdb_t *ns_smfile_open (path);
int ns_close (ns);

// Begin or commit a transaction
txn_t *ns_begin (ns);
int ns_commit (ns, txn);
int ns_rollback (ns, txn);

// Return the size of the file
sb_size ns_smfile_size (smf, tx);

// Insert data into the interior of the file (increasing the file length)
sb_size ns_smfile_insert (smf, tx, src, bofst, slen);

// Overwrite data inside the file (keep the file length the same)
sb_size ns_smfile_write (smf, tx, src, size, bofst, stride, nelem);

// Read data from the file with a given element size and stride
sb_size ns_smfile_read (smf, tx, dest, size, bofst, stride, nelem);

// Remove data from the file - and write the results to dest
sb_size ns_smfile_remove (smf, tx, dest, size, bofst, stride, nelem);
```

Numstore writes are atomic, meaning they either happen or they don't. There's
no "half writes" or "half reads". Everything either happens or doesn't.

Quick Start
===========

You need a C11 compiler and CMake 3.20 or newer. Python needs 3.9 or newer.

<details open>
    <summary>Python Quick Start Guide</summary>

    git clone https://github.com/NumstoreDB/Numstore
    cd Numstore
    python3 -m venv .venv && . .venv/bin/activate
    pip install build
    make -C bindings/python install
    python bindings/python/samples/sample1_basic.py

`make -C bindings/python help` lists the other targets (`test`, `samples`,
`dev`, `sdist`, ...).

The Python library is a lightweight wrapper around the C library. All 
the bindings live in `bindings/python/src/c/ns_pynumstore.c`. See
[bindings/python](bindings/python/README.md) for the API.
</details>

<details>
    <summary>C Quick Start Guide</summary>
        
    mkdir -p build/release
    cd build/release
    cmake ../../numstore -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    ./bin/ns_sample1_basic_crud

The Numstore C library is intentionally simple. These are the most important 
outputs:
* `build/release/lib/libnumstore.a` - all numstore code in a single library
* `build/release/bin/*_sample*` - a bunch of samples, found in
  `numstore/apps/samples/`
* `numstore/include/numstore.h` - The only header file you need for
  numstore 

</details>

AI Usage Policy
===============

I use AI the way I use a language server: as a tool, not a co-author. AI
usage is fine, but not for heavy lifting.

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
code I'll immediately refactor. Every algorithm in this codebase was written
by me.

Contributing
============

Contributions are welcome, but I don't have the bandwidth to make the process 
easy (something I am trying my best to change). Feel free to 
File a ticket on GitHub for bugs, feature requests, or questions. Tickets
that are easy to contribute to will be marked `good first issue`.

License
=======

Apache 2.0. See [LICENSE](LICENSE).
