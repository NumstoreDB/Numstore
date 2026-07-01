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

    cd <numstore>/src
    gcc *.c apps/samples/sample1_basic_crud.c -I. -o sample
    ./sample

    # Hint - you can get rid of all that logging with:

    gcc *.c apps/samples/sample1_basic_crud.c -I. -o sample -DNLOG

    # Hint - Try increasing the page size:

    gcc *.c apps/samples/sample1_basic_crud.c -I. -o sample -DNLOG -DPAGE_SIZE=4096

</details>

<details>
<summary><strong>Windows (MSVC)</strong></summary>

    cd <numstore>\src
    cl *.c apps\samples\sample1_basic_crud.c /I. /Fe:sample.exe
    sample.exe

    REM Hint - you can get rid of all that logging with:

    cl *.c apps\samples\sample1_basic_crud.c /I. /Fe:sample.exe /DNLOG

    REM Hint - Try increasing the page size:

    cl *.c apps\samples\sample1_basic_crud.c /I. /Fe:sample.exe /DNLOG /DPAGE_SIZE=4096

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
