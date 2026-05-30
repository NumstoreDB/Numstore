<p align="center">
  <img src="assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# Numstore

**A database for bytes**

---

1.0 Synopsis
============

1.1 System requirements
-----------------------

* C compiler supporting the C11 standard
* CMake
* Python 3

1.2 Linux, Mac, Windows
-----------------------

    cd <path>/numstore
    cmake --preset debug


2.0 Build!
==========

Numstore is built with CMake:

    cmake --build --preset debug

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

* `--preset debug-no-asan`       - Same as debug but without address sanitizer
                                   (useful for running against valgrind).

* `--preset debug-ntests`        - Debug build without any tests compiled in.

* `--preset debug-ntests-no-asan`- Same as debug-ntests without address sanitizer.

* `--preset release`             - Release build.
                                   Logging disabled.
                                   No asserts.
                                   No tests.
                                   Optimized for your machine.
                                   No address sanitizer.

* `--preset release-tests`       - Same as release but with tests compiled in
                                   (to ensure unit tests pass with 
                                   or without asserts).

* `--preset package-release`     - Portable binary with stripped symbols.

There's an upfront Makefile that has common targets that I run a lot 
but in general this is just a tool to help me reduce keystrokes 
new developers should use cmake as much as possible - while using 
make if there are any common workflows.

3.0 Run!
========

3.1 Pynumstore
--------------

pynumstore is the python wrapper around numstore that interacts heavily with numpy arrays.
pynumstore is not published to PyPI (yet). The package is self-contained under
libs/pynumstore and uses scikit-build-core - the C extension is compiled automatically
as part of the install step.

Note: Pynumstore is experimental - expect bugs around wrapping python and c -
the core logic is found in numstore / nscore.

3.1.0 Quick Start
-----------------

3.1.0.0. Install

    pip install -e libs/pynumstore

This compiles the C extension and installs the package in editable mode. CMake and a C
compiler must be available on your PATH.

3.1.0.1. Run a sample

    python samples/pynumstore/basic_operations.py

All PyNumstore samples live in samples/pynumstore/.

Note: pynumstore is in an unstable state - more docs to come while work is done on
pynumstore - for now run samples in samples/pynumstore.


4.0 Configure your build
========================

Numstore has compile time configuration of various application specific attributes
such as page size, number of bits for a page number etc.

You can see them all in cmake/Config.cmake.

These are common configuration options:

* `-DNS_PAGE_SIZE=4096`          - Page size.

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


5.0 Project Layout
==================

* `apps`                         - All applications in this directory are built
                                   and linked with all libraries built in this
                                   project, making it easy to create on the fly
                                   applications for simple utilities on the
                                   database.

* `samples`                      - A list of samples for easy developer
                                   onboarding.

* `libs`                         - Each directory inside this is a library.

* `scripts`                      - Simple scripts for auto code generation or
                                   formatting or ci / cd. Mostly python.

* `cmake`                        - CMake utilities and includes.

* `docs`                         - Documentation for numstore.


6.0 Summary of all Libraries
============================

The application contains various targets:

* `libnumstore`                  - The numstore library is a wrapper over nscore
                                   that adds convenience one time operations
                                   over arrays.

* `libnscore`                    - This is the core of numstore - all headers
                                   are exposed - all heavy weight algorithms
                                   live here.

* `lib_pynumstore`               - The python wrapper over libnumstore.

* `lib_smartfiles`               - The smart files library is a simple api over
                                   a file like object with inner mutations,
                                   kind of like numstore but every type is a
                                   uint8_t.

* `libc_specx`                   - Core common logic.

* `libnstesting`                 - Testing scaffolding.

The submodule repository qtrepotools contains useful 
scripts for developers and release engineers. 
Consider adding qtrepotools/bin to your PATH 
environment variable to access them.


7.0 Documentation
=================

After configuring and compiling, building the documentation is possible by running:

    cmake --build . --target docs

After having built the documentation, install it with:

    cmake --build . --target install_docs

The documentation is installed in the path specified with the configure argument
-docdir.

Note: Building the documentation is only tested on desktop platforms.


8.0 AI Usage Policy
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


9.0 Contributing
================

File a ticket on GitHub for bugs, feature requests, or questions.
Pull requests are welcome - see CONTRIBUTING.md for guidelines.
Windows CI/CD support is an open and approachable first contribution.


10.0 License
============

Apache 2.0. See LICENSE.
