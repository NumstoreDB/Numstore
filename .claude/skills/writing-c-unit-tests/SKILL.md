---
name: writing-c-unit-tests
description: How to write, place and run C tests in Numstore - the TEST/TEST_CASE framework, test_ctx, assertions, fixtures, test marks, fault injection, crash capture, regression tests and the simulation harness. Use whenever you add, change or review a C test under libs/numstore/src/.
---

Writing C tests in Numstore
===========================

General principles (grouping, parameterization, simulation, regressions)
are in [what-makes-a-good-test.md](references/what-makes-a-good-test.md).
This file is how they map onto this codebase.

For functions that can fail on OS or memory errors, also read
[error-tests.md](references/error-tests.md). Style for the test code itself
is in [writing-code](../writing-code/SKILL.md). Crash and recovery tests
should be checked against
[working-with-aries-logic](../working-with-aries-logic/SKILL.md).

Where tests live
----------------

Tests sit at the bottom of the `.c` file they test, inside `#ifndef NDEBUG`:

```c
#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

/* ... implementation ... */

#ifndef NDEBUG
TEST (wal_open)
{
  ...
}
#endif
```

There is no registration. `apps/gen_tests.py` scrapes every `TEST (name)`
under `libs/numstore/src/` and generates `unit_tests.c`. Test names must be unique across
the whole tree; prefix them with the module (`wal_`, `pgr_`, `lockt_`).

Other places:

| Kind                          | Location                                                                   |
| ----------------------------- | -------------------------------------------------------------------------- |
| Unit tests                    | bottom of the `.c` under test                                              |
| End to end ARIES / crash      | `libs/numstore/src/nscore/testing/ns_aries_tests.c`                        |
| Public API (`ns_*`) txn flows | `libs/numstore/src/nscore/testing/numstore/NNNN_*.c`                       |
| Regression tests              | `libs/numstore/src/nscore/testing/regressions/NNNN_*.c`                    |
| Multithreaded stress          | next to the algorithm, e.g. `algorithms/numstore/tests/`                   |
| Randomized simulation         | `libs/numstore/src/nscore/testing/simulation/`, run by `apps/ns_simtest.c` |

New files under `testing/` must be added to that directory's `CMakeLists.txt`.

Running
-------

```sh
make test-unit                         # all unit tests, debug build
./build/debug/bin/unit_tests 42        # seed is required
./build/debug/bin/unit_tests 42 wal_   # filter: substring of the test name
make test-simtest                      # randomized simulation
make valgrind-unit / make leaks-unit   # memcheck build
```

The memcheck build uses `-Wextra -Werror`, so unused variables in tests break
it. `make leaks-unit` doesn't rebuild an existing memcheck binary, so run
`cmake --build build/memcheck -j12` first or you'll test stale code.

The framework (`core/ns_testing.h`)
-----------------------------------

### TEST and ctx

`TEST (name) { ... }` gives the body a `struct test_ctx *ctx`:

| Field              | Use                                                    |
| ------------------ | ------------------------------------------------------ |
| `ctx->e`           | an `error` to pass to the code under test              |
| `ctx->mem`         | the real allocator                                     |
| `ctx->os`          | the real OS                                            |
| `ctx->faulty_mem`  | allocator you can make fail (see error-tests.md)       |
| `ctx->faulty_os`   | OS you can make fail                                   |
| `ctx->alloc`       | arena over `mem`, freed automatically                  |
| `ctx->faulty_alloc`| arena over `faulty_mem`                                |

`ctx` is freed when the test returns, and that asserts no file is left open
and no thread unjoined. Close and join everything you open.

`TEST_DISABLED (name)` keeps a test compiling but stops it running. Use it
sparingly and say why in a comment.

### TEST_CASE

Group related checks into one `TEST` with `TEST_CASE` blocks. Each case logs
its own pass or fail:

```c
TEST (test_mark_match)
{
  TEST_CASE ("basics")
  {
    test_assert (test_mark_match ("foo", "foo"));
    test_fail_if (test_mark_match ("foo", "fo"));
  }

  TEST_CASE ("wildcard")
  {
    test_assert (test_mark_match ("*", ""));
    test_assert (test_mark_match ("foo:*", "foo:bar"));
  }
}
```

`TEST_CASE` takes a printf format, so it works inside loops:
`TEST_CASE ("Fail after: %d", fail_after)`.

### Assertions

All of them log the location and `return` from the test body on failure. That
means cleanup after a failed assert does not run, which is fine: the run is
already failing.

| Macro                                  | Checks                                |
| -------------------------------------- | ------------------------------------- |
| `test_assert (expr)`                   | `expr` is true                        |
| `test_fail_if (expr)`                  | `expr` is false                       |
| `test_assert_equal (a, b)`             | `a == b`                              |
| `test_assert_int_equal (a, b)`         | `a == b` as `i32`, prints both values |
| `test_assert_type_equal (a, b, T, FMT)`| `a == b` as type `T`, prints with `FMT` (e.g. `u64, PRIu64`) |
| `test_assert_ptr_equal (a, b)`         | pointer equality                      |
| `test_assert_memequal (a, b, n)`       | `memcmp == 0`                         |
| `test_err_t_check (expr, exp, &e)`     | `expr` returns `exp`, then resets `e` |

Prefer the typed forms: a failure that prints both values is much faster to
debug than `Expected: x == y`.

After asserting an expected error, call `error_reset (&ctx->e)` (or use
`test_err_t_check`) so the next call starts clean.

### Parameterized tests

There's no parameterization macro. Use a static table and a loop with a
`TEST_CASE` per row, as in `core/ns_tests.c`:

```c
static const struct
{
  const char *name;
  u16         h16;
  u32         expected_f32_bits;
} f16_f32_cases[] = {
    {"positive zero", 0x0000u, 0x00000000u},
    {"-1.0",          0xBC00u, 0xBF800000u},
    ...
};

TEST (f16_to_f32)
{
  for (u32 i = 0; i < arrlen (f16_f32_cases); i++) {
    TEST_CASE ("%s", f16_f32_cases[i].name)
    {
      ...
    }
  }
}
```

Name every row so a failure says which one.

### Test marks: asserting a branch was taken

To prove a test reaches a specific path (a rebalance case, a lock wait), drop
a mark in the implementation and assert on it in the test. Marks compile away
in release builds.

```c
// implementation
TEST_MARK ("gr_lock:gr_lock:wait");

// test
test_reset_marks ();
...
test_assert_mark_hit ("gr_lock:gr_lock:wait");
test_assert_mark_not_hit ("gr_lock:*:immediate_acquire");
```

Name marks `module:function:branch`. Patterns support `*`.

### Named faults

`FAULT (expr, "name")` and `FAULT_NULL (expr, "name")` in implementation code
make a specific call fail when a test calls `fault_set ("name")`. Call
`fault_reset_all ()` at the end of the test. Use this when you need one
specific call to fail; use `fail_after` (error-tests.md) to sweep every call.

### Crashes

Code that hits an unrecoverable failure calls `intentionally_crash ()`. In a
test, wrap it so the crash becomes an error code:

```c
err_t ret;
capture_intentional_crash (&ret, &ctx->e)
{
  ret = pgr_begin_txn (&f.tx, f.p, &ctx->e);
}
// ret == ERR_CRASHED if it crashed
```

A captured crash `longjmp`s out, so latches taken inside are still held.
See error-tests.md for how to clean up.

To simulate a whole-process crash on purpose, use the module's crash call
(`ns_crash`, `nsdb_crash`, `pgr_crash`, `wal_crash`) and reopen.

Fixtures
--------

Reuse fixtures before writing setup by hand.

- `struct pgr_fixture` (`nscore/page/ns_page_fixture.h`): a pager plus a
  `txn` and an arena. `pgr_fixture_create (&f, &ctx->e)` uses the real OS and
  allocator; `pgr_fixture_create_with (&f, os, mem, &ctx->e)` takes your own
  (pass `ctx->faulty_os` / `ctx->faulty_mem` for error tests). Always pair with
  `pgr_fixture_teardown[_with]`.
- `build_tree_from_descr` and the in-page builders in the same header build
  page layouts to test against.
- Database-level tests: `nsdb_cleanup (name, ctx->os, &ctx->e)` before
  `nsdb_open`, so each case starts from an empty database.
- WAL tests: the `wal_test_fresh` helper in `ns_wal.c`.

Each `TEST_CASE` should set up its own state when cases could interfere. Start
database tests with a cleanup so a previous failed run can't leak in.

Concurrency
-----------

Anything using latches, locks or atomics deserves a threaded test. Follow
`ns_insert_same_var_multithreaded.c`: a `ready` counter and a `go` flag so
every thread starts at once, then join all threads and assert invariants.
Run it under `make valgrind-unit` or `make leaks-unit` too.

Regression tests
----------------

Every fixed bug gets one in `libs/numstore/src/nscore/testing/regressions/`, named
`NNNN_short_description.c` with the next free number, test name matching the
file name. Keep it the smallest sequence that reproduces the bug, and put a
`CAUSE:` comment at the step that used to fail:

```c
  // Delete the variable
  //
  // This Failed - it shouldnt
  //        CAUSE:
  //          The first log of fsm is a fsm update log. But the fsm page starts
  //          uninitialized, therefore it needs one upfront physical log first
  //          before it can be used ...
  res = ns_execute (db, NULL, "delete MkWMJ9a");
  test_assert_int_equal (res, 0);
```

If the simulation found it, record the seed in the comment.

Before finishing
----------------

1. Is the test under `#ifndef NDEBUG`, with a unique module-prefixed name?
2. Are related checks grouped into `TEST_CASE`s instead of many tiny tests?
3. Do assertions use typed forms that print values?
4. Is every file closed, thread joined, fixture torn down?
5. Does a function that touches the OS or allocator have a fail_after test?
   Is every function, including new helpers, called by some test? Function
   coverage must stay at 100%.
6. Does a fixed bug have a numbered regression test with a `CAUSE:` comment?
7. Does `make test-unit` pass, and the memcheck build still compile?
