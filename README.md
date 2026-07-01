<p align="center">
  <img src="docs/assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

Numstore
========

[![codecov](https://codecov.io/gh/NumstoreDB/Numstore/graph/badge.svg?token=GMNPHQID77)](https://codecov.io/gh/NumstoreDB/Numstore)
![GitHub Actions](https://github.com/NumstoreDB/Numstore/actions/workflows/develop.yml/badge.svg)

**A database for arrays**

Numstore is a single file embedded ACID database built for arrays written entirely
in C with no dependencies.

Conceptually, it's an ACID file with [faster inner file mutations](https://theolincke.com/blog/13_inner_inserts)

Getting Started 
===============

To get started, choose your platform and run:

<details>
<summary><strong>Linux / MacOS</strong></summary>

    # Build Numstore:
    cd <numstore>/src
    gcc *.c -o numstore
    ./numstore

    # Hint - you can get rid of all that logging with:
    cd <numstore>/src
    gcc *.c -o numstore -DNLOG
    ./numstore

    # Build sample apps by adding NUMSTORE_LIB
    cd <numstore>/src
    gcc *.c samples/sample1_basic_crud.c -DNUMSTORE_LIB -o sample -I. -DNLOG
    ./sample

</details>

<details>
<summary><strong>Windows (MSVC)</strong></summary>

    REM Build Numstore:
    cd <numstore>\src
    cl *.c /Fe:numstore.exe
    numstore.exe

    REM Hint - you can get rid of all that logging with:
    cl *.c /Fe:numstore.exe /DNLOG
    numstore.exe

    REM Build sample apps by adding NUMSTORE_LIB
    cl *.c samples\sample1_basic_crud.c /DNUMSTORE_LIB /I. /Fe:sample.exe /DNLOG
    sample.exe

</details>

For more information, refer to the [Documentation](docs/index.md).

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
