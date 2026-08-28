Numstore
========

**A database for arrays**

Numstore is a single file embedded ACID database built for arrays written
entirely in C with no dependencies.

Conceptually, it's an ACID file with 

[faster inner file mutations](https://theolincke.com/blog/13_inner_inserts)

Thinking about it like a file - I've created a "smart files" interface, where
it's just an ACID file

But the original reason I wrote it was to store numerical arrays (arrays of
bytes where every 4 bytes is an int or every 8 bytes is a u64...)

Therefore, there are two interfaces - smartfiles (a simple ACID transactional
file) and numstore (an embedded database for numerical arrays).

more info: [Documentation](docs/index.md)

Quick Start
===========

I want to use Numstore python
-----------------------------

Although Numstore is not strictly a python library, it is easiest to use in
it's python form. Try out any of the samples in binginds/python/samples:

        make python-package
        pip3 install build/python/target/*.whl --force-reinstall
        python3 bindings/python/samples/sample1_basic.py 

I want to use the numstore embedded C Library
---------------------------------------------

This is the more advanced case - but numstore is primarily a C - library
with `numstore.h` and `smartfiles.h` being the two main points of entry

* build everything (debug is default)

       make

* Populate some data

        make python-package
        pip3 install build/python/target/*.whl --force-reinstall
        python3 bindings/python/samples/sample1*

* run numstore (the cli / repl is a work in progress)

       ./build/debug/target/bin/numstore example.db
       > get prices;

* build a release version instead (no asserts, no logs, -O3)

       make TARGET=release
       ./build/release/target/bin/numstore example.db

* run the unit tests

       ./build/debug/target/bin/unit_tests

* build and run a sample program (using the numstore or smartfiles library)

       ls build/debug/target/bin | grep sample
       ./build/debug/target/bin/smfile_sample1_basic_crud

* clean up

       make clean

headers and libs land in build/<target>/include and build/<target>/lib if you
want to link against numstore/smartfiles/core yourself

Main Outputs
============

Numstore Executable
--------------------

    build/bin/numstore

A cli app used for examining a database (work in progress)


Numstore Library
-----------------

    build/lib/libnumstore.a
    build/include/numstore.h

An embedded database for numerical arrays see `src/numstore/numstore.h` or
`src/numstore/samples/*`


Smartfiles Library
--------------------

    build/lib/libnumstore.a
    build/include/smartfiles.h

An embedded database for files see `src/smartfiles/smartfiles.h` or
`src/smartfiles/samples/*`. Smartfiles is compiled into the same
`libnumstore.a` as the numstore library above; there is only one static
library.



AI Usage Policy
===============

I use AI the way I use a language server: as a tool, not a co-author. AI usage
is fine but not for heavy tasks.

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
============

File a ticket on GitHub for bugs, feature requests, or questions.
Many tickets that are easy to contribute will be marked

License
=======

Apache 2.0. See LICENSE.
