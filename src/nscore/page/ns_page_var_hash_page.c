
/******************************************************************************
 * SECTION: Var Hash Page
 ******************************************************************************/

DEFINE_DBG_ASSERT (page, vh_page, d, { ASSERT (d); })

void
vh_init_empty (page *p)
{
  ASSERT (page_get_type (p) == PG_VAR_HASH_PAGE);
  for (p_size i = 0; i < VH_HASH_LEN; ++i)
  {
    vh_set_hash_value (p, i, PGNO_NULL);
  }
}

// Validation
err_t
vh_validate_for_db (const page *p, error *e)
{
  DBG_ASSERT (vh_page, p);

  if (page_get_type (p) != PG_VAR_HASH_PAGE)
  {
    return error_causef (e, ERR_CORRUPT, "wrong page type for var hash page");
  }

  return SUCCESS;
}

// Utils
void
i_log_vh (const int level, const page *vh)
{
  i_log (level, "=== VAR HASH TABLE PAGE START ===\n");

  bool empty = true;

  for (p_size i = 0; i < VH_HASH_LEN; ++i)
  {
    const pgno p = vh_get_hash_value (vh, i);
    if (p != PGNO_NULL)
    {
      empty = false;
      i_log_printf (level, "[%" PRp_size "]: %" PRpgno "\n", i, p);
    }
  }

  if (empty)
  {
    i_log_printf (level, "Empty\n");
  }

  i_log (level, "=== VAR HASH TABLE PAGE END ===\n");
}

#ifdef TESTING
TEST (i_log_vh)
{
  page vh;
  vh.pg = 10;

  page_init_empty (&vh, PG_VAR_HASH_PAGE);
  i_log_vh (LOG_INFO, &vh);

  vh_set_hash_value (&vh, 10, 10);
  vh_set_hash_value (&vh, 12, 10);
  i_log_vh (LOG_INFO, &vh);
}
#endif
