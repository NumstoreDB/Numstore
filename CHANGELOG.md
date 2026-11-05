# Changelog

The internals record: what changed in the tree, file by file. For what a
release means if you *use* numstore, read the notes in
[docs/releases/](docs/releases/).

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Nothing yet.

## [1.5.1] - Unreleased

User-facing notes: [docs/releases/v1.5.1.md](docs/releases/v1.5.1.md).

### Added

- `libs/nsserver`: Rust gRPC service (tonic 0.14 / prost 0.14), with
  `apps/numstore` serving it and `apps/nsclient` as a demo client. Four RPCs -
  Push, Pull, Execute, Get - over a vendored `protoc`. No numstore integration;
  all behaviour is in `libs/nsserver/src/hooks.rs`.
- Cargo workspace at the root with explicit members, and `Cargo.lock` committed.
- Corrosion under `cmake/Rust.cmake`, included by every non-wheel build. Wheel
  builds return before it and need no Rust toolchain.
- `ns_crash.{c,h}`: `intentionally_crash()` longjmps out to an enclosing
  `capture_intentional_crash` in debug builds, aborts in release.
- WAL split into `ns_wal_istream.{c,h}`, `ns_wal_ostream.{c,h}` and
  `ns_wal_record.{c,h}`, each with its own tests.
- ARIES restart split into `ns_pgr_restart_{analysis,redo,undo}.c`, with the
  recovery context in `ns_aries_ctx.c`.
- `ns_node_updates.{c,h}`, `ns_numstore_remove.c`, checked arithmetic in
  `ns_bounds.h`, `gcovr.cfg`.
- Regression `0005_crash_twice_in_txn`, and ten WAL unit tests with allocator
  and OS fault injection.
- `make rust` and `make test-rust`; a `rust` job in CI.

### Changed

- Layout: `csrc/` -> `libs/numstore/src/`, `include/numstore.h` ->
  `libs/numstore/include/numstore.h`, `pynumstore/` and `csrc/ns_pynumstore.*`
  -> `libs/pynumstore/src/pynumstore/`, `pytest/` -> `libs/pynumstore/tests/`.
  No `#include` line changed. `gen_tests.py` takes the source root as an
  argument; `pyproject.toml` uses `wheel.packages`, `wheel.exclude` and
  `sdist.exclude`.
- WAL writer stays closed until a forward scan has found the end of the log;
  `wal_size` and `wal_flush_all` assert it exists.
- `walis_mark` commits what was read and returns the resulting position.
- WAL write failures latch into the stream (`walos_failed` / `walos_take_err`)
  instead of crashing inside it; the latch owner releases before dying.
- `WAL_START_LSN` / `WAL_RECORDS_START` replace open-coded `sizeof (lsn)`.
- `struct wal_clr_read.type` narrowed to `wlh` to match its on-disk width.
- `wal_forward_scan` replaces the `wal_read_next` cursor.
- Docs reorganised into an audience tree under `docs/`; `docs/release.md`
  removed in favour of `docs/releases/`.

### Fixed

- `wal_init` opened the reader read-only on a file that need not exist, so a
  fresh WAL could not be opened.
- `wal_write_start_lsn` opened a second writer at offset 0, letting the first
  record overwrite the start-lsn header.
- `wal_read_one` fell through every case into `UNREACHABLE()`.
- `wal_read_entry` dereferenced a NULL destination record.
- `wal_seek_unsafe(LSN_NULL)` seeked to `U64_MAX`.
- CLR sub types decoded three uninitialised bytes.
- `wal_forward_scan` reported the previous record's start as the end of the
  log, truncating an intact record on every restart; and `walos_open` now
  truncates to the position it opens at, so a torn tail cannot read back later
  as a bad log type or CRC.
- Pages changed by redo, undo and rollback were never marked dirty, so a long
  replay was discarded frame by frame as the buffer pool evicted.
- WAL-before-page was skipped for all of recovery, which is wrong for undo;
  recovery now also flushes its own END and CLR records.
- `pgr_open` discarded the WAL when one header lsn was torn, instead of
  repairing the slot and recovering.
- `dpgt_merge_into` could insert a duplicate entry for a page the destination
  already held.
- `walos_open` left its own `error` uninitialised.
- `wal_ensure_flush_next` wrote a megabyte of uninitialised stack into the log.
- `wal_delete_and_reopen` leaked the latch and the file name on error paths.
- `ns_rope_in_scan` requested `PG_DATA_LIST` while walking the inner-node
  sibling chain.

## [1.5.0] - 2026-10-29

History start: [docs/releases/v1.5.0.md](docs/releases/v1.5.0.md). Releases
before this one exist as tags but were never written up.

[Unreleased]: https://github.com/NumstoreDB/Numstore/compare/v1.5.1...HEAD
[1.5.1]: https://github.com/NumstoreDB/Numstore/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/NumstoreDB/Numstore/releases/tag/v1.5.0
