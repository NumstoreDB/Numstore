# Changelog

## [v1.5.0] - 2026-10-13

CMake everywhere, one public `ns_*` API with compiled query plans, an
injectable OS layer that can be made to fail on purpose, and a deterministic
simulation test built on top of it.

## Changed
- The build is CMake now - `module.mk` files and the hand written make rules
  are gone, every source directory has a `CMakeLists.txt`, and the top level
  `Makefile` is a thin wrapper around configure/build for the `debug`,
  `release`, `coverage` and `release-package` trees
    - `make release-package` is a plain `cmake --install` into
      `build/release/package` (headers, static library, apps) via
      `GNUInstallDirs`
    - `unit_tests.c` is generated into the build tree by `gen_tests.py` at
      build time instead of being a checked in file, and only in Debug
    - Logging is compiled out by build config (`NLOG` on Release) rather than
      by a separate `NS_NLOG` option
    - `numstore/version.txt` feeds the CMake `project(VERSION)`, and
      `bindings/python/version.txt` feeds the wheel version through
      scikit-build-core, so the version lives in one place per artifact
- The library moved under `numstore/` - `numstore/src/core`,
  `numstore/src/nscore`, public header at `numstore/include/numstore.h`, and
  every executable (cli, pretty printers, samples, unit tests, simtest) under
  `numstore/apps/`
- One public header, one prefix. Everything a user calls is `ns_*` in
  `numstore.h`, and smartfiles is no longer a separate API
    - `nsdb_open` / `nsdb_close` / `nsdb_crash` / `nsdb_cleanup` are now
      `ns_open` / `ns_close` / `ns_crash` / `ns_cleanup`
    - `ns_open_with_error` reports why an open failed into a caller supplied
      buffer instead of just returning `NULL`
    - `nsdb_fexecute` / `nsdb_fexecute_malloc` are replaced by compiled query
      plans - `ns_plan_fcreate` compiles a query once and
      `ns_plan_execute` / `ns_plan_read` / `ns_plan_write` / `ns_plan_malloc`
      / `ns_plan_get_var` run it as many times as you like. The one shot
      `ns_execute` / `ns_read` / `ns_write` / `ns_malloc` / `ns_get_var`
      wrap that for when a plan isn't worth keeping
    - Byte level smartfiles access is the same handle and header now -
      `ns_smfile_open`, `ns_smfile_size`, `ns_smfile_read`, `ns_smfile_write`,
      `ns_smfile_insert`, `ns_smfile_remove`
    - `ns_var_free` takes only the variable, `ns_plan_strerror` gets the error
      off a plan, and `nsdb_perror` is gone
- The OS abstraction is a single `struct i_os` vtable (files, filesystem,
  threading, timing) in `core/os/ns_os.h` with posix and windows backends,
  replacing the four separate `ns_file` / `ns_filesystem` / `ns_threading` /
  `ns_time` interfaces and their per platform implementations. Allocation
  goes through `struct i_mem` the same way
- The pager is one file per operation (`ns_pgr_open`, `ns_pgr_new`,
  `ns_pgr_get`, `ns_pgr_get_writable`, `ns_pgr_release`, `ns_pgr_reserve`,
  `ns_pgr_evict`, `ns_pgr_flush`, `ns_pgr_begin_txn`, `ns_pgr_commit`,
  `ns_pgr_rollback`, `ns_pgr_cancel`, `ns_pgr_checkpoint`, `ns_pgr_recover`,
  `ns_pgr_delete`, `ns_pgr_close`, `ns_pgr_crash`, `ns_pgr_write_header`)
  instead of a single 2800 line `ns_pager.c`
- Algorithms are split per operation under `nscore/algorithms/` with a module
  each for `numstore`, `smartfiles`, `rope` and `var`
- `ns_nsdb_execute.c` folded back into `ns_nsdb.c`; `ns_nsdb_cli.c` still owns
  the command line query parsing
- Page headers, WAL records and the pager header are checksummed
- pynumstore is a different package inside
    - The extension lives at `bindings/python/src/c/ns_pynumstore.{c,h}` and
      builds through the library's CMake with scikit-build-core instead of
      setuptools
    - The wrapper is split into `database`, `transaction`, `plan`, `query`,
      `var` and `utils` modules rather than one `__init__.py`
    - `Database` and `Transaction` are context managers, `db.prepare(query)`
      returns a reusable `Plan`, and `execute` / `read` / `read_into` /
      `write` / `get` are available on both the database and a transaction
    - `pynumstore.__version__` is exposed
    - Minimum Python is 3.9 again, down from 3.10
- CI is four workflows instead of one - CI (build, unit tests, simulation
  test, release package), Coverage, Format and Wheels - each a matrix over
  linux / macos / windows by x86_64 / arm64. Every step calls a `make ci-*`
  target, so the commands live in the Makefile and not in yaml
- Docs build through CMake instead of their own Makefile
- `LICENSE.md` renamed to `LICENSE`

## Added
- A faulty OS (`core/os/faulty/ns_faulty_os.c`) - a decorator over any
  `i_os` that fails each fallible call with a configurable probability, with
  the same treatment for allocation, so error paths get exercised instead of
  assumed
- A deterministic simulation test under `nscore/testing/simulation/` - an
  operation generator, a reference state machine, and a database state
  machine that the real database is checked against after every step
- `ns_simtest`, the simulation driver, with `--dbname`, `--duration`,
  `--seed`, `--commit-hash`, `--seqid`, `--fail-percent` and `--disable`, run
  in CI on every push
- Transaction level tests (`testing/numstore/0001_insert_txn` through
  `0004_delete_txn`) and numbered regression tests for bugs that were actually
  hit - create/delete/rollback/delete, create/crash/close/delete, and rollback
  against an invalid WAL header
- `make valgrind` and `make leaks` (plus `-unit` / `-simtest` variants) over a
  dedicated `build/memcheck` tree, and `make lldb` / `make lldb-simtest` for
  dropping into the debugger at a crash
- `-Wall -Wextra -Werror` on every build type
- Five python samples under `bindings/python/samples/`, and a pytest suite
  split by subject - database, dtype, get, insert, read, remove, transactions,
  variables, write
- `ns_big_file` and `print_all_vars` apps
- Codecov upload with coverage thresholds, `CITATION.cff`, a funding file, a
  logo, and `docs/ideas.md`

## Removed
- nsserver - the server, the client, and the darwin socket layer. It wasn't
  finished and it wasn't being built
- The mingw/wine docker image used to cross compile Windows releases. Windows
  is built natively with MSVC on GitHub runners now, including arm64
- `scripts/amalgamate.py`

## Fixed
- Reading an empty variable from python raised instead of returning an empty
  array, which disagreed with `read_into` returning 0 for the same query
- A signed/unsigned mismatch in the console read loop in `ns_nsdb.c`
- A handful of Windows portability bugs - `PATH_MAX` assumptions, missing
  platform guards, and a simulation test that filled the runner's disk because
  it never removed its database

## [v1.4.1] - 2026-08-27 

## Changed
- Removed logging from python and release builds

## Added
- Adds release to the release package

## [v1.4.1] - 2026-08-27

Explicit transaction handles, Python package, and Windows/Docker build support.

## Changed
- Transactions are now first-class handles instead of implicit per-db state
    - `nsdb_begin` returns an `ns_txn_t *` instead of mutating hidden state on
      `nsdb_t`
    - `nsdb_commit` / `nsdb_rollback` / `nsdb_fexecute` /
      `nsdb_fexecute_malloc` all take that `ns_txn_t *` (or `NULL` for
      auto-commit) instead of always operating against a single active
      transaction on the db
    - `nsdb_var_free` now also takes the owning `nsdb_t *`
    - Lets multiple transactions be open against the same database at once
- Split `ns_nsdb.c` into `ns_nsdb_cli.c` (query parsing) and
  `ns_nsdb_execute.c` (query execution) instead of one file doing both
- Renamed `ns_chunk_alloc` to `ns_arena_alloc` to match what it actually is
- Rewrote the Windows filesystem backend, splitting file ops out of
  `ns_win32_filesystem.c` into a dedicated `ns_win32_file.c`
- POSIX file, filesystem, threading, and timing backends cleaned up and
  hardened
- `pynumstore` is now a proper installable package (`pip install
  ./bindings/python`) instead of a loose `.c`/`.pyi` pair
    - Extension source lives under `bindings/python/csrc/`, wrapper under
      `bindings/python/src/pynumstore/`, with `pyproject.toml` / `setup.py` /
      `py.typed`
    - Updated to the new explicit-transaction API - `db.begin()` returns a
      `Transaction` context manager that commits on clean exit and rolls back
      on exception

## Added
- New `variables` module (`src/nscore/variables/`) - in-memory representation
  of a variable, pulled out of nsdb
- Windows cross-compiled release build, built via a
  `docker/windows-x64.Dockerfile` image (mingw + wine) and wired into
  CI/release packaging
- pynumstore test suite (`bindings/python/tests/`) and README with install and
  usage docs
- `bindings/python/src/pynumstore/_pynumstore.pyi` type stub, plus `py.typed`
  marker

## [v1.3.0] - 2026-08-24

Major overhaul of build system and numstore. Broken into a single library
called libnumstore.a. This release is all about usability.

## Changed 
- When you build sarrays, it flattens dimensions instead of creating nested
  dimensions 
    - `[10][5] TYPE` gets reduced from `SARRAY(10, SARRAY(5, TYPE))` to
      `SARRAY(10, 5, TYPE)`
- CI is lightweight on the develop branch and heavy weight on master branch 
- Column limit from 80 to 100
- Reorganizes src into modules - core, nscore, numstore, smartfiles, nsserver
    - Merges smartfiles into numstore - it's no longer a separate library
      wrapped around numstore
- Memory, filesystem, threading, and timing all go through injected os
  interfaces now (posix / windows / test backends) instead of calling straight
  through to the syscalls
- Github release workflow only builds linux x64, linux arm64, and macos arm64
  for now - pulled windows and macos 13 out temporarily

## Added 
- Lots of rebalance tests
- nscli - a command line tool for numstore that reads data from the numstore
  file
- The ability to compile queries directly into nsdb_execute rather than
  seperate utilities for each
- Github workflow that builds release binaries and publishes them on tagged
  releases, pulling release notes straight out of this file
- More var hash map tests

## [v1.1.3] - 2026-06-09

### Changed 
- Big changes to the readability and organization of the code to make it more
  friendly for open source developers
    - Removes all the excess libraries - now there's just one: Numstore 
    - Adds a consistent comment and documentation scheme

## [v1.1.2] - 2026-06-01 

### Changed 

- Image link for the numstore logo in pypi release

## [v1.1.1] - 2026-06-01

### Changed

- Broke libraries into components: 
    - c_specx: In charge of core common code
    - smartfiles: A filesystem in a file 
    - nscore: Core numstore algorithms
    - numstore: smartfiles with a type system built in 
- Unit tests are now auto generated using python to remove the dependency on
  any type of constructor attributes to maintain portability
- Renames PAGE_SIZE to NS_PAGE_SIZE because some x86_64 compilers reserve
  PAGE_SIZE in limits.h

### Removed 

- submodule dependency on c_specx (added as its own dedicated library)

### Added

- pynumstore - A Python binding for numstore written in CPython
- numstore - a typed smartfiles - which constricts the variables to typed
  values
    - `prim` - A primitive type
    - `struct` - A Product type
    - `union`  - A Summation type
    - `sarray` - A strict array type
- Compilers for all new types
    - `type_ref`        - A compound type_accessor to create a new type
    - `type_accessor`   - A typed version of byte_accessor
    - `subtype`         - Sub type of an existing type
- Code coverage ci/cd target to test code coverage on unit tests 
- Swarm tests - for irwr (insert read write remove) and cgd (create get delete)
- A bunch of new ci / cd tools:
    - Builds numstore on a bunch of different architectures 
    - Runs tests 
    - Runs cibuildwheel 
    - Publishes to pypi on releases
    - Code coverage job that uploads to codecov

## [v1.0.0] - 2026-05-01

### Changed

- Files are now not suffixed by ".db" - they are exactly as you specify in open

## [v0.0.3] - 2026-04-20

### Added

- `smfile_open` / `smfile_close` — open and close a smart file in read/write
  mode
- `smfile_cleanup` — release all on-disk resources associated with a file path
- `smfile_new_context` — create an independent transaction context from an
  existing smart file
- `smfile_strerror` / `smfile_perror` — error introspection in the style of the
  C standard library

**Simple API** — treats the file as a flat byte sequence:
- `smfile_size` — query total file size in bytes
- `smfile_read` — read elements from a byte offset
- `smfile_write` — overwrite elements at a byte offset (atomic)
- `smfile_insert` — insert bytes into the middle of a file
- `smfile_remove` — remove elements and close the gap (atomic, with optional
  capture)
- `smfile_delete` — delete a named variable

**Power API** — named variables and strided element access:
- `smfile_psize` — query the size of a named variable
- `smfile_pread` — strided read from a named variable
- `smfile_pwrite` — strided write into a named variable
- `smfile_pinsert` — insert into a named variable at a byte offset
- `smfile_premove` — strided remove from a named variable (atomic, with
  optional capture)

**Transactions** — WAL-backed, two-phase locking:
- `smfile_begin` — begin an explicit transaction
- `smfile_commit` — commit all operations as a single atomic unit
- `smfile_rollback` — undo all mutations since the last `smfile_begin`

### Notes

- All simple and power operations are individually atomic by default
- Explicit transactions promote a sequence of operations to a single atomic
  unit
- Negative byte offsets are interpreted relative to end of file
- Explicit-width types used throughout for deterministic on-disk layout

[v1.5.0]: https://github.com/NumstoreDB/Numstore/compare/v1.4.1...v1.5.0
[v1.4.1]: https://github.com/NumstoreDB/Numstore/compare/v1.4.0...v1.4.1
[v1.4.0]: https://github.com/lincketheo/smartfiles/compare/v1.4.0...v1.3.0
[v1.3.0]: https://github.com/lincketheo/smartfiles/compare/v1.3.0...v1.2.0
[v1.2.0]: https://github.com/lincketheo/smartfiles/compare/v1.2.0...v1.1.3
[v1.1.3]: https://github.com/lincketheo/smartfiles/compare/v1.1.2...v1.1.3
[v1.1.2]: https://github.com/lincketheo/smartfiles/compare/v1.1.1...v1.1.2
[v1.1.1]: https://github.com/lincketheo/smartfiles/compare/v1.1.0...v1.1.1
[v1.1.0]: https://github.com/lincketheo/smartfiles/compare/v1.0.0...v1.1.0
[v1.0.0]: https://github.com/lincketheo/smartfiles/compare/v0.0.3...v1.0.0
[v0.0.3]: https://github.com/lincketheo/smartfiles/compare/v0.0.2...v0.0.3
[v0.0.2]: https://github.com/lincketheo/smartfiles/compare/v0.0.1...v0.0.2
[v0.0.1]: https://github.com/lincketheo/smartfiles/compare/releases/tag/v0.0.1
