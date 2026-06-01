<p align="center">
  <img src="assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# Numstore

**A database for bytes**

---

1.0 Quick Start
===============

1.0 Install Numstore using pip
------------------------------
```
pip3 install numstore 
```

1.1 Run a Sample Application 
----------------------------

The following example shows the main operations you can do on a 
numstore database:

The available things you can do to a numstore database are:
*  **Create**       a new variable
*  **Delete**       a variable variable
*  **Get**          a variable 
*  **Insert**       data into a variable
*  **Read**         data from a variable
*  **Write**        data into a variable
*  **Remove**       data from a variable
*  **Begin**        a transation
*  **Commit**       an open transaction
*  **Rollback**     an open transaction
*  **Close**        and reopen a database

You can see more examples in samples/pynumstore

```
import numpy as np
import pynumstore as ns

# Basic Operations
with ns.open("mydb") as db:
    ######## Part 1 - A simple Create Insert Read Write Remove example
    y = db.get_or_create("y", dtype="f32")

    # Start with a fresh dataset if y already existed
    del y[0:]
    print("Initial State: ", y[0:])

    # Append twice
    y.append(np.array([1.0, 2.0, 3.0], dtype=np.float32))
    y.append(np.array([4.0, 5.0, 6.0], dtype=np.float32))

    # Retrieve the whole array
    print("Seed Data: ", y[0:])

    # Insert in the middle (inner mutations are first class operations)
    y.insert(2, np.array([4.0, 5.0, 6.0], dtype=np.float32))

    # Retrieve the whole array
    print("After inner insert: ", y[0:])

    # Overwrite data at the start
    y[1:4] = np.array([9.0, 9.0, 9.0], dtype=np.float32)
    print("After overwrite 1: ", y[0:])

    # Overwrite data at the start
    y[3:7] = np.array([1, 2, 10, 12], dtype=np.float32)
    print("After overwrite 2: ", y[0:]) 

    # Remove every even index
    del y[0::2]
    print("End state: ", y[0:])

    ######## Part 2 - Demonstrating some ACID properties of numstore

    # Without a transaction - mutations happen in "auto transaction mode"
    counts = db.get_or_create("counts", dtype="i32")
    del counts[0:]

    # You can wrap operations inside a transaction to be explicit
    with db.begin_txn() as txn:
        txn["counts"].append(np.array([1, 2, 3], dtype=np.int32))

    print("First state: ", counts[0:])

    # When an exception is thrown - numstore roll's back changes:
    try:
        with db.begin_txn() as txn:
            txn["counts"].append(np.array([99, 99, 99], dtype=np.int32))
            print("After modification (inside tx): ", txn["counts"][0:])
            raise RuntimeError("something went wrong")
    except RuntimeError:
        pass

    print("counts after rollback: ", db["counts"][0:])  
```

2.0 Building from Source
========================

2.1 System requirements
-----------------------

* C compiler supporting the C11 standard
* CMake
* Python 3

2.2 Linux, Mac, Windows
-----------------------

    cd <path>/numstore
    cmake --preset debug


2.3 Build!
----------

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

2.4 Pynumstore
--------------

2.4.1. Install

    pip install -e libs/pynumstore

This compiles the C extension and installs the package in editable mode. CMake and a C
compiler must be available on your PATH.

2.4.2. Run a sample

    python samples/pynumstore/basic_operations.py

All PyNumstore samples live in samples/pynumstore/.

Note: pynumstore is in an unstable state - more docs to come while work is done on
pynumstore - for now run samples in samples/pynumstore.


3.0 Configure your build
========================

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


4.0 Project Layout
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


5.0 Summary of all Libraries
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


6.0 Documentation
=================

After configuring and compiling, building the documentation is possible by running:

    cmake --build . --target docs

After having built the documentation, install it with:

    cmake --build . --target install_docs

The documentation is installed in the path specified with the configure argument
-docdir.

Note: Building the documentation is only tested on desktop platforms.


7.0 AI Usage Policy
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


8.0 Contributing
================

File a ticket on GitHub for bugs, feature requests, or questions.
Pull requests are welcome - see CONTRIBUTING.md for guidelines.
Windows CI/CD support is an open and approachable first contribution.


9.0 License
============

Apache 2.0. See LICENSE.
