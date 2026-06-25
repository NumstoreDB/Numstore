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

Numstore has two paradigms so far:

1. Smart files:
   Smartfiles is conceptually a database for "file like data". That is, just an array of bytes.
   Smartfiles has far better performance for inner insertions and removals than regular files
   Smartfiles is ACID - your file never writes half of what you want it to write, even if you unplug your
   computer while it's doing work.

1. Numstore:
   Numstore is the same as smartfiles but it has a rich type system built on top to handle numerical data

To get started, choose your platform and run:

<details>
<summary><strong>Linux / MacOS</strong></summary>

```
cd <numstore>/src
gcc -c *.c
ar rcs libnumstore.a *.o
gcc apps/samples/smartfiles/sample1_basic_crud.c -o sample libnumstore.a -I.
./sample
```

</details>

<details>
<summary><strong>Windows (MSVC)</strong></summary>

```
cd <numstore>\src
cl /c *.c
lib /OUT:numstore.lib *.obj
cl apps\samples\smartfiles\sample1_basic_crud.c /Fe:sample.exe numstore.lib /I.
sample.exe
```

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
