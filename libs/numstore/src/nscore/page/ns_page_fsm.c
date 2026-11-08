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

#include "nscore/page/ns_page_fsm.h"

#include "core/ns_testing.h"

#include <string.h>

/******************************************************************************
 * SECTION: Free Space Map
 ******************************************************************************/

void
fsm_init_empty (page *in)
{
  DBG_ASSERT (page_base, in);
  ASSERT (page_get_type (in) == PG_FREE_SPACE_MAP);
  memset (fsm_get_bitmap_mut (in), 0, FS_BTMP_SIZE);
  fsm_set_bit (in, 0); // First bit is always set (that's me)
}

err_t
fsm_validate_for_db (const page *hl, error *e)
{
  DBG_ASSERT (page_base, hl);

  const pgh header = page_get_type (hl);

  if (header != (pgh)PG_FREE_SPACE_MAP) {
    return error_causef (
        e,
        ERR_CORRUPT,
        "expected header: %" PRpgh " but got: %" PRpgh,
        (pgh)PG_FREE_SPACE_MAP,
        header
    );
  }

  return SUCCESS;
}

void
i_log_fsm (const int level, const page *t)
{
  DBG_ASSERT (page_base, t);
  i_log (level, "=== FREE SPACE PAGE START ===\n");
  for (p_size i = 0; i < FS_BTMP_NPGS; ++i) {
    if (fsm_get_bit (t, i)) {
      i_log_printf (level, "|%" PRp_size "| -- Occupied\n", i);
    }
  }
  i_log (level, "=== FREE SPACE PAGE END ===\n");
}

#ifndef NDEBUG
TEST (i_log_fsm)
{
  page fsm;
  fsm.pg = 10;

  page_init_empty (&fsm, PG_FREE_SPACE_MAP);
  fsm_set_bit (&fsm, 10);
  i_log_fsm (LOG_INFO, &fsm);
}

TEST (fsm_validate_for_db)
{
  page pg;

  TEST_CASE ("A free space map page is valid")
  {
    page_init_empty (&pg, PG_FREE_SPACE_MAP);
    test_err_t_check (fsm_validate_for_db (&pg, &ctx->e), SUCCESS, &ctx->e);
  }

  TEST_CASE ("Any other page type is corrupt")
  {
    page_init_empty (&pg, PG_DATA_LIST);
    test_err_t_check (fsm_validate_for_db (&pg, &ctx->e), ERR_CORRUPT, &ctx->e);
  }
}

TEST (fsm_next_freebit)
{
  page pg;
  page_init_empty (&pg, PG_FREE_SPACE_MAP);

  TEST_CASE ("Bit 0 is the map itself, so the first free bit is 1")
  {
    test_assert_int_equal (fsm_next_freebit (&pg, 0), 1);
  }

  TEST_CASE ("Set bits are skipped, including within the start byte")
  {
    fsm_set_bit (&pg, 1);
    fsm_set_bit (&pg, 2);
    test_assert_int_equal (fsm_next_freebit (&pg, 0), 3);
    test_assert_int_equal (fsm_next_freebit (&pg, 5), 5);
    fsm_clr_bit (&pg, 1);
    test_assert_int_equal (fsm_get_bit (&pg, 1), 0);
    test_assert_int_equal (fsm_next_freebit (&pg, 0), 1);
  }

  TEST_CASE ("A full map has no free bit")
  {
    memset (fsm_get_bitmap_mut (&pg), 0xFF, FS_BTMP_SIZE);
    test_assert_int_equal (fsm_next_freebit (&pg, 0), -1);
  }

  TEST_CASE ("Free bits before the start bit in the start byte are ignored")
  {
    memset (fsm_get_bitmap_mut (&pg), 0xFF, FS_BTMP_SIZE);
    fsm_clr_bit (&pg, 8);
    fsm_clr_bit (&pg, 17);
    test_assert_int_equal (fsm_next_freebit (&pg, 9), 17);
  }
}
#endif
