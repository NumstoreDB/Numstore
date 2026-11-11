Error tests (fail_after sweeps)
===============================

Use this for any function that can fail because of the OS (file I/O, threads,
timers) or the allocator. Instead of finding every call that can fail, make
the Nth fallible call fail and sweep N upward from 0 until the function
succeeds. Every failure point gets hit once, in order, and new failure points
added later are covered automatically.

The tools
---------

From `common/os/ns_os.h`, `common/os/ns_malloc.h` and `common/ns_crash.h`:

| Call                                        | Effect                                      |
| ------------------------------------------- | ------------------------------------------- |
| `faulty_os_fail_after (ctx->faulty_os, n)`  | let `n` OS calls succeed, fail the next one |
| `faulty_os_reset (ctx->faulty_os)`          | stop failing                                |
| `faulty_mem_fail_after (ctx->faulty_mem, n)`| same for allocations                        |
| `faulty_mem_reset (ctx->faulty_mem)`        | stop failing                                |
| `capture_intentional_crash (&ret, &ctx->e) { ... }` | turn `intentionally_crash ()` into `ret = ERR_CRASHED` |

`n = 0` fails the very first call. After a failure fires, the budget disarms
itself, but always call the reset anyway.

The code under test must be built on the faulty interfaces: pass
`ctx->faulty_os` and `ctx->faulty_mem` into whatever you construct, e.g.
`pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e)` or
`wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0)`.

The pattern
-----------

```c
#ifndef NDEBUG
TEST (pgr_begin_txn_fail_no_wal)
{
  struct pgr_fixture f;
  pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

  struct txn tx;
  int        fail_after = 0;
  err_t      ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // Seed the state so the call takes the path you want to cover
      wal_flush_all (f.p->ww);

      // Arm, call, disarm - nothing else between arm and disarm
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      ret = pgr_begin_txn (&tx, f.p, &ctx->e);
      faulty_os_reset (ctx->faulty_os);

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  // The last iteration succeeded - finish the work and clean up
  pgr_commit (f.p, &tx, &ctx->e);
  pgr_fixture_teardown_with (&f, &ctx->e);
}
#endif
```

Rules:

1. Loop with `do { ... } while (ret != SUCCESS)`. The sweep ends at the first
   `n` large enough that nothing failed.
2. One `TEST_CASE ("Fail after: %d", fail_after)` per iteration, so a failure
   names the `n` that broke.
3. `error_reset (&ctx->e)` at the top of each iteration.
4. Only the call under test sits between arm and reset. Setup and cleanup run
   on a healthy OS/allocator.
5. Write one sweep for the OS (`faulty_os_*`) and one for memory
   (`faulty_mem_*`). Name them `<func>_fail_<scenario>`.
6. After a failed call the object must still be usable (or cleanly torn
   down). Use it again, or tear it down, to prove nothing leaked or stayed
   locked. The valgrind/leaks builds catch what you miss.

Seed the path you want to cover
-------------------------------

A sweep only covers the calls the function actually makes. If a failure point
sits behind a condition, set the condition up first. Use your knowledge of the
code: for example the WAL only writes to disk when its buffer is full, so a
plain `pgr_begin_txn` makes no OS calls at all. To cover the flush path, write
a second test that calls `wal_ensure_flush_next (w)` first, so the next append
must flush.

When the failure crashes
------------------------

Some failures are unrecoverable by design: a WAL write that fails calls
`intentionally_crash ()` instead of returning. Wrap the call so the crash
becomes `ret = ERR_CRASHED`, and rebuild state every iteration, because a
crashed object can't be reused:

```c
TEST (pgr_begin_txn_fail_full_wal)
{
  struct pgr_fixture f;
  int                fail_after = 0;
  err_t              ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      // Fresh fixture every iteration
      pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

      // Force the next append to flush
      wal_ensure_flush_next (f.p->ww);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_begin_txn (&f.tx, f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      // Release the db lock on success
      if (ret == SUCCESS) {
        pgr_commit (f.p, &f.tx, &ctx->e);
      }

      // Teardown handles both the success and the crash case
      pgr_fixture_teardown_with (&f, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}
```

If the call returns `void`, set `ret = SUCCESS;` as the last statement inside
the capture block, so reaching it means no crash.

Cleaning up after a captured crash
----------------------------------

A captured crash `longjmp`s out of the middle of the code under test, skipping
its normal unlock and cleanup. After a real crash locks are meaningless, so
the test may break internals to get back to a clean state:

- Release latches the crashed function held, e.g. `latch_unlock (&w->l)` for
  the WAL latch taken by `wal_append_*`.
- Tear down with the crash path (`wal_crash`, `pgr_crash`) rather than a
  clean close, then remove leftover files.
- `error_reset` between cleanup steps, since cleanup after a crash may itself
  report errors you don't care about.

```c
  do {
    TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
    {
      // Buffer is full, so the next append has to flush
      struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
      test_assert (w != NULL);
      wal_ensure_flush_next (w);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        wal_append_begin_log (w, 1);
        ret = SUCCESS;
      }

      // wal_append_begin_log holds w->l when it crashes
      latch_unlock (&w->l);

      faulty_os_reset (ctx->faulty_os);
      error_reset (&ctx->e);
      wal_crash (w, &ctx->e);
      error_reset (&ctx->e);
      ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);
```

Undo only what the crash left behind (the latch the crashed
call was holding, the files it opened) and say why in a comment. Don't reach
further into internals than that. If cleanup needs more than a line or two of
surgery, the code under test probably needs a proper crash/teardown path
instead.

Checklist
---------

1. Is everything under `#ifndef NDEBUG`?
2. Is the object built on `ctx->faulty_os` / `ctx->faulty_mem`?
3. Is there a sweep for each of OS and memory where both can fail?
4. Is every conditional failure path seeded by its own test?
5. Is only the call under test between arm and reset?
6. Do crash-capable calls sit inside `capture_intentional_crash`, with fresh
   state each iteration?
7. After a crash, are held latches released and the object torn down through
   its crash path?
