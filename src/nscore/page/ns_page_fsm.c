#include "nscore/page/ns_page_fsm.h"

#include <string.h>

#include "core/testing/ns_testing.h"

/******************************************************************************
 * SECTION: Free Space Map
 ******************************************************************************/

void
fsm_init_empty (page *in)
{
  ASSERT (page_get_type (in) == PG_FREE_SPACE_MAP);
  memset (fsm_get_bitmap_mut (in), 0, FS_BTMP_SIZE);
  fsm_set_bit (in, 0); // First bit is always set (that's me)
}

err_t
fsm_validate_for_db (const page *hl, error *e)
{
  const pgh header = page_get_type (hl);

  if (header != (pgh)PG_FREE_SPACE_MAP)
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "expected header: %" PRpgh " but got: %" PRpgh,
        (pgh)PG_FREE_SPACE_MAP,
        (pgh)header
    );
  }

  return SUCCESS;
}

// Utils
void
i_log_fsm (const int level, const page *t)
{
  i_log (level, "=== FREE SPACE PAGE START ===\n");
  for (p_size i = 0; i < FS_BTMP_NPGS; ++i)
  {
    if (fsm_get_bit (t, i))
    {
      i_log_printf (level, "|%" PRp_size "| -- Occupied\n", i);
    }
  }
  i_log (level, "=== FREE SPACE PAGE END ===\n");
}

#ifdef TESTING
TEST (i_log_fsm)
{
  page fsm;
  fsm.pg = 10;

  page_init_empty (&fsm, PG_FREE_SPACE_MAP);
  fsm_set_bit (&fsm, 10);
  i_log_fsm (LOG_INFO, &fsm);
}
#endif
