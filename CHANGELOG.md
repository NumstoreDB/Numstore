# Changelog

## [v1.1.0-Unreleased]

### Changed

- Broke libraries into components: 
    - c_specx: In charge of core common code
    - smartfiles: A filesystem in a file 
    - nscore: Core numstore algorithms
    - numstore: smartfiles with a type system built in 
- Unit tests are now auto generated using python to remove the 
  dependency on any type of constructor attributes to maintain 
  portability
- Renames PAGE_SIZE to NS_PAGE_SIZE because some x86_64 compilers 
  reserve PAGE_SIZE in limits.h

### Removed 

- submodule dependency on c_specx (added as its own dedicated library)

### Added

- pynumstore - A Python binding for numstore written in CPython
- numstore - a typed smartfiles - which constricts the variables to typed values
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

- `smfile_open` / `smfile_close` — open and close a smart file in read/write mode
- `smfile_cleanup` — release all on-disk resources associated with a file path
- `smfile_new_context` — create an independent transaction context from an existing smart file
- `smfile_strerror` / `smfile_perror` — error introspection in the style of the C standard library

**Simple API** — treats the file as a flat byte sequence:
- `smfile_size` — query total file size in bytes
- `smfile_read` — read elements from a byte offset
- `smfile_write` — overwrite elements at a byte offset (atomic)
- `smfile_insert` — insert bytes into the middle of a file
- `smfile_remove` — remove elements and close the gap (atomic, with optional capture)
- `smfile_delete` — delete a named variable

**Power API** — named variables and strided element access:
- `smfile_psize` — query the size of a named variable
- `smfile_pread` — strided read from a named variable
- `smfile_pwrite` — strided write into a named variable
- `smfile_pinsert` — insert into a named variable at a byte offset
- `smfile_premove` — strided remove from a named variable (atomic, with optional capture)

**Transactions** — WAL-backed, two-phase locking:
- `smfile_begin` — begin an explicit transaction
- `smfile_commit` — commit all operations as a single atomic unit
- `smfile_rollback` — undo all mutations since the last `smfile_begin`

### Notes

- All simple and power operations are individually atomic by default
- Explicit transactions promote a sequence of operations to a single atomic unit
- Negative byte offsets are interpreted relative to end of file
- Explicit-width types used throughout for deterministic on-disk layout

[v1.1.0-Unreleased]: https://github.com/lincketheo/smartfiles/compare/v1.0.0...HEAD
[v1.0.0]: https://github.com/lincketheo/smartfiles/compare/v0.0.3...v1.0.0
[v0.0.3]: https://github.com/lincketheo/smartfiles/compare/v0.0.2...v0.0.3
[v0.0.2]: https://github.com/lincketheo/smartfiles/compare/v0.0.1...v0.0.2
[v0.0.1]: https://github.com/lincketheo/smartfiles/compare/releases/tag/v0.0.1
