<p align="center">
  <img src="assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# Numstore

[![codecov](https://codecov.io/gh/lincketheo/Numstore/graph/badge.svg?token=GMNPHQID77)](https://codecov.io/gh/lincketheo/Numstore)
![GitHub Actions](https://github.com/lincketheo/Numstore/actions/workflows/unit_tests.yml/badge.svg)

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

    cd <path>/numstore
    cmake --preset debug


1.3 Build!
----------

Numstore is built with CMake:

    cmake --build --preset debug

1.4 Run! 
--------
Run a sample application inside `apps/samples` use these as pedagogical 
sample applications that show you how numstore works

2.0 Configure your build
========================

2.1 Choose a different build prefix 
-----------------------------------

You can see the options of various presets in CMakePresets.json
as well as build your own custom presets using flags found in
./cmake/Options.cmake

Here are some other cmake presets:

* `--preset debug`               - Debug build.
                                   Logging enabled.
                                   Asserts enabled.
                                   Symbols included.
                                   Portable.
                                   Not stripped.
                                   Address sanitizer compiled in.

* `--preset debug-valgrind`      - Same as debug but without address sanitizer
                                   (useful for running against valgrind) and portable.

* `--preset debug-coverage`      - Debug with coverage enabled

* `--preset release`             - Release build.
                                   Logging disabled.
                                   No asserts.
                                   No tests.
                                   Optimized for your machine.
                                   No address sanitizer.

* `--preset release-tests-asan`  - Same as release but with tests compiled in
                                   (to ensure unit tests pass with 
                                   or without asserts).

* `--preset package-release`     - Portable binary with stripped symbols.

There's an upfront Makefile that has common targets that I run a lot 
but in general this is just a tool to help me reduce keystrokes 
new developers should use cmake as much as possible - while using 
make if there are any common workflows.

2.2 Configure Compile Time attributes 
-------------------------------------

Numstore has compile time configuration of various application specific attributes
such as page size, number of bits for a page number etc.

You can see them all in cmake/Config.cmake.

These are common configuration options:

* `-DNS_NS_PAGE_SIZE=4096`          - Page size.

* `-DNS_MEMORY_PAGE_LEN=4096`    - Number of pages to fit inside the pager
                                   buffer pool.

* `-DNS_WAL_BUFFER_CAP=1048576`  - The size of the internal in memory WAL.

These should rarely be changed:

* `-DTSIZE_BITS=32`              - Number of bits needed to represent the size
                                   of a data type.

* `-DPSIZE_BITS=32`              - Number of bits needed to represent the size
                                   of an index into a page.

* `-DBSIZE_BITS=64`              - Number of bits needed to represent the size
                                   of an index into an array.

* `-DPGNO_BITS=64`               - Number of bits needed to represent the size
                                   of a page number.

* `-DTXID_BITS=32`               - Number of bits needed to represent the size
                                   of a transaction id.

* `-DLSN_BITS=64`                - Number of bits needed to represent the size
                                   of a log sequence number.

* `-DPGH_BITS=8`                 - Number of bits needed to represent the size
                                   of a page header.

* `-DWLH_BITS=8`                 - Number of bits needed to represent the size
                                   of a log header.


3.0 Project Layout
==================

* `apps`                         - All applications in this directory are built
                                   and linked with all libraries built in this
                                   project, making it easy to create on the fly
                                   applications for simple utilities on the
                                   database.

* `src`                          - Source code and private headers

* `include`                      - Public headers

* `scripts`                      - Simple scripts for auto code generation or
                                   formatting or ci / cd. Mostly python.

* `cmake`                        - CMake utilities and includes.

* `docs`                         - Documentation for numstore.

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

The CLAUDE.md file ensures that if AI-assisted code ever does land here, it follows
consistent standards.


5.0 Contributing
================

File a ticket on GitHub for bugs, feature requests, or questions.
Pull requests are welcome - see CONTRIBUTING.md for guidelines.
Windows CI/CD support is an open and approachable first contribution.


6.0 License
============

Apache 2.0. See LICENSE.
