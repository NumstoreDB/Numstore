# Changelog

## [v1.4.0] - 2026-08-27

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
