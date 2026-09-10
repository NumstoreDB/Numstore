Numstore
========

**A database for arrays**

Numstore is a single-file, embedded, ACID database built for arrays, written
entirely in C with no dependencies.

Conceptually, it's an ACID file with [faster inner-file
mutations](https://theolincke.com/blog/13_inner_inserts) than a typical
database.

Thinking about it as "just a file" led to a second interface: **smartfiles**, a
plain ACID transactional file with no array-specific semantics.

The original reason Numstore exists is to store numerical arrays - arrays of
bytes where every 4 bytes is an `int`, every 8 bytes is a `u64`, and so on.

So there are two interfaces:

- **smartfiles** - a simple ACID transactional file.
- **numstore** - an embedded database for numerical arrays.

More info: [Documentation](docs/index.md)

Quick Start
===========

I want to use Numstore from Python
-----------------------------------

Numstore isn't strictly a Python library, but it's easiest to try out in its
Python form. Run any of the samples in `bindings/python/samples`:

    pip3 install build
    make python-package
    pip3 install build/python/target/*.whl --force-reinstall
    python3 bindings/python/samples/sample1_basic.py

I want to use the Numstore embedded C library
-----------------------------------------------

This is the more advanced path. Numstore is primarily a C library, with
`numstore.h` and `smartfiles.h` as the two main entry points.

* Build everything (debug is the default target):

      make

* Populate some data (using the Python bindings, for convenience):

      make python-package
      pip3 install build/python/target/*.whl --force-reinstall
      python3 bindings/python/samples/sample1_basic.py

* Run the numstore CLI/REPL (work in progress):

      ./build/debug/*/bin/numstore example.db
      > get prices;

* Build a release version instead (no asserts, no logs, `-O3`):

      make TARGET=release
      ./build/release/*/bin/numstore example.db

* Run the unit tests:

      ./build/debug/*/bin/unit_tests SEED <filter>

* Build and run a sample program (using the numstore or smartfiles library):

      ls build/debug/*/bin | grep sample
      ./build/debug/*/bin/smfile_sample1_basic_crud

* Clean up:

      make clean

Headers and libraries land in `build/<target>/<artifact>/include` and
`build/<target>/<artifact>/lib` if you want to link against numstore,
smartfiles, or core yourself. See `lib/pkgconfig/numstore.pc` for a
`pkg-config`-friendly way to pick those paths up automatically.

Main Outputs
============

Numstore Executable
--------------------

    build/<target>/<artifact>/bin/numstore

A CLI app for examining a database (work in progress).

Numstore Library
-----------------

    build/<target>/<artifact>/lib/libnumstore.a
    build/<target>/<artifact>/include/numstore/numstore.h

An embedded database for numerical arrays. See `src/numstore/numstore.h` or
`src/numstore/samples/*`.

Smartfiles Library
--------------------

    build/<target>/<artifact>/lib/libnumstore.a
    build/<target>/<artifact>/include/smartfiles/smartfiles.h

An embedded ACID file interface. See `src/smartfiles/smartfiles.h` or
`src/smartfiles/samples/*`. Smartfiles compiles into the same
`libnumstore.a` as the numstore library above - there is only one static
library, with two separate headers.

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

File a ticket on GitHub for bugs, feature requests, or questions. Tickets
that are easy to contribute to will be marked `good first issue`.

License
=======

Apache 2.0. See [LICENSE](LICENSE).
