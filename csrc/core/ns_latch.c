#include "core/ns_latch.h"

#include "core/ns_testing.h"

/******************************************************************************
 * SECTION: Latch (tests)
 ******************************************************************************/

#ifndef NDEBUG
struct data
{
  const u32 iters;
  int       value;
  latch     l;
};

static void *
data_thread (void *_data)
{
  struct data *d = _data;

  for (u32 i = 0; i < d->iters; ++i) {
    latch_lock (&d->l);
    d->value += 1;
    latch_unlock (&d->l);
  }

  return NULL;
}

TEST (latch)
{
  struct data d = {
      .iters = 1000,
      .value = 0,
      .l     = 0,
  };

  i_thread threads[10];

  for (u32 i = 0; i < 10; ++i) {
    ctx->os.table->thread_create (ctx->os.self, &threads[i], data_thread, &d, &ctx->e);
  }

  for (u32 i = 0; i < 10; ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  test_assert_int_equal (d.value, 10 * 1000);
}

/******************************************************************************
 * SECTION: Shared / Exclusive Latch (tests)
 ******************************************************************************/

enum
{
  // Bound every spin so a broken latch fails the test instead of hanging the
  // whole suite
  spx_spin_cap = 200000000,
};

struct spx_data
{
  const u32          iters;
  int                value;
  sx_latch           l;

  // Readers use these to prove they were all inside the latch together
  _Atomic (unsigned) arrived;
  unsigned           nreaders;
  _Atomic (unsigned) all_in;

  // Set by the writer once it has actually taken X
  _Atomic (unsigned) x_taken;
};

static void *
spx_x_thread (void *_d)
{
  struct spx_data *d = _d;

  for (u32 i = 0; i < d->iters; ++i) {
    spx_lock_x (&d->l);
    d->value += 1;
    spx_unlock_x (&d->l);
  }

  return NULL;
}

// Each reader parks inside the S latch until every other reader has arrived.
// If S were exclusive this would never complete, so reaching [nreaders] is the
// evidence that S is genuinely shared.
static void *
spx_s_thread (void *_d)
{
  struct spx_data *d = _d;

  spx_lock_s (&d->l);

  atomic_fetch_add (&d->arrived, 1);
  for (long spins = 0; spins < spx_spin_cap; ++spins) {
    if (atomic_load (&d->arrived) >= d->nreaders) {
      atomic_store (&d->all_in, 1);
      break;
    }
  }

  spx_unlock_s (&d->l);
  return NULL;
}

static void *
spx_x_waiter_thread (void *_d)
{
  struct spx_data *d = _d;

  spx_lock_x (&d->l);
  atomic_store (&d->x_taken, 1);
  spx_unlock_x (&d->l);

  return NULL;
}

TEST (spx_lock_s_and_x_states)
{
  sx_latch l;
  spx_latch_init (&l);

  TEST_CASE ("A fresh latch is neither shared nor exclusive")
  {
    test_assert (!SLOCKED (l));
    test_assert (!XLOCKED (l));
  }

  TEST_CASE ("Shared holders accumulate and release one at a time")
  {
    spx_lock_s (&l);
    test_assert_type_equal (SLOCKED (l), 1u, u32, PRIu32);
    test_assert (!XLOCKED (l));

    // A second shared holder is admitted rather than blocked
    spx_lock_s (&l);
    test_assert_type_equal (SLOCKED (l), 2u, u32, PRIu32);

    spx_unlock_s (&l);
    test_assert_type_equal (SLOCKED (l), 1u, u32, PRIu32);
    spx_unlock_s (&l);
    test_assert_type_equal (SLOCKED (l), 0u, u32, PRIu32);
  }

  TEST_CASE ("An exclusive try fails while a reader holds the latch")
  {
    spx_lock_s (&l);
    test_assert (!spx_trylock_x (&l));
    spx_unlock_s (&l);

    // ... and succeeds once the reader leaves
    test_assert (spx_trylock_x (&l));
    spx_unlock_x (&l);
  }

  TEST_CASE ("Exclusive excludes both readers and other writers")
  {
    spx_lock_x (&l);
    test_assert (XLOCKED (l));
    test_assert (!spx_trylock_s (&l));
    test_assert (!spx_trylock_x (&l));

    spx_unlock_x (&l);
    test_assert_type_equal ((u32)l, 0u, u32, PRIu32);
  }

  TEST_CASE ("A shared try succeeds on a free latch")
  {
    test_assert (spx_trylock_s (&l));
    test_assert_type_equal (SLOCKED (l), 1u, u32, PRIu32);
    spx_unlock_s (&l);
  }
}

// The counter is only correct if spx_lock_x really serialises the writers
TEST (spx_lock_x_serialises_writers)
{
  struct spx_data d = {
      .iters = 1000,
      .value = 0,
      .l     = 0,
  };

  i_thread threads[10];

  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_create (ctx->os.self, &threads[i], spx_x_thread, &d, &ctx->e);
  }
  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  test_assert_int_equal (d.value, (int)(arrlen (threads) * 1000));
  test_assert_type_equal ((u32)d.l, 0u, u32, PRIu32);
}

TEST (spx_lock_s_admits_readers_concurrently)
{
  struct spx_data d = {
      .iters    = 0,
      .value    = 0,
      .l        = 0,
      .nreaders = 8,
  };

  i_thread threads[8];

  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_create (ctx->os.self, &threads[i], spx_s_thread, &d, &ctx->e);
  }
  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  test_assert_type_equal (atomic_load (&d.all_in), 1u, u32, PRIu32);
  test_assert_type_equal (atomic_load (&d.arrived), 8u, u32, PRIu32);
  test_assert_type_equal ((u32)d.l, 0u, u32, PRIu32);
}

// Phase 2 of spx_lock_x: the X bit is claimed immediately, but the writer must
// not proceed until in-flight readers have drained
TEST (spx_lock_x_waits_for_readers_to_drain)
{
  struct spx_data d = {
      .iters = 0,
      .value = 0,
      .l     = 0,
  };

  spx_lock_s (&d.l);

  i_thread writer;
  ctx->os.table->thread_create (ctx->os.self, &writer, spx_x_waiter_thread, &d, &ctx->e);

  // The writer claims X quickly, but must still be stuck draining this reader
  for (long spins = 0; spins < 1000000; ++spins) {
    if (XLOCKED (atomic_load (&d.l))) {
      break;
    }
  }
  test_assert_type_equal (atomic_load (&d.x_taken), 0u, u32, PRIu32);

  // Releasing the reader lets the writer through
  spx_unlock_s (&d.l);
  ctx->os.table->thread_join (ctx->os.self, writer.self);

  test_assert_type_equal (atomic_load (&d.x_taken), 1u, u32, PRIu32);
  test_assert_type_equal ((u32)d.l, 0u, u32, PRIu32);
}
#endif
