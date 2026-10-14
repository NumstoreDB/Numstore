# Release TODO

Findings from an audit of the tree at `717f901` (83 commits past `v1.4.1`).
Everything here was verified by building and running, not inferred.

If only three get done: **1** (test what you ship), **2** (the 133 MB landmine),
**3** (make the library installable).

## Blockers

### 1. The configuration we ship is the only one never tested

`unit_tests` is guarded to Debug (`numstore/apps/unit_tests/CMakeLists.txt:3`),
and a Release build confirms it: the target does not exist. Meanwhile Release is
`-O3 -DNDEBUG`, and the NDEBUG branch of `numstore/src/core/ns_csx_assert.h`
turns `DBG_ASSERT` into `((void)sizeof(expr))`, `ASSERT_NN(ptr)` into `ptr`, and
`panic()` into nothing.

So all 336 C tests and every invariant check run *only* in the configuration we
don't ship. The Python wheel is built Release, which makes the 97 Python tests
the sole Release coverage of a 26,749-line C library.

Cheap first step: `ns_simtest` *does* build in Release (it is unguarded in
`numstore/apps/CMakeLists.txt`). A `ci-release-simtest` target running it
against the Release tree exercises crash/recovery with assertions compiled out,
which is the shape we actually ship.

### 2. A 133 MB file named `test` sits in the repo root, untracked and unignored

35 call sites open a database literally named `"test"` in the current working
directory — e.g. `ns_read_var_page.c:307`, `ns_visit_variables.c:175`,
`numstore/src/nscore/testing/numstore/0003_create_txn.c:31`.

- `git check-ignore test` reports **not ignored**, so `git add -A` stages 133 MB
- `scripts/clean_all.sh` does not match it; its patterns are `*.db`, `*.nsdb`,
  `*.wal`, `testdb*`, `vgcore.*`, `gmon.out`

Fix properly by having tests write to a temp directory. At minimum, gitignore
the name and add the pattern to `clean_all.sh`.

### 3. Consumers cannot use the installed library

`make ci-release` installs exactly two files:

```
include/numstore.h
lib/libnumstore.a
```

`numstore/CMakeLists.txt:72` declares `install(TARGETS ... EXPORT
numstoreTargets ...)` but there is **no matching `install(EXPORT ...)`**, so the
export set is dead. There is no `numstoreConfig.cmake` and no pkg-config `.pc`
file, so `find_package(numstore)` cannot work and integrators must hand-wire
include and link paths.

### 4. Three `TODO`s in the public header

`numstore/include/numstore.h` lines 414 (`end of the file - TODO.`), 442, and
472. This is the one file every user reads.

### 5. No thread-safety contract, with open concurrency TODOs

Neither `README.md` nor `numstore.h` states whether an `nsdb_t` may be shared
across threads — the first question anyone asks of a database. The primitives
carry unresolved notes:

| location | note |
|---|---|
| `core/ns_concurrency.h:132` | ABA problem |
| `core/ns_concurrency.c:721` | spurious wakeups |
| `core/ns_concurrency.c:78` | caller must ensure all threads have released locks |

Either document single-threaded-only use, or close these.

## Correctness worth closing

### 6. `txn_update()` ignores its `state` argument

`numstore/src/nscore/txn_table/ns_txn_table.c:61` does `(void)state;` and then
hardcodes `.state = TX_CANDIDATE_FOR_UNDO`.

**Latent, not live.** The only caller, `ns_pgr_recover.c:129`, passes exactly
that value. But this is in the ARIES recovery path and will silently do the
wrong thing the first time anyone passes `TX_COMMITTED` or `TX_DONE`.

### 7. Two acknowledged error-path gaps, both corruption-adjacent

- `nscore/algorithms/rope/ns_balance_and_release.c:546` — "graceful error
  handling and clean up of partial"
- `nscore/lock_table/ns_lock_table.c:260` — "error handling here for failed
  locks"

A rope rebalance that fails part-way is the kind of thing the ACID claim rests
on.

### 8. `leaks` cannot be a gate yet

`make leaks-unit` reports **277 leaks / 91,761,024 bytes**, but every large
block traces to `pgr_fixture_create` (`nscore/page/ns_page_fixture.c:73`) —
test fixtures with no teardown, not library leaks:

```
default_calloc                    ns_malloc.c:65
pgr_open                          ns_pgr_open.c:159
pgr_fixture_create                ns_page_fixture.c:73
__test_body__smartfiles_insert    ns_smartfiles_insert.c:92
```

Give the fixtures teardown, or exclude them with `leaks -e <symbol>`, so the
target becomes meaningful. `make leaks-simtest` is the one that could plausibly
be green today — it is unverified. Valgrind is entirely unverified: there is no
working valgrind for macOS arm64, so it needs a Linux CI job.

## Release hygiene

### 9. `CHANGELOG.md` has two identical release headings

`## [v1.4.1] - 2026-08-27` appears at both line 3 and line 11; the first is
presumably unreleased work. Nested `## Changed` should be `###` — the levels are
flat, so every subsection renders as a sibling of the release heading.

### 10. `LICENSE` still carries the template placeholder

`Copyright [yyyy] [name of copyright owner]` is unfilled, while
`scripts/add_copywrite.py` stamps `Copyright 2026 Theo Lincke` into all 240
source files. The license file is the odd one out.

### 11. 83 commits since `v1.4.1` with no tag

Also note git tags are not GitHub Releases; cutting one does not create the
other.

## Python / PyPI

### 12. No publish path exists

The upload scripts and twine were removed. Use trusted publishing (OIDC) on tag
rather than restoring API tokens.

### 13. Decide the version

`bindings/python/version.txt` is `0.1.0`, which signals "expect breakage", while
the C library is `1.4.1`. A published version cannot be walked back down.

### 14. Metadata gaps in `bindings/python/pyproject.toml`

- no `classifiers` (PyPI shows the package uncategorized, and does not advertise
  supported Pythons)
- no `keywords`
- `project.urls` has only `Homepage` — no Source, Issues, or Changelog
- the license **text** will not ship: the SPDX identifier is set, but there is no
  `LICENSE` in `bindings/python/` and no `license-files` entry

### 15. Add `cp314`

`build = "cp39-* cp310-* cp311-* cp312-* cp313-*"`; Python 3.14 is current.

## CI confidence

### 16. `ci-wheels` has never completed

It previously died at argument parsing, so no leg has produced a wheel in CI.
The Linux legs matter most, since that is where cibuildwheel's `/project` copy
semantics apply.

### 17. CI no longer fuzzes

Seeds are fixed at `123142`. Previously six matrix lanes explored new random
seeds on every push and reported them via `::notice`. For a crash-recovery
database this is the largest lost signal; one overridable `SEED ?=` restores it.

### 18. Unverified CI assumptions

- Windows runners are assumed to have GNU Make on `PATH`
- coverage thresholds (80 line / 45 branch) were extrapolated from an
  Apple-clang run measuring 88.0% line / 54.9% branch; gcc counts branches
  differently

## Before starting any of the above

Commit or discard the working tree: `Makefile`, `numstore/CMakeLists.txt`,
`ns_nsdb.c`, both `version.txt` files, three files under
`bindings/python/src/pynumstore/`, and four new `tests/*_ai.py` files — plus the
uncommitted `branches: [develop]` workflow trigger change, and the stray `test`
file from item 2.
