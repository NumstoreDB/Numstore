Numstore
========

**An embedded ACID database for Numpy arrays.**

Features:
---------
- **Single File.** The whole database lives in a single file. There is no
  server - see `libs/numstore/include/numstore.h`
- **ACID.** Every insert, write and remove either happens completely or not at
  all, even if the process crashes partway through. Transactions are strictly
  serializable - one is open at a time - and crash recovery uses ARIES over a
  unique rope variant of the B+Tree to index into arrays. Concurrent readers and
  writers are in development the infrastructure (guarded under the 
  `EXPERIMENTAL_HIERARCHIAL_TXNS` feature flag).
- **Arrays.** Numstore stores images, sensor streams, tick data and any fixed
  layout of numbers you can describe as a type.
- **No dependencies.** Written in C11 with first class Python bindings that
  return numpy arrays.

Pynumstore is the easiest way to get started:

```sh
pip install pynumstore
```

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

The wheel brings numpy and nothing else. To build the bindings from source
instead - which is what you want if you're working on Numstore itself - you'll
need Python 3.9+, a C11 compiler and CMake 3.20+:

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
python3 -m venv .venv && . .venv/bin/activate # Optional
pip install .
python samples/sample1_basic.py
```

`pip install .` compiles the C library and the bindings in one step. The sample
creates a database file in your current directory.

Next steps:

* [**Documentation**](docs/index.md) - the guided tour: the query language, the
  type system, transactions and durability, all in Python.
* [**Python API Reference**](docs/python_api.md) - every class and method in
  `pynumstore`.
* [**C API Reference**](docs/c_api.md) - `numstore.h`, linking, and a complete
  program per operation. Also see the [C Quick Start](#c-quick-start) below.

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

| Query                 | What it does                                      |
| --------------------- | ------------------------------------------------- |
| `create prices f64`   | create a variable                                 |
| `delete prices`       | delete a variable                                 |
| `insert prices 0 3`   | insert 3 elements at position 0                   |
| `write prices[0:3]`   | overwrite existing elements                       |
| `read prices[0:10:2]` | read elements, with numpy-style `start:stop:step` |
| `remove prices[0:2]`  | remove elements and return them                   |
| `get prices`          | look up a variable's type and length              |

### Types

**Primitives.** The number is the size in bits.

| Family               | Types             | Size       |
| -------------------- | ----------------- | ---------- |
| unsigned int         | `u8` to `u64`     | 1-8 bytes  |
| signed int           | `i8` to `i64`     | 1-8 bytes  |
| float                | `f16` to `f128`   | 2-16 bytes |
| complex float        | `cf32` to `cf256` | 4-32 bytes |
| complex signed int   | `ci16` to `ci128` | 2-16 bytes |
| complex unsigned int | `cu16` to `cu128` | 2-16 bytes |

**Composites.** Build bigger types out of smaller ones:

* **Strict array:** a fixed size, multi-dimensional array. `[3][256][256] f32`
  is three 256x256 grids of floats, like an RGB image.
* **Struct:** fields stored one after another, with no padding. The size is the
  sum of the fields. `struct { a u32, b f16 }` is 4 + 2 = 6 bytes.
* **Union:** fields that overlap, so it holds one of them at a time. The size
  is that of the largest field. `union { a u32, b f16 }` is 4 bytes.

They nest freely: `struct { a u32, b struct { c f16, d [10]f32 } }` is 4 + 2 +
40 = 46 bytes.

The full rules for sizing and nesting types are in
[the type system](docs/index.md#the-numstore-type-system).

C Quick Start
-------------

From the repository root:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
./build/release/bin/ns_sample1_basic_crud
```

What you get:

* `libs/numstore/include/numstore.h`: the only header you need
* `build/release/lib/libns_*.a`: the library, as one static archive per layer
* `build/release/bin/`: one binary per C sample in `samples/`, plus `ns_simtest`

To compile a program of your own against it:

```sh
L=build/release/lib
cc -std=c11 -I libs/numstore/include myprog.c \
    $L/libns_numstore.a $L/libns_nsdb.a $L/libns_algorithms.a \
    $L/libns_nscore.a $L/libns_disk_wal.a $L/libns_disk_pager.a \
    $L/libns_types.a $L/libns_page.a $L/libns_common.a -o myprog
```

The archives are listed so that each one comes before the ones it depends on,
which is the order a GNU linker needs. On Linux add `-pthread -lm`. For a debug
build, use `-B build/debug -DCMAKE_BUILD_TYPE=Debug` instead.

The [C API Reference](docs/c_api.md) has the rest: linking from CMake, and a
complete program for queries, transactions, crash recovery, packed structs,
prepared plans and smart files.

Developing
----------

The C code is a stack of static libraries under `libs/`, one directory each. A
library's headers sit next to its sources, and its directory is its include
directory, so `#include "ns_page.h"` names a file in `libs/page/` and
`#include "pager/ns_pager.h"` one in `libs/nscore/`. Each library
links to the ones it includes from, and nothing links upward:

| Directory          | Library         | What it holds                                                           | Links to                                   |
| ------------------ | --------------- | ----------------------------------------------------------------------- | ------------------------------------------ |
| `libs/common/`     | `ns_common`     | data structures, errors, logging, the test framework, `os/`             | -                                          |
| `libs/page/`       | `ns_page`       | the page formats                                                        | `common`                                   |
| `libs/nslang/`     | `ns_types`      | the query language: lexer, parsers, types, variables                    | `common`, `page`                           |
| `libs/disk_pager/` | `ns_disk_pager` | page sized reads and writes on the database file                        | `common`                                   |
| `libs/disk_wal/`   | `ns_disk_wal`   | byte streams on the WAL file                                            | `common`                                   |
| `libs/nscore/`     | `ns_nscore`     | `txn_table/`, `lock_table/`, `dpg_table/`, `wal/`, `pager/`             | `common`, `page`, `disk_pager`, `disk_wal` |
| `libs/algorithms/` | `ns_algorithms` | what a query does to pages: `var/`, `rope/`, `numstore/`, `smartfiles/` | `page`, `nslang`, `nscore`                 |
| `libs/nsdb/`       | `ns_nsdb`       | the database handle: open, close, transactions, plans                   | `page`, `nslang`, `nscore`, `algorithms`   |
| `libs/numstore/`   | `ns_numstore`   | the public `ns_*` API, and the tests that need a database               | `nsdb` and what it needs                   |

Only `ns_numstore` has a public header, `libs/numstore/include/numstore.h`.
Everything else is private to the build. `libs/numstore-rs/` and
`libs/pynumstore/` are the Rust and Python bindings.

The root `CMakeLists.txt` builds everything, and `make` wraps the cmake
invocations so the flags live in one place:

| Target                 | What it does                                                       |
| ---------------------- | ------------------------------------------------------------------ |
| `make debug`           | debug build into `build/debug` (the default target)                |
| `make release`         | release build into `build/release`                                 |
| `make release-package` | release build, then `cmake --install` into `build/release/package` |
| `make test`            | `test-unit`, then `test-simtest`                                   |
| `make test-unit`       | debug build, then `unit_tests` with a random seed                  |
| `make test-simtest`    | debug build, then `ns_simtest` for 50 seconds                      |
| `make test-python`     | pip install the package, then run `pytest`                         |
| `make wheel`           | build a wheel into `dist/` (`make sdist` for a tarball)            |
| `make coverage`        | gcovr HTML report into `build/coverage/html`                       |
| `make valgrind`        | `unit_tests` and `ns_simtest` under valgrind                       |
| `make leaks`           | the same two under macOS `leaks`                                   |
| `make format`          | clang-format, gersemi and ruff over the tree                       |
| `make tidy`            | clang-tidy over the C sources                                      |
| `make clean`           | remove build artifacts                                             |

Tool settings live in `.config/` (`clang-format`, `clang-tidy`, `gcovr.cfg`,
`valgrind`, `leaks.env`).
`coverage` needs gcovr, `format` needs clang-format, gersemi and ruff, and
`valgrind` and `leaks` need the tool they're named after. CI runs `make
test-unit` and `make release-package` on every push.

`unit_tests` takes a seed and an optional name filter, so
`./build/debug/bin/unit_tests 123142 wal` runs only the tests whose name
contains `wal`. It exists in debug builds only: its `main` is generated at
configure time by `apps/gen_tests.py`, which scrapes every `TEST(...)` in
`libs/`.

`ns_simtest` drives a database with random operations, checks it against a
reference model and crashes it at random:
`./build/debug/bin/ns_simtest --duration 5`.

AI Usage Policy
---------------

Moderate controlled AI usage is encouraged in Numstore but due to the highly
fragile nature of databases, heavy AI auditing and human driving is required.

Numstore's philosophy is: If you're going to use it, use it right. That's why
I've defined a series of skills in `.claude/skills`. When agents do work with
the code (and they will), they better do it following the evolving guidance
defined at the outset.

Every single line of code that goes into Numstore, whether written by a
human or AI, should ideally get both human and AI eyes on it. AI is a
great code reviewer to find "fat fingered" mistakes and point out design
problems, but a _concious_ and critical thinking human should be driving
all decisions. There will never be a line of code in Numstore that hasn't
been re written by a human.

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
