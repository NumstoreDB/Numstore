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
  error       e = error_create ();

  struct data d = {
      .iters = 1000,
      .value = 0,
      .l     = 0,
  };

  i_thread threads[10];

  for (u32 i = 0; i < 10; ++i) {
    os.table->thread_create (os.self, &threads[i], data_thread, &d, &e);
  }

  for (u32 i = 0; i < 10; ++i) {
    os.table->thread_join (os.self, threads[i].self);
  }

  test_assert_int_equal (d.value, 10 * 1000);
}
#endif
