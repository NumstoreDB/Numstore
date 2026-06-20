<p align="center">
  <img src="docs/assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# Numstore

[![codecov](https://codecov.io/gh/NumstoreDB/Numstore/graph/badge.svg?token=GMNPHQID77)](https://codecov.io/gh/NumstoreDB/Numstore)
![GitHub Actions](https://github.com/NumstoreDB/Numstore/actions/workflows/unit_tests.yml/badge.svg)

**A database for arrays**

Numstore is a single file embedded ACID database built for arrays written entirely 
in C with no dependencies. Currently it has a nice easy python interface 
to use - but stay tuned for more features as it grows. 

---

1.0 Quick Start
===============

1.1 System requirements
-----------------------

* C compiler supporting the C11 standard
* CMake
* Python 3

1.2 Linux, Mac, Windows
-----------------------

Numstore is built with CMake.
Default build config is `Debug`:

    cd <path>/numstore
    mkdir build 
    cd build 
    cmake ..
    cmake --build .


1.3 Build in Release Mode
-------------------------

Release mode has no tests, and no ASSERTS

    cd <path>/numstore
    mkdir build 
    cd build 
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .

1.4 Run
-------

Run a sample application inside `src/apps/samples` use these as pedagogical 
sample applications that show you how numstore works

2.0 Configure your build
========================

2.1 CMake Options
-----------------

* `ENABLE_PORTABLE`              - Build code without machine optimized instructions (to distribute and valgrind)
* `ENABLE_TESTS`                 - Include testing specific code in the numstore library
* `ENABLE_LOGGING`               - Include logging code in the numstore library 
* `ENABLE_ASAN`                  - Compile in an address sanitizer
* `ENABLE_COVERAGE`              - Compile in coverage flag (for gnu only)
* `BUILD_TOOLS`                  - Build a bunch of command line tools
* `BUILD_SAMPLES`                - Build the sample code

2.2 Configure Compile Time attributes 
-------------------------------------

Numstore has compile time configuration of various application specific attributes
such as page size, number of bits for a page number etc.

You can see them all in cmake/Config.cmake.

These are common configuration options:

* `-DNS_NS_PAGE_SIZE=4096`       - Page size.
* `-DNS_MEMORY_PAGE_LEN=4096`    - Number of pages to fit inside the pager buffer pool.
* `-DNS_WAL_BUFFER_CAP=1048576`  - The size of the internal in memory WAL.

These should rarely be changed:

* `-DTSIZE_BITS=32`              - Number of bits needed to represent the size of a data type.
* `-DPSIZE_BITS=32`              - Number of bits needed to represent the size of an index into a page.
* `-DBSIZE_BITS=64`              - Number of bits needed to represent the size of an index into an array.
* `-DPGNO_BITS=64`               - Number of bits needed to represent the size of a page number.
* `-DTXID_BITS=32`               - Number of bits needed to represent the size of a transaction id.
* `-DLSN_BITS=64`                - Number of bits needed to represent the size of a log sequence number.
* `-DPGH_BITS=8`                 - Number of bits needed to represent the size of a page header.
* `-DWLH_BITS=8`                 - Number of bits needed to represent the size of a log header.

Or you can choose from a preset:
* `--preset release`             - Optimized build with tools and samples.
* `--preset debug`               - Debug build with tests, ASan, and logging.
* `--preset package`             - Portable, stripped build for packaging.
* `--preset ci-tests`            - Debug + ASan + tests. Used by CI.
* `--preset ci-release-tests`    - Release + tests. Used by CI.
* `--preset ci-valgrind`         - Debug + tests, no ASan. Used by CI under Valgrind.
* `--preset ci-coverage`         - Debug + tests + gcov instrumentation. Used by CI.

3.0 Project Layout
==================

* `src\*.c`                      - All the library files - numstore is flat - you could just compile them all without CMake
* `src\testing\*.c`              - Code for libnumstore that's test specific - leave out by default
* `src\apps\samples`             - Pedagogical sample apps
* `src\apps\scripts`             - Scripts to run on the code base
* `src\apps\testing`             - Tests
* `src\apps\tools`               - Utility debug tools
* `src\templates`                - Files that get generated during configure time

4.0 AI Usage Policy
===================

I use AI the way I use a language server: as a tool, not a co-author. AI usage is fine
but not for heavy tasks.

Things I ask AI to do:

* Add edge-case test scenarios to existing unit tests (reviewed before committing).
* Review an algorithm I've written and flag anything that looks wrong.
* Write formatting scripts, CI/CD glue, and other boilerplate I could write myself
  but would rather not.

Things I don't ask AI to do:

* Implement features.
* Delete or replace code I've written.
* Read a paper and implement the algorithm.

In practice, AI is useful for ideation, code review, and generating mundane code I'll
immediately refactor. Every algorithm in this codebase was written by me.

5.0 Contributing
================

File a ticket on GitHub for bugs, feature requests, or questions.
Many tickets that are easy to contribute will be marked

6.0 License
============

Apache 2.0. See LICENSE.
