/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#include "c_specx/concurrency/latch.h"
#include "c_specx/concurrency/spx_latch.h"
#include "compile_config.h"
#include "pager.h"
#include "pager/page_fixture.h"

static inline u32
pgr_spin_clock (struct pager *p)
{
  ASSERT (MEMORY_PAGE_LEN % 2 == 0); // For overflow and faster modulo
  return atomic_fetch_add_explicit (&p->clock, 1, memory_order_relaxed) & (MEMORY_PAGE_LEN - 1);
}

i32
pgr_reserve_and_ctrl_lock (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  struct page_frame *mp = NULL; // The working page frame
  u32 clock = pgr_spin_clock (p);
  bool ready_to_evict = false; // First round - don't evict any pages

  /**
   * Loop forever - this is highly concurrent
   */
  for (;; clock = pgr_spin_clock (p))
    {
      mp = &p->pages[clock];

      if (!latch_trylock (&mp->ctrl))
        {
          // If we can't lock - don't spin - just move on and find a new slot
          continue;
        }

      // Found an empty spot
      if (!(mp->flags & PW_PRESENT))
        {
          goto found_spot;
        }

      // Pinned, skip it
      if (mp->pin > 0)
        {
          latch_unlock (&mp->ctrl);
          continue;
        }

      // Access bit is on - set off and continue
      if (mp->flags & PW_ACCESS)
        {
          mp->flags &= ~PW_ACCESS;

          latch_unlock (&mp->ctrl);
          continue;
        }

      if (ready_to_evict)
        {
          // Found a spot - but it's not being used - safe to evict it
          if (pgr_evict_unsafe (p, mp, e) < 0)
            {
              return error_trace (e);
            }

          goto found_spot;
        }
      else
        {
          // The first round - don't evict anything
          latch_unlock (&mp->ctrl);
          ready_to_evict = true;
        }
    }

found_spot:
  return clock;
}

#ifndef NTEST
TEST (pgr_reserve_and_ctrl_lock_st)
{
  TEST_CASE ("single threaded clock iterator test")
  {
    struct pgr_fixture pgr;
    pgr_fixture_create (&pgr);

    i32 clock1 = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    i32 clock2 = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    i32 clock3 = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;

    test_assert_int_equal (clock1, 0);
    test_assert_int_equal (clock2, 1);
    test_assert_int_equal (clock3, 2);

    latch_unlock (&pgr.p->pages[0].ctrl);
    latch_unlock (&pgr.p->pages[1].ctrl);
    latch_unlock (&pgr.p->pages[2].ctrl);

    // If I don't do anything and just unlock - it should repeat
    clock1 = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    clock2 = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    clock3 = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;

    test_assert_int_equal (clock1, 0);
    test_assert_int_equal (clock2, 1);
    test_assert_int_equal (clock3, 2);

    latch_unlock (&pgr.p->pages[0].ctrl);
    latch_unlock (&pgr.p->pages[1].ctrl);
    latch_unlock (&pgr.p->pages[2].ctrl);

    pgr_fixture_teardown (&pgr);
  }
}
#endif
