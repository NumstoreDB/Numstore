Conventions
===========

Match the file you're in before any rule below.

Formatting
----------

Formatting is owned by `.clang-format`. Run it rather than hand-aligning. The
shape it produces, which you should write from the start:

- Return type on its own line, then `name (args)` with a space before the
  paren (calls too: `wal_flush_all (p->ww)`).
- Braces on the same line for control flow, own line for function bodies.
- Aligned declarations in structs and blocks of locals.

Naming
------

- `lower_snake_case` for functions, variables, struct tags and enum values,
  unless the file uses `UPPER_CASE` for that enum.
- Prefix functions with their module: `pgr_`, `wal_`, `walos_`, `lockt_`,
  `txnt_`, `nsdb_`.
- Name lengths grow with scope: `l`, `tx`, `p` are fine for locals, never for
  globals or public functions.
- Functions are verbs (`pgr_begin_txn`), data is nouns (`struct txn_data`).
- Use the project types: `u8`..`u64`, `i32`, `lsn`, `slsn`, `txid`, `pgno`,
  `spgno`, `err_t`.

Errors
------

- Fallible functions return `err_t` (or a pointer / signed value where
  negative or `NULL` means failure) and take `error *e` as the last parameter.
- Start an error with `error_causef (e, ERR_..., "fmt", ...)`; propagate it
  with `return error_trace (e);`.
- On failure, undo what this function already did (unlock, free) before
  returning, in reverse order of acquisition.
- Return early on failure so the happy path stays flat.
- Unrecoverable failures (for example a WAL write that can't complete) call
  `intentionally_crash ()` rather than returning an error.
- Check for failure with `< 0`, not truthiness:
  `if (pgr_new (&pg, p, tx, type, e) < 0)`.
- Program bugs are assertions, not errors. See
  [assertions.md](assertions.md).

Headers
-------

- Group declarations under short `// Lifecycle`, `// Getters`, `// Appending`
  style headings, or `SECTION:` banners in larger files.
- Put the struct's `DEFINE_DBG_ASSERT` right after the struct.
- Doc comments use `///` and refer to parameters as `[name]`.
- Debug-only helpers and all tests live under `#ifndef NDEBUG`.
