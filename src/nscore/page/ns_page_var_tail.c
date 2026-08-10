#include "nscore/page/ns_page_var_tail.h"

#include "core/ns_numerics.h"
#include "core/testing/ns_testing.h"

/******************************************************************************
 * SECTION: Var Tail
 ******************************************************************************/

DEFINE_DBG_ASSERT (page, vt_page, v, { ASSERT (v); })

////////////////////////////////////////////////////////////
// INITIALIZATION

#ifdef TESTING
TEST (vt_init_empty)
{
  page p;

  rand_bytes (p.raw, NS_PAGE_SIZE);
  page_init_empty (&p, PG_VAR_TAIL);

  test_assert_equal (vt_get_next (&p), PGNO_NULL);
}
#endif

////////////////////////////////////////////////////////////
// VALIDATION

err_t
vt_validate_for_db (const page *p, error *e)
{
  if (page_get_type (p) != PG_VAR_TAIL)
  {
    return error_causef (e, ERR_CORRUPT, "wrong page type for var tail node");
  }
  return SUCCESS;
}

#ifdef TESTING
TEST (vt_validate)
{
  page  sut;
  error e = error_create ();

  TEST_CASE ("Invalid page type")
  {
    page_init_empty (&sut, PG_DATA_LIST);
    test_err_t_check (vt_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }
}
#endif

////////////////////////////////////////////////////////////
// UTILS

void
i_log_vt (const int level, const page *vp)
{
  i_log (level, "=== VARIABLE TAIL START ===\n");

  i_log_printf (level, "PGNO:   %" PRpgno "\n", vp->pg);
  if (vt_get_next (vp) == PGNO_NULL)
  {
    i_log_printf (level, "NEXT:   NULL\n");
  }
  else
  {
    i_log_printf (level, "NEXT:   %" PRpgno "\n", vt_get_next (vp));
  }

  i_log (level, "=== VARIABLE TAIL END ===\n");
}

#ifdef TESTING
TEST (i_log_vt)
{
  page vt;
  vt.pg = 10;

  page_init_empty (&vt, PG_VAR_TAIL);

  vt_set_next (&vt, PGNO_NULL);
  i_log_vt (LOG_INFO, &vt);

  vt_set_next (&vt, 10);
  i_log_vt (LOG_INFO, &vt);
}
#endif
