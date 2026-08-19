Numstore
========

**A database for arrays**

Numstore is a single file embedded ACID database built for arrays written entirely
in C with no dependencies.

Conceptually, it's an ACID file with 
[faster inner file mutations](https://theolincke.com/blog/13_inner_inserts)

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

    build/lib/libsmartfiles.a
    build/include/smartfiles.h

An embedded database for files see `src/smartfiles/smartfiles.h` or
`src/smartfiles/samples/*`


Quick Start
===========

pick a platform, follow the steps

Linux / MacOS
-------------

1. build everything (debug is default)

       make

2. run numstore

       ./build/debug/bin/numstore foo.db

3. build a release version instead (no asserts, no logs, -O3)

       make TARGET=release
       ./build/release/bin/numstore foo.db

4. run the unit tests

       ./build/debug/bin/unit_tests

5. build and run a sample program (using the numstore or smartfiles library)

       ls build/debug/bin | grep sample
       ./build/debug/bin/smfile_sample1_basic_crud

6. clean up

       make clean

headers and libs land in build/<target>/include and build/<target>/lib if you
want to link against numstore/smartfiles/core yourself

more info: [Documentation](docs/index.md)

AI Usage Policy
===============

I use AI the way I use a language server: as a tool, not a co-author. AI usage is fine
but not for heavy tasks.

Things I ask AI to do:

- Add edge-case test scenarios to existing unit tests (reviewed before committing).
- Review an algorithm I've written and flag anything that looks wrong.
- Write formatting scripts, CI/CD glue, and other boilerplate I could write myself
  but would rather not.

Things I don't ask AI to do:

- Implement features.
- Delete or replace code I've written.
- Read a paper and implement the algorithm.

In practice, AI is useful for ideation, code review, and generating mundane code I'll
immediately refactor. Every algorithm in this codebase was written by me.

Contributing
============

File a ticket on GitHub for bugs, feature requests, or questions.
Many tickets that are easy to contribute will be marked

License
=======

Apache 2.0. See LICENSE.
