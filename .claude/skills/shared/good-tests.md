Good tests
==========

Real tests from `csrc/` to copy the shape of. How the framework works is in
[writing-c-unit-tests/SKILL.md](../writing-c-unit-tests/SKILL.md). The general
reasoning is in
[what-makes-a-good-test.md](../writing-c-unit-tests/references/what-makes-a-good-test.md).

If the code moves, the excerpts here may drift. Check the source before
quoting one as current.

A parameterized table
---------------------

`csrc/core/ns_tests.c`, `f16_to_f32_normals_and_specials`:

```c
static const struct
{
  const char *name;
  u16         h16;
  u32         expected_f32_bits;
} f16_f32_cases[] = {
    // Positive zero: sign=0, exp=0, mant=0
    {"positive zero", 0x0000u, 0x00000000u},
    // 1.0:  sign=0, exp=15, mant=0  → f32 exp = 15+112 = 127
    {"1.0", 0x3C00u, 0x3F800000u},
    // +infinity: sign=0, exp=31, mant=0
    {"+infinity", 0x7C00u, 0x7F800000u},
    ...
};

TEST (f16_to_f32_normals_and_specials)
{
  for (u32 i = 0; i < arrlen (f16_f32_cases); ++i) {
    TEST_CASE ("%s", f16_f32_cases[i].name)
    {
      float result = f16_to_f32 (f16_f32_cases[i].h16);
      u32   bits;
      memcpy (&bits, &result, sizeof bits);
      test_assert_type_equal (bits, f16_f32_cases[i].expected_f32_bits, u32, PRIu32);
    }
  }
}
```

Every row is plain data and has a name, so a failure says which case broke.
The comment on each row shows how the expected bits were worked out. NaN and
subnormals have their own tests because their checks don't fit the row shape.

A fail_after sweep
------------------

`csrc/core/ns_ext_array.c`, `ext_array_insert_fail_nomem`:

```c
  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);
      struct ext_array a = ext_array_create (ctx->faulty_mem);
      test_assert_type_equal (ext_array_insert (&a, 0, head, 4, &ctx->e), (i64)4, i64, PRId64);

      // Insert in the middle so both the grow and the tail copy allocate
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = ext_array_insert (&a, 2, mid, 3, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      if (ret < 0) {
        // A failed insert leaves the contents alone
        test_assert_type_equal (ext_array_get_len (&a), (u64)4, u64, PRIu64);
        test_assert_memequal (a.data, head, 4);
      } else {
        const u8 expected[7] = {1, 2, 9, 9, 9, 3, 4};
        test_assert_type_equal (ext_array_get_len (&a), (u64)7, u64, PRIu64);
        test_assert_memequal (a.data, expected, 7);
      }

      ext_array_free (&a);
      fail_after++;
    }
  }
  while (ret < 0);
```

The setup chooses the path that reaches both allocations ("insert in the
middle"). Only the call under test sits between arm and reset. Both outcomes
are checked: a failure leaves the array untouched, and a success gives the
right bytes. The full rules are in
[error-tests.md](../writing-c-unit-tests/references/error-tests.md).

A threaded test with no timing assumptions
------------------------------------------

`csrc/core/ns_gr_lock.c`, `gr_lock_is_x_blocks`:

```c
  // Thread 1 holds IS until we release it, so once thread 2 is on the waiter
  // list it stays there - X is incompatible with IS
  int held[LM_COUNT];
  while (!lock_sample (&lock, held)) {
    spin_pause ();
  }

  test_assert (atomic_load (&lctx.t1_acquired));
  test_assert (atomic_load (&lctx.t2_blocked));
  test_assert (!atomic_load (&lctx.t2_acquired));

  // Thread 2 is parked, so it holds nothing - only thread 1's IS is granted
  test_assert_equal (held[LM_IS], 1);
  test_assert_equal (held[LM_X], 0);

  atomic_store (&lctx.t1_release, 1);

  ctx->os.table->thread_join (ctx->os.self, t1.self);
  ctx->os.table->thread_join (ctx->os.self, t2.self);
```

It waits for a state it can observe (thread 2 on the waiter list) instead of
sleeping and hoping. Thread 1 doesn't release until the check is done. Both
threads are joined. The comment above the test records the flaky version it
replaced (see [good-comments.md](good-comments.md#recording-a-bug-fix)).

Don't copy the compatibility matrix test earlier in the same file: it still
does `i_sleep_ms (10)` and then asserts the second thread hasn't locked.

A regression test
-----------------

`csrc/nscore/testing/regressions/0002_create_crash_close_delete.c`:

```c
TEST (0002_create_crash_close_delete)
{
  sb_size res;

  // Clean re open database
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Create the variable
  res = ns_execute (db, NULL, "create MkWMJ9a [8][9][3][3] i16");
  test_assert_int_equal (res, 0);

  // Crash, then re open
  test_assert_int_equal (ns_crash (db), 0);
  db = ns_open ("test");
  test_assert (db != NULL);

  // Cleanly close, then re open
  test_assert_int_equal (ns_close (db), 0);
  db = ns_open ("test");
  test_assert (db != NULL);

  // Delete the variable
  //
  // This Failed - it shouldnt
  //        CAUSE:
  //          ...
  res = ns_execute (db, NULL, "delete MkWMJ9a");
  test_assert_int_equal (res, 0);

  // Close database
  test_assert_int_equal (ns_close (db), 0);
}
```

It's the shortest sequence that reproduces the bug. It starts from a clean
database, uses the public API, has a one-line comment on each step, and has a
`CAUSE:` on the step that used to fail. The file and test share a number.

A narrated unit test
--------------------

`csrc/core/ns_slab_alloc.c`, `slab_alloc_free_middle_slab`:

```c
  // Setup - three full slabs of two items: head -> slab3 -> slab2 -> slab1
  ...

  // Run - emptying slab2 unlinks it from between its neighbors
  slab_alloc_free (&alloc, s2[0]);
  slab_alloc_free (&alloc, s2[1]);

  // Verify others survive
  test_item_verify (s1[0], 10);
  ...

  // Teardown
  slab_alloc_free (&alloc, s1[0]);
  ...
```

The Setup comment describes the state being built, and the Run comment says
which behavior is being exercised. Use this shape for any test longer than a
screen.
