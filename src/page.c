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

#include "page.h"

#include "testing/inner_node_testing.h"
#include "testing/testing.h"

/******************************************************************************
 * SECTION: Page Common
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

void
page_init_empty (page *p, const enum page_type type)
{
  page_set_type (p, type);
  page_set_page_lsn (p, PGNO_NULL);
  page_set_checksum (p, 0);

  switch (type)
  {
    case PG_DATA_LIST:
    {
      dl_init_empty (p);
      return;
    }
    case PG_INNER_NODE:
    {
      in_init_empty (p);
      return;
    }
    case PG_FREE_SPACE_MAP:
    {
      fsm_init_empty (p);
      return;
    }
    case PG_VAR_PAGE:
    {
      vp_init_empty (p);
      return;
    }
    case PG_VAR_TAIL:
    {
      vt_init_empty (p);
      return;
    }
    case PG_VAR_HASH_PAGE:
    {
      vh_init_empty (p);
      return;
    }
  }
  UNREACHABLE ();
}

err_t
page_validate_for_db (const page *p, const int flags, error *e)
{
  ASSERT (p);

  if (flags & PG_PERMISSIVE)
  {
    return SUCCESS;
  }

  const pgh header = page_get_type (p);

  if (!(header & flags))
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "expected page type %d, got %d",
        flags,
        header
    );
  }

  if (!(flags & PG_SKIP_CHECKSUM))
  {
    const u32 actual_checksum   = page_get_checksum (p);
    const u32 expected_checksum = page_compute_checksum (p);

    if (actual_checksum != expected_checksum)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "Corrupt page. Expected checksum: "
          "%d Actual Checksum: %d",
          expected_checksum,
          actual_checksum
      );
    }
  }

  if (header == PG_TRASH)
  {
    return SUCCESS;
  }

  switch ((enum page_type)header)
  {
    case PG_DATA_LIST:
    {
      return dl_validate_for_db (p, e);
    }
    case PG_INNER_NODE:
    {
      return in_validate_for_db (p, e);
    }
    case PG_FREE_SPACE_MAP:
    {
      return fsm_validate_for_db (p, e);
    }
    case PG_VAR_PAGE:
    {
      return vp_validate_for_db (p, e);
    }
    case PG_VAR_TAIL:
    {
      return vt_validate_for_db (p, e);
    }
    case PG_VAR_HASH_PAGE:
    {
      return vh_validate_for_db (p, e);
    }
  }

  UNREACHABLE ();
}

////////////////////////////////////////////////////////////
// SETTERS

#ifndef NTEST
TEST (page_set_get_simple)
{
  page p, q;
  rand_bytes (p.raw, NS_PAGE_SIZE);
  rand_bytes (q.raw, NS_PAGE_SIZE);

  TEST_CASE ("Initialization: each page type")
  {
    page_init_empty (&p, PG_DATA_LIST);
    test_assert_int_equal (page_get_type (&p), PG_DATA_LIST);

    page_init_empty (&p, PG_INNER_NODE);
    test_assert_int_equal (page_get_type (&p), PG_INNER_NODE);

    page_init_empty (&p, PG_FREE_SPACE_MAP);
    test_assert_int_equal (page_get_type (&p), PG_FREE_SPACE_MAP);
  }

  TEST_CASE ("Setters / Getters: checksum + page_lsn + type")
  {
    page_init_empty (&p, PG_DATA_LIST);

    page_set_checksum (&p, 0xDEADBEEF);
    test_assert_int_equal (page_get_checksum (&p), 0xDEADBEEF);

    page_set_page_lsn (&p, 1234);
    test_assert_int_equal (page_get_page_lsn (&p), 1234);

    page_set_type (&p, PG_INNER_NODE);
    test_assert_int_equal (page_get_type (&p), PG_INNER_NODE);
  }

  TEST_CASE ("page_memcpy roundtrip")
  {
    const struct bytes src = {.head = q.raw, .len = NS_PAGE_SIZE};
    page_memcpy (&p, src);
    test_assert_memequal (p.raw, q.raw, NS_PAGE_SIZE);
  }
}
#endif

void
i_log_page (const int log_level, const page *p)
{
  switch ((enum page_type)page_get_type (p))
  {
    case PG_DATA_LIST:
    {
      i_log_dl (log_level, p);
      return;
    }
    case PG_INNER_NODE:
    {
      i_log_in (log_level, p);
      return;
    }
    case PG_FREE_SPACE_MAP:
    {
      i_log_fsm (log_level, p);
      return;
    }
    case PG_VAR_PAGE:
    {
      i_log_vp (log_level, p);
      return;
    }
    case PG_VAR_TAIL:
    {
      i_log_vt (log_level, p);
      return;
    }
    case PG_VAR_HASH_PAGE:
    {
      i_log_vh (log_level, p);
      return;
    }
  }
}

#ifndef NTEST
TEST (i_log_page)
{
  page pg;
  pg.pg = 10;

  page_init_empty (&pg, PG_DATA_LIST);
  i_log_page (LOG_INFO, &pg);

  page_init_empty (&pg, PG_INNER_NODE);
  i_log_page (LOG_INFO, &pg);

  page_init_empty (&pg, PG_FREE_SPACE_MAP);
  i_log_page (LOG_INFO, &pg);

  page_init_empty (&pg, PG_VAR_PAGE);
  i_log_page (LOG_INFO, &pg);

  page_init_empty (&pg, PG_VAR_TAIL);
  i_log_page (LOG_INFO, &pg);

  page_init_empty (&pg, PG_VAR_HASH_PAGE);
  i_log_page (LOG_INFO, &pg);
}
#endif

/******************************************************************************
 * SECTION: Free Space Map
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
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
      i_printf (level, "|%" PRp_size "| -- Occupied\n", i);
    }
  }
  i_log (level, "=== FREE SPACE PAGE END ===\n");
}

#ifndef NTEST
TEST (i_log_fsm)
{
  page fsm;
  fsm.pg = 10;

  page_init_empty (&fsm, PG_FREE_SPACE_MAP);
  fsm_set_bit (&fsm, 10);
  i_log_fsm (LOG_INFO, &fsm);
}
#endif

/******************************************************************************
 * SECTION: Data List
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

err_t
dl_validate_for_db (const page *d, error *e)
{
  DBG_ASSERT (data_list, d);

  const enum page_type type = page_get_type (d);

  if (type != (u8)PG_DATA_LIST)
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "expected header: %" PRpgh " but got: %" PRpgh,
        (pgh)PG_DATA_LIST,
        (pgh)type
    );
  }

  const p_size used = dl_used (d);

  if (used > DL_DATA_SIZE)
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "used %" PRp_size " > max %" PRp_size " (page_size=%" PRp_size ")",
        used,
        DL_DATA_SIZE,
        NS_PAGE_SIZE
    );
  }

  // Root check
  if (dl_is_root (d))
  {
    if (dl_used (d) == 0)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "Root node must have at least 1 "
          "element"
      );
    }
  }
  else
  {
    if (dl_used (d) < DL_DATA_SIZE / 2)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "non-root data list node below "
          "half capacity"
      );
    }
  }

  return SUCCESS;
}

#ifndef NTEST
TEST (dl_validate)
{
  page  sut;
  error e = error_create ();

  // Initialized page is base valid
  TEST_CASE ("Initialized page base - no changes is valid")
  {
    page_init_empty (&sut, PG_DATA_LIST);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }

  // Corrupt page header
  TEST_CASE ("Corrupt page header")
  {
    page_init_empty (&sut, PG_DATA_LIST);
    page_set_type (&sut, PG_INNER_NODE);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }

  TEST_CASE ("Root page size checks")
  {
    page_init_empty (&sut, PG_DATA_LIST);

    // Hack the page size
    // Can't do this b/c of ASSERT - but it's still possible on
    // corruption dl_set_used(&sut, DL_DATA_SIZE + 1);
    p_size size = DL_DATA_SIZE + 1;
    PAGE_SIMPLE_SET_IMPL (&sut, size, DL_BLEN_OFST);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);

    // Max size is valid
    dl_set_used (&sut, DL_DATA_SIZE);
    test_err_t_check (dl_validate_for_db (&sut, &e), SUCCESS, &e);

    // 0 size is invalid
    dl_set_used (&sut, 0);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);

    // in between size is valid
    dl_set_used (&sut, DL_DATA_SIZE / 2);
    test_err_t_check (dl_validate_for_db (&sut, &e), SUCCESS, &e);

    // in between size is valid for root
    dl_set_used (&sut, DL_DATA_SIZE / 2 - 1);
    test_err_t_check (dl_validate_for_db (&sut, &e), SUCCESS, &e);
  }

  TEST_CASE ("Non root page size checks")
  {
    page_init_empty (&sut, PG_DATA_LIST);
    dl_set_next (&sut, 5);

    // Hack the page size
    // Can't do this b/c of ASSERT - but it's still possible on
    // corruption dl_set_used(&sut, DL_DATA_SIZE + 1);
    p_size size = DL_DATA_SIZE + 1;
    PAGE_SIMPLE_SET_IMPL (&sut, size, DL_BLEN_OFST);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);

    // Max size is valid
    dl_set_used (&sut, DL_DATA_SIZE);
    test_err_t_check (dl_validate_for_db (&sut, &e), SUCCESS, &e);

    // 0 size is invalid
    dl_set_used (&sut, 0);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);

    // in between size is valid
    dl_set_used (&sut, DL_DATA_SIZE / 2);
    test_err_t_check (dl_validate_for_db (&sut, &e), SUCCESS, &e);

    // in between size is valid for root
    dl_set_used (&sut, DL_DATA_SIZE / 2 - 1);
    test_err_t_check (dl_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }
}
#endif

// Getters

#ifndef NTEST
TEST (dl_set_get)
{
  page p;
  rand_bytes (p.raw, NS_PAGE_SIZE);

  // Randomize array to pretend random

  // Start
  TEST_CASE ("Initial page values")
  {
    page_init_empty (&p, PG_DATA_LIST);
    test_assert_type_equal (dl_get_next (&p), PGNO_NULL, pgno, PRpgno);
    test_assert_type_equal (dl_get_prev (&p), PGNO_NULL, pgno, PRpgno);
    test_assert_int_equal (dl_used (&p), 0);
    test_assert_int_equal (dl_avail (&p), DL_DATA_SIZE);
  }

  // Set / Get
  TEST_CASE ("Getters / Setters work")
  {
    page_init_empty (&p, PG_DATA_LIST);
    dl_set_next (&p, 11);
    dl_set_prev (&p, 12);
    dl_set_used (&p, 13);
    dl_set_byte (&p, 10, 5);

    test_assert_type_equal (dl_get_next (&p), 11, pgno, PRpgno);
    test_assert_type_equal (dl_get_prev (&p), 12, pgno, PRpgno);
    test_assert_int_equal (dl_used (&p), 13);
    test_assert_int_equal (dl_avail (&p), DL_DATA_SIZE - 13);
    test_assert_int_equal (dl_get_byte (&p, 10), 5);
  }
}
#endif

p_size
dl_read (const page *d, u8 *dest, const p_size offset, const p_size nbytes)
{
  DBG_ASSERT (data_list, d);
  ASSERT (nbytes > 0);

  const p_size dlen = dl_used (d);
  const u8    *base = dl_get_data (d);

  ASSERT (offset <= dlen);

  if (offset == dlen)
  {
    return 0;
  }

  const p_size avail  = dlen - offset;
  const p_size toread = MIN (avail, nbytes);

  if (toread > 0 && dest)
  {
    memcpy (dest, base + offset, toread);
  }

  return toread;
}

#ifndef NTEST
TEST (dl_read)
{
  page p;
  u8   buf[DL_DATA_SIZE];
  u8   src[DL_DATA_SIZE];

  TEST_CASE ("Read on an empty page")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    test_assert_int_equal (dl_read (&p, buf, 0, 1), 0);
    test_assert_int_equal (dl_read (&p, buf, 0, DL_DATA_SIZE), 0);
  }

  TEST_CASE ("Random data")
  {
    rand_bytes (src, DL_DATA_SIZE);

    page_init_empty (&p, PG_DATA_LIST);
    memcpy (dl_get_data (&p), src, DL_DATA_SIZE);
    dl_set_used (&p, DL_DATA_SIZE);
  }
  TEST_CASE ("full read")
  {
    memset (buf, 0, sizeof buf);
    const p_size got = dl_read (&p, buf, 0, DL_DATA_SIZE);
    test_assert_int_equal (got, DL_DATA_SIZE);
    test_assert_memequal (buf, src, DL_DATA_SIZE);
  }

  TEST_CASE ("partial read (first 16 bytes)")
  {
    memset (buf, 0, sizeof buf);
    const p_size got = dl_read (&p, buf, 0, 16);
    test_assert_int_equal (got, 16);
    test_assert_memequal (buf, src, 16);
  }

  TEST_CASE ("offset into middle")
  {
    const p_size off = 10;
    const p_size n   = 20;
    memset (buf, 0, sizeof buf);
    const p_size got = dl_read (&p, buf, off, n);
    test_assert_int_equal (got, n);
    test_assert_memequal (buf, src + off, n);
  }

  TEST_CASE ("request past end")
  {
    const p_size off = DL_DATA_SIZE - 8;
    memset (buf, 0, sizeof buf);
    const p_size got = dl_read (&p, buf, off, 32);
    test_assert_int_equal (got, 8);
    test_assert_memequal (buf, src + off, 8);
  }

  TEST_CASE ("offset == used (should return 0)")
  {
    const p_size off = DL_DATA_SIZE;
    const p_size got = dl_read (&p, buf, off, 1);
    test_assert_int_equal (got, 0);
  }
}
#endif

p_size
dl_read_into_cbuffer (
    const page     *d,
    struct cbuffer *dest,
    const p_size    offset,
    const p_size    b
)
{
  ASSERT (b > 0);

  const p_size dlen = dl_used (d);
  const u8    *base = dl_get_data (d);

  ASSERT (offset <= dlen);

  if (offset == dlen)
  {
    return 0;
  }

  const p_size avail  = dlen - offset;
  p_size       toread = MIN (avail, b);

  if (toread > 0 && dest)
  {
    toread = cbuffer_write (base + offset, 1, toread, dest);
  }

  return toread;
}

p_size
dl_read_out_into_cbuffer (
    page           *d,
    struct cbuffer *dest,
    const p_size    offset,
    const p_size    b
)
{
  ASSERT (b > 0);

  const p_size dlen = dl_used (d);
  u8          *base = dl_get_data (d);

  ASSERT (offset <= dlen);

  if (offset == dlen)
  {
    return 0;
  }

  const p_size avail  = dlen - offset;
  p_size       toread = MIN (avail, b);

  if (toread > 0 && dest)
  {
    toread = cbuffer_write (base + offset, 1, toread, dest);
  }

  // [ ++++++++++ __________ ++++++ ]
  //            ^           ^
  //            ofst     ofst + toread
  //                        [       ] = dlen - (ofst + toread)
  const p_size remain = dlen - toread - offset;
  memmove (base + offset, base + offset + toread, remain);
  dl_set_used (d, offset + remain);

  return toread;
}

void
dl_read_expect (
    const page  *d,
    u8          *dest,
    const p_size offset,
    const p_size nbytes
)
{
  const p_size read = dl_read (d, dest, offset, nbytes);
  ASSERT (read == nbytes);
}

p_size
dl_read_out_from (page *d, u8 *dest, const p_size offset)
{
  DBG_ASSERT (data_list, d);
  ASSERT (dest);

  p_size dlen = dl_used (d);

  ASSERT (offset <= dlen);

  if (offset == dlen)
  {
    return 0;
  }

  const u8 *head = dl_get_data (d);

  head += offset;
  dlen -= offset;

  if (dlen > 0)
  {
    memcpy (dest, head, dlen);
    dl_set_used (d, dl_used (d) - dlen);
  }

  return dlen;
}

#ifndef NTEST
TEST (dl_read_out_from)
{
  page dl;
  page_init_empty (&dl, PG_DATA_LIST);

  u8 dest[DL_DATA_SIZE];
  u8 alldata[DL_DATA_SIZE];
  u8 somedata[DL_DATA_SIZE / 2];
  arr_range (alldata);
  arr_range (somedata);

  TEST_CASE ("Empty read")
  {
    test_assert_int_equal (dl_used (&dl), 0);
    p_size ret = dl_read_out_from (&dl, dest, 0);
    test_assert_int_equal (ret, 0);
    test_assert_int_equal (dl_used (&dl), 0);
  }

  TEST_CASE ("Read all from 0")
  {
    dl_append (&dl, somedata, DL_DATA_SIZE / 2);
    p_size ret = dl_read_out_from (&dl, dest, 0);
    test_assert_int_equal (ret, DL_DATA_SIZE / 2);
    test_assert_int_equal (dl_used (&dl), 0);

    for (p_size i = 0; i < DL_DATA_SIZE / 2; ++i)
    {
      test_assert_int_equal (dest[i], somedata[i]);
    }
    dl_set_used (&dl, 0);
  }

  TEST_CASE ("Read some from middle")
  {
    _Static_assert (
        DL_DATA_SIZE / 2 > 1,
        "DL_DATA_SIZE is too small. Increase page size"
    );

    dl_append (&dl, somedata, DL_DATA_SIZE / 2);
    p_size ret = dl_read_out_from (&dl, dest, 1);
    test_assert_int_equal (ret, DL_DATA_SIZE / 2 - 1);
    test_assert_int_equal (dl_used (&dl), 1);

    for (p_size i = 0; i < DL_DATA_SIZE / 2 - 1; ++i)
    {
      test_assert_int_equal (dest[i], somedata[i + 1]);
    }
    for (p_size i = 0; i < 1; ++i)
    {
      test_assert_int_equal (dl_get_byte (&dl, i), i);
    }
    dl_set_used (&dl, 0);
  }

  TEST_CASE ("Read some later in the middle")
  {
    _Static_assert (
        DL_DATA_SIZE / 2 > 10,
        "DL_DATA_SIZE is too small. Increase page size"
    );

    dl_append (&dl, somedata, DL_DATA_SIZE / 2);
    p_size ret = dl_read_out_from (&dl, dest, 10);
    test_assert_int_equal (ret, DL_DATA_SIZE / 2 - 10);
    test_assert_int_equal (dl_used (&dl), 10);

    for (p_size i = 0; i < DL_DATA_SIZE / 2 - 10; ++i)
    {
      test_assert_int_equal (dest[i], somedata[i + 10]);
    }
    for (p_size i = 0; i < 10; ++i)
    {
      test_assert_int_equal (dl_get_byte (&dl, i), i);
    }
    dl_set_used (&dl, 0);
  }

  TEST_CASE ("Read some from the end")
  {
    _Static_assert (
        DL_DATA_SIZE / 2 > 10,
        "DL_DATA_SIZE is too small. Increase page size"
    );

    dl_append (&dl, somedata, DL_DATA_SIZE / 2);
    p_size ret = dl_read_out_from (&dl, dest, DL_DATA_SIZE / 2);
    test_assert_int_equal (ret, 0);
    test_assert_int_equal (dl_used (&dl), DL_DATA_SIZE / 2);

    for (p_size i = 0; i < DL_DATA_SIZE / 2; ++i)
    {
      test_assert_int_equal (dl_get_byte (&dl, i), somedata[i]);
    }
    dl_set_used (&dl, 0);
  }

  TEST_CASE ("Read full middle")
  {
    _Static_assert (
        DL_DATA_SIZE > 1,
        "DL_DATA_SIZE is too small. Increase page size"
    );

    dl_append (&dl, alldata, DL_DATA_SIZE);
    p_size ret = dl_read_out_from (&dl, dest, 1);
    test_assert_int_equal (ret, DL_DATA_SIZE - 1);
    test_assert_int_equal (dl_used (&dl), 1);

    for (p_size i = 0; i < DL_DATA_SIZE - 1; ++i)
    {
      test_assert_int_equal (dest[i], alldata[i + 1]);
    }
    for (p_size i = 0; i < 1; ++i)
    {
      test_assert_int_equal (dl_get_byte (&dl, i), alldata[i]);
    }
    dl_set_used (&dl, 0);
  }

  TEST_CASE ("Read full later middle")
  {
    _Static_assert (
        DL_DATA_SIZE > 10,
        "DL_DATA_SIZE is too small. Increase page size"
    );

    dl_append (&dl, alldata, DL_DATA_SIZE);
    p_size ret = dl_read_out_from (&dl, dest, 10);
    test_assert_int_equal (ret, DL_DATA_SIZE - 10);
    test_assert_int_equal (dl_used (&dl), 10);

    for (p_size i = 0; i < DL_DATA_SIZE - 10; ++i)
    {
      test_assert_int_equal (dest[i], alldata[i + 10]);
    }
    for (p_size i = 0; i < 10; ++i)
    {
      test_assert_int_equal (dl_get_byte (&dl, i), alldata[i]);
    }
    dl_set_used (&dl, 0);
  }

  TEST_CASE ("Read full end")
  {
    dl_append (&dl, alldata, DL_DATA_SIZE);
    p_size ret = dl_read_out_from (&dl, dest, DL_DATA_SIZE);
    test_assert_int_equal (ret, 0);
    test_assert_int_equal (dl_used (&dl), DL_DATA_SIZE);

    for (p_size i = 0; i < DL_DATA_SIZE; ++i)
    {
      test_assert_int_equal (dl_get_byte (&dl, i), alldata[i]);
    }
    dl_set_used (&dl, 0);
  }
}
#endif

p_size
dl_append (page *d, const u8 *src, const p_size nbytes)
{
  DBG_ASSERT (data_list, d);
  ASSERT (src);
  ASSERT (nbytes > 0);

  const p_size avail = DL_DATA_SIZE - dl_used (d);
  const p_size next  = MIN (avail, nbytes);

  u8 *data = dl_get_data (d);

  if (next > 0)
  {
    u8 *tail = data + dl_used (d);

    memcpy (tail, src, next);

    dl_set_used (d, dl_used (d) + next);
  }

  DBG_ASSERT (data_list, d);
  return next;
}

void
dl_append_from_cbuffer (page *d, struct cbuffer *src, const p_size amnt)
{
  DBG_ASSERT (data_list, d);
  ASSERT (amnt > 0);
  ASSERT (src);
  ASSERT (amnt <= DL_DATA_SIZE - dl_used (d));
  ASSERT (amnt <= cbuffer_len (src));

  u8 *data = dl_get_data (d);
  u8 *tail = data + dl_used (d);
  cbuffer_read_expect (tail, 1, amnt, src);
  dl_set_used (d, dl_used (d) + amnt);

  DBG_ASSERT (data_list, d);
}

#ifndef NTEST
TEST (dl_append)
{
  page p;
  u8   src[DL_DATA_SIZE];

  rand_bytes (src, sizeof src);

  TEST_CASE ("empty page, append small data")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    const p_size n   = 16;
    const p_size got = dl_append (&p, src, n);
    test_assert_int_equal (got, n);
    test_assert_int_equal (dl_used (&p), n);
    test_assert_memequal (dl_get_data (&p), src, n);
  }

  TEST_CASE ("multiple appends accumulate")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    const p_size n1 = 8;
    const p_size n2 = 12;

    const p_size got1 = dl_append (&p, src, n1);
    const p_size got2 = dl_append (&p, src + n1, n2);

    test_assert_int_equal (got1, n1);
    test_assert_int_equal (got2, n2);
    test_assert_int_equal (dl_used (&p), n1 + n2);
    test_assert_memequal (dl_get_data (&p), src, n1 + n2);
  }

  TEST_CASE ("append more than available → clipped")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    const p_size big = DL_DATA_SIZE + 100;
    const p_size got = dl_append (&p, src, big);

    test_assert_int_equal (got, DL_DATA_SIZE);
    test_assert_int_equal (dl_used (&p), DL_DATA_SIZE);
    test_assert_memequal (dl_get_data (&p), src, DL_DATA_SIZE);
  }

  TEST_CASE ("no space left → append returns 0")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    // fill page fully
    dl_set_used (&p, DL_DATA_SIZE);

    const p_size got = dl_append (&p, src, 32);
    test_assert_int_equal (got, 0);
    test_assert_int_equal (dl_used (&p), DL_DATA_SIZE);
  }
}
#endif

p_size
dl_write (
    const page  *d,
    const u8    *src,
    const p_size offset,
    const p_size nbytes
)
{
  DBG_ASSERT (data_list, d);
  ASSERT (nbytes > 0);

  const p_size dlen = dl_used (d);
  u8          *base = dl_get_data (d);

  ASSERT (offset <= dlen);

  if (offset == dlen)
  {
    return 0;
  }

  const p_size avail  = dlen - offset;
  const p_size toread = MIN (avail, nbytes);

  if (toread > 0 && src)
  {
    memcpy (base + offset, src, toread);
  }

  DBG_ASSERT (data_list, d);
  return toread;
}

#ifndef NTEST
TEST (dl_write)
{
  page p;
  u8   src[DL_DATA_SIZE];

  rand_bytes (src, sizeof src);

  // Fill page fully with known data
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    memcpy (dl_get_data (&p), src, DL_DATA_SIZE);
    dl_set_used (&p, DL_DATA_SIZE);
  }

  TEST_CASE ("overwrite beginning")
  {
    u8 newdata[8];
    rand_bytes (newdata, sizeof newdata);

    const p_size got = dl_write (&p, newdata, 0, sizeof newdata);
    test_assert_int_equal (got, sizeof newdata);
    test_assert_memequal (dl_get_data (&p), newdata, sizeof newdata);
    test_assert_memequal (
        (u8 *)dl_get_data (&p) + sizeof newdata,
        src + sizeof newdata,
        DL_DATA_SIZE - sizeof newdata
    );
  }

  TEST_CASE ("overwrite middle")
  {
    u8 newdata[16];
    rand_bytes (newdata, sizeof newdata);

    const p_size off = 32;
    const p_size got = dl_write (&p, newdata, off, sizeof newdata);
    test_assert_int_equal (got, sizeof newdata);
    test_assert_memequal (
        (u8 *)dl_get_data (&p) + off,
        newdata,
        sizeof newdata
    );
  }

  TEST_CASE ("overwrite near end, clipped")
  {
    u8 newdata[32];
    rand_bytes (newdata, sizeof newdata);

    const p_size off = DL_DATA_SIZE - 10;
    const p_size got = dl_write (&p, newdata, off, sizeof newdata);
    test_assert_int_equal (got, 10); // only 10 bytes available
    test_assert_memequal ((u8 *)dl_get_data (&p) + off, newdata, got);
  }

  TEST_CASE ("offset == used → no write")
  {
    u8 newdata[8];
    rand_bytes (newdata, sizeof newdata);

    const p_size off = DL_DATA_SIZE;
    const p_size got = dl_write (&p, newdata, off, sizeof newdata);
    test_assert_int_equal (got, 0);
  }
}
#endif

p_size
dl_write_from_buffer (
    const page     *d,
    struct cbuffer *src,
    const p_size    offset,
    const p_size    nbytes
)
{
  ASSERT (nbytes > 0);

  const p_size dlen = dl_used (d);
  u8          *base = dl_get_data (d);

  ASSERT (offset <= dlen);

  if (offset == dlen)
  {
    return 0;
  }

  const p_size avail  = dlen - offset;
  p_size       toread = MIN (avail, nbytes);

  if (toread > 0 && src)
  {
    toread = cbuffer_read (base + offset, 1, toread, src);
  }

  return toread;
}

p_size
dl_memset_from_buffer (page *d, struct cbuffer *src, const p_size nbytes)
{
  ASSERT (nbytes > 0);
  ASSERT (nbytes < DL_DATA_SIZE);

  const p_size read = cbuffer_read (dl_get_data (d), 1, nbytes, src);
  dl_set_used (d, read);

  return read;
}

void
dl_memset_from_buffer_expect (page *d, struct cbuffer *src, const p_size nbytes)
{
  const p_size read = dl_memset_from_buffer (d, src, nbytes);
  ASSERT (read == nbytes);
}

void
dl_memset (page *d, const u8 *buf, const p_size len)
{
  DBG_ASSERT (data_list, d);
  ASSERT (len <= DL_DATA_SIZE);

  memmove (dl_get_data (d), buf, len);
  dl_set_used (d, len);
}

#ifndef NTEST
TEST (dl_memset)
{
  page p;
  u8   buf1[64];
  u8   buf2[128];

  rand_bytes (buf1, sizeof buf1);
  rand_bytes (buf2, sizeof buf2);

  TEST_CASE ("basic fill")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    dl_memset (&p, buf1, sizeof buf1);

    test_assert_int_equal (dl_used (&p), sizeof buf1);
    test_assert_memequal (dl_get_data (&p), buf1, sizeof buf1);
  }

  TEST_CASE ("overwrite existing data")
  {
    dl_memset (&p, buf2, sizeof buf2);

    test_assert_int_equal (dl_used (&p), sizeof buf2);
    test_assert_memequal (dl_get_data (&p), buf2, sizeof buf2);
  }

  TEST_CASE ("len = 0 (valid, empties page)")
  {
    dl_memset (&p, buf1, 0);

    test_assert_int_equal (dl_used (&p), 0);
  }

  TEST_CASE ("len = DL_DATA_SIZE (fills entire available space)")
  {
    u8 full[DL_DATA_SIZE];
    rand_bytes (full, sizeof full);

    dl_memset (&p, full, DL_DATA_SIZE);

    test_assert_int_equal (dl_used (&p), DL_DATA_SIZE);
    test_assert_memequal (dl_get_data (&p), full, DL_DATA_SIZE);
  }
}
#endif

void
dl_set_data (page *p, const struct dl_data d)
{
  ASSERT (d.blen <= DL_DATA_SIZE);
  dl_memset (p, d.data, d.blen);
}

// Data Movement
void
dl_move_left (page *dest, page *src, const p_size len)
{
  DBG_ASSERT (data_list, dest);
  DBG_ASSERT (data_list, src);

  ASSERT (len <= dl_avail (dest));

  i_log_trace ("%" PRp_size " %" PRp_size "\n", dl_used (src), dl_used (dest));

  const p_size actual = MIN (len, dl_used (src));
  if (actual > 0)
  {
    dl_append (dest, dl_get_data (src), actual);
    dl_memset (src, (u8 *)dl_get_data (src) + actual, dl_used (src) - actual);
  }

  i_log_trace ("%" PRp_size " %" PRp_size "\n", dl_used (src), dl_used (dest));
}

#ifndef NTEST
TEST (dl_move_left)
{
  page src, dest;
  u8   srcbuf[64];
  u8   destbuf[64];

  rand_bytes (srcbuf, sizeof srcbuf);
  rand_bytes (destbuf, sizeof destbuf);

  TEST_CASE ("simple move")
  {
    rand_bytes (src.raw, NS_PAGE_SIZE);
    rand_bytes (dest.raw, NS_PAGE_SIZE);

    page_init_empty (&src, PG_DATA_LIST);
    page_init_empty (&dest, PG_DATA_LIST);

    dl_memset (&src, srcbuf, sizeof srcbuf);

    const p_size orig_used_src  = dl_used (&src);
    const p_size orig_used_dest = dl_used (&dest);

    dl_move_left (&dest, &src, 32);

    test_assert_int_equal (dl_used (&dest), orig_used_dest + 32);
    test_assert_int_equal (dl_used (&src), orig_used_src - 32);
    test_assert_memequal (dl_get_data (&dest), srcbuf, 32);
  }

  TEST_CASE ("request more than src has")
  {
    rand_bytes (src.raw, NS_PAGE_SIZE);
    rand_bytes (dest.raw, NS_PAGE_SIZE);

    page_init_empty (&src, PG_DATA_LIST);
    page_init_empty (&dest, PG_DATA_LIST);

    dl_memset (&src, srcbuf, 20);

    const p_size got_src_before  = dl_used (&src);
    const p_size got_dest_before = dl_used (&dest);

    dl_move_left (&dest, &src, 64); // only 20 available

    test_assert_int_equal (dl_used (&dest), got_dest_before + 20);
    test_assert_int_equal (dl_used (&src), got_src_before - 20);
  }

  TEST_CASE ("request when src empty")
  {
    rand_bytes (src.raw, NS_PAGE_SIZE);
    rand_bytes (dest.raw, NS_PAGE_SIZE);

    page_init_empty (&src, PG_DATA_LIST);
    page_init_empty (&dest, PG_DATA_LIST);

    dl_move_left (&dest, &src, 10);

    test_assert_int_equal (dl_used (&dest), 0);
    test_assert_int_equal (dl_used (&src), 0);
  }
}
#endif

void
dl_shift_right (page *d, const p_size len)
{
  DBG_ASSERT (data_list, d);
  ASSERT (len <= dl_avail (d));

  const p_size used = dl_used (d);

  // Shift existing bytes to the right
  memmove ((u8 *)dl_get_data (d) + len, dl_get_data (d), used);

  // Account for the new space
  dl_set_used (d, used + len);
}

#ifndef NTEST
TEST (dl_shift_right)
{
  page p;
  u8   src[32];
  u8   gapfill[4];

  rand_bytes (src, sizeof src);
  rand_bytes (gapfill, sizeof gapfill);

  TEST_CASE ("basic shift")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    // load 32 bytes
    dl_memset (&p, src, sizeof src);

    // shift right by 4
    dl_shift_right (&p, 4);

    // used should increase
    test_assert_int_equal (dl_used (&p), sizeof src + 4);

    // original data should now be at offset 4
    test_assert_memequal ((u8 *)dl_get_data (&p) + 4, src, sizeof src);

    // gap is writable
    memcpy (dl_get_data (&p), gapfill, sizeof gapfill);
    test_assert_memequal (dl_get_data (&p), gapfill, sizeof gapfill);
  }

  TEST_CASE ("zero shift is a no-op")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    dl_memset (&p, src, sizeof src);

    const p_size before = dl_used (&p);
    dl_shift_right (&p, 0);

    test_assert_int_equal (dl_used (&p), before);
    test_assert_memequal (dl_get_data (&p), src, sizeof src);
  }

  TEST_CASE ("maximum legal shift")
  {
    rand_bytes (p.raw, NS_PAGE_SIZE);
    page_init_empty (&p, PG_DATA_LIST);

    // fill small data
    dl_memset (&p, src, 8);

    const p_size avail = dl_avail (&p);
    dl_shift_right (&p, avail);

    // should now be completely full
    test_assert_int_equal (dl_used (&p), DL_DATA_SIZE);
    test_assert_memequal ((u8 *)dl_get_data (&p) + avail, src, 8);
  }
}
#endif

void
dl_move_right (page *src, page *dest, const p_size len)
{
  DBG_ASSERT (data_list, dest);
  DBG_ASSERT (data_list, src);

  ASSERT (len <= dl_avail (dest));

  const p_size actual = MIN (len, dl_used (src));
  if (actual == 0)
  {
    return;
  }

  const p_size ofst = dl_used (src) - actual;

  // Make space in dest
  dl_shift_right (dest, actual);

  // Copy the chunk from the *end* of src into the new space in dest
  memcpy (dl_get_data (dest), (u8 *)dl_get_data (src) + ofst, actual);

  // Shrink src
  dl_set_used (src, dl_used (src) - actual);
}

#ifndef NTEST
TEST (dl_move_right)
{
  page src, dest;
  u8   srcbuf[64];
  u8   destbuf[32];

  rand_bytes (srcbuf, sizeof srcbuf);
  rand_bytes (destbuf, sizeof destbuf);

  TEST_CASE ("basic move")
  {
    rand_bytes (src.raw, NS_PAGE_SIZE);
    rand_bytes (dest.raw, NS_PAGE_SIZE);

    page_init_empty (&src, PG_DATA_LIST);
    page_init_empty (&dest, PG_DATA_LIST);

    dl_memset (&src, srcbuf, sizeof srcbuf);
    dl_memset (&dest, destbuf, sizeof destbuf);

    const p_size src_before  = dl_used (&src);
    const p_size dest_before = dl_used (&dest);

    dl_move_right (&src, &dest, 16);

    // src shrinks, dest grows
    test_assert_int_equal (dl_used (&src), src_before - 16);
    test_assert_int_equal (dl_used (&dest), dest_before + 16);

    // the 16 bytes moved should equal the last 16 of srcbuf
    test_assert_memequal (
        dl_get_data (&dest),
        srcbuf + (sizeof srcbuf - 16),
        16
    );
  }

  TEST_CASE ("request more than src has → only available moves")
  {
    rand_bytes (src.raw, NS_PAGE_SIZE);
    rand_bytes (dest.raw, NS_PAGE_SIZE);

    page_init_empty (&src, PG_DATA_LIST);
    page_init_empty (&dest, PG_DATA_LIST);

    dl_memset (&src, srcbuf, 20);
    dl_memset (&dest, destbuf, 8);

    const p_size src_before  = dl_used (&src);
    const p_size dest_before = dl_used (&dest);

    dl_move_right (&src, &dest, 64); // only 20 available

    test_assert_int_equal (dl_used (&src), 0);
    test_assert_int_equal (dl_used (&dest), dest_before + src_before);

    // check last 20 of srcbuf landed at start of dest
    test_assert_memequal (dl_get_data (&dest), srcbuf, src_before);
  }

  TEST_CASE ("src empty → no-op")
  {
    rand_bytes (src.raw, NS_PAGE_SIZE);
    rand_bytes (dest.raw, NS_PAGE_SIZE);

    page_init_empty (&src, PG_DATA_LIST);
    page_init_empty (&dest, PG_DATA_LIST);

    dl_memset (&dest, destbuf, 12);

    const p_size src_before  = dl_used (&src);
    const p_size dest_before = dl_used (&dest);

    dl_move_right (&src, &dest, 10);

    test_assert_int_equal (dl_used (&src), src_before);
    test_assert_int_equal (dl_used (&dest), dest_before);
    test_assert_memequal (dl_get_data (&dest), destbuf, 12);
  }
}
#endif

void
i_log_dl (const int level, const page *d)
{
  i_log (level, "=== DATA LIST PAGE START ===\n");

  i_printf (level, "PGNO: %" PRpgno "\n", d->pg);
  if (dl_get_next (d) == PGNO_NULL)
  {
    i_printf (level, "NEXT: NULL\n");
  }
  else
  {
    i_printf (level, "NEXT: %" PRpgno "\n", dl_get_next (d));
  }
  if (dl_get_prev (d) == PGNO_NULL)
  {
    i_printf (level, "PREV: NULL\n");
  }
  else
  {
    i_printf (level, "PREV: %" PRpgno "\n", dl_get_prev (d));
  }
  i_printf (level, "BLEN: %u\n", dl_used (d));

  i_log (level, "=== DATA LIST PAGE END ===\n");
}

#ifndef NTEST
TEST (i_log_dl)
{
  page dl;
  dl.pg = 10;

  page_init_empty (&dl, PG_DATA_LIST);
  dl_make_valid (&dl);

  dl_set_next (&dl, PGNO_NULL);
  dl_set_prev (&dl, PGNO_NULL);
  i_log_dl (LOG_INFO, &dl);

  dl_set_next (&dl, 10);
  dl_set_prev (&dl, PGNO_NULL);
  i_log_dl (LOG_INFO, &dl);

  dl_set_next (&dl, PGNO_NULL);
  dl_set_prev (&dl, 10);
  i_log_dl (LOG_INFO, &dl);

  dl_set_next (&dl, 10);
  dl_set_prev (&dl, 10);
  i_log_dl (LOG_INFO, &dl);
}
#endif

void
dl_make_valid (page *d)
{
  dl_set_used (d, DL_DATA_SIZE);
}

#ifndef NTEST
TEST (dl_make_valid)
{
  page dl;
  dl.pg = 10;

  memset (dl.raw, 0, sizeof (dl.raw));
  page_init_empty (&dl, PG_DATA_LIST);
  dl_make_valid (&dl);
  page_set_checksum (&dl, page_compute_checksum (&dl));
  test_assert_equal (page_validate_for_db (&dl, PG_DATA_LIST, NULL), SUCCESS);
}
#endif

/******************************************************************************
 * SECTION: Inner Node
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

#define VTYPE  int
#define KTYPE  pgno
#define SUFFIX pgno
#include "robin_hood_ht.h"
#undef VTYPE
#undef KTYPE
#undef SUFFIX

// Initialization
void
in_init_empty (page *in)
{
  ASSERT (page_get_type (in) == PG_INNER_NODE);
  in_set_next (in, PGNO_NULL);
  in_set_prev (in, PGNO_NULL);
  in_set_len (in, 0);
  DBG_ASSERT (inner_node, in);
}

err_t
in_validate_for_db (const page *in, error *e)
{
  const pgh header = page_get_type (in);

  // Has the correct header
  if (header != (pgh)PG_INNER_NODE)
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "expected header: %" PRpgh " but got: %" PRpgh,
        (pgh)PG_INNER_NODE,
        (pgh)header
    );
  }

  // Len is less than Max Keys
  if (in_get_len (in) > IN_MAX_KEYS)
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "inner node len %" PRp_size " > max %" PRp_size " (page_size=%" PRp_size
        ")",
        in_get_len (in),
        IN_MAX_KEYS,
        NS_PAGE_SIZE
    );
  }

  // Check for duplicates
  hash_table_pgno ht;
  hentry_pgno     data[IN_MAX_KEYS];
  ht_init_pgno (&ht, data, IN_MAX_KEYS);

  for (p_size i = 0; i < in_get_len (in); ++i)
  {
    const hdata_pgno _data = {
        .key   = in_get_leaf (in, i),
        .value = 0,
    };
    const hti_res res = ht_insert_pgno (&ht, _data);
    if (res != HTIR_SUCCESS)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "duplicate leaf %" PRpgno " in inner node",
          _data.key
      );
    }
  }

  if (in_is_root (in))
  {
    if (in_get_len (in) == 0)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "Root node must have at least 1 "
          "element"
      );
    }
  }
  else
  {
    if (in_get_len (in) < IN_MAX_KEYS / 2)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "non-root inner node below half "
          "capacity (len=%" PRp_size " min=%" PRp_size ")",
          in_get_len (in),
          IN_MAX_KEYS / 2
      );
    }
  }

  return SUCCESS;
}

#ifndef NTEST
TEST (in_validate_for_db)
{
  error e = error_create ();
  page  in;

  TEST_CASE ("Invalid header")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 2);
    in_set_key_leaf (&in, 0, 1, 2);
    in_set_key_leaf (&in, 1, 2, 3);
    page_set_type (&in, PG_DATA_LIST);
    test_assert_int_equal (in_validate_for_db (&in, &e), ERR_CORRUPT);
    e.cause_code = SUCCESS;
  }

  TEST_CASE ("Len is too large")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 2);
    in_set_key_leaf (&in, 0, 1, 2);
    in_set_key_leaf (&in, 1, 2, 3);
    in_set_len (&in, IN_MAX_KEYS + 4);
    test_assert_int_equal (in_validate_for_db (&in, &e), ERR_CORRUPT);
    e.cause_code = SUCCESS;
  }

  TEST_CASE ("Keys are not strict monotonic (ok - used to be bad)")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 2);
    in_set_key_leaf (&in, 0, 2, 2);
    in_set_key_leaf (&in, 1, 1, 3);
    test_assert_int_equal (in_validate_for_db (&in, &e), SUCCESS);
    e.cause_code = SUCCESS;
  }

  TEST_CASE ("Duplicate leafs")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 2);
    in_set_key_leaf (&in, 0, 2, 2);
    in_set_key_leaf (&in, 1, 5, 2);
    test_assert_int_equal (in_validate_for_db (&in, &e), ERR_CORRUPT);
    e.cause_code = SUCCESS;
  }

  TEST_CASE ("Green path")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 2);
    in_set_key_leaf (&in, 0, 2, 2);
    in_set_key_leaf (&in, 1, 5, 3);
    test_assert_int_equal (in_validate_for_db (&in, &e), SUCCESS);
  }
}
#endif

// Getters

#ifndef NTEST
TEST (in_set_get_simple)
{
  page p;
  rand_bytes (p.raw, NS_PAGE_SIZE);

  TEST_CASE ("Start: freshly initialized page")
  {
    page_init_empty (&p, PG_INNER_NODE);

    test_assert_int_equal (in_get_len (&p), 0);
    test_assert_int_equal (in_get_size (&p), 0);
    test_assert_int_equal (in_get_avail (&p), IN_MAX_KEYS);
    test_assert_type_equal (in_get_next (&p), PGNO_NULL, pgno, PRpgno);
    test_assert_type_equal (in_get_prev (&p), PGNO_NULL, pgno, PRpgno);
  }

  TEST_CASE ("Set / Get basics")
  {
    page_init_empty (&p, PG_INNER_NODE);

    // Set next/prev pointers
    in_set_next (&p, 42);
    in_set_prev (&p, 43);
    test_assert_type_equal (in_get_next (&p), 42, pgno, PRpgno);
    test_assert_type_equal (in_get_prev (&p), 43, pgno, PRpgno);

    // Push a few keys/leaves
    in_push_end (&p, 5, 100);
    in_push_end (&p, 7, 101);
    in_push_end (&p, 9, 102);

    // Length & capacity
    test_assert_int_equal (in_get_len (&p), 3);
    test_assert_int_equal (in_get_avail (&p), IN_MAX_KEYS - 3);

    // Keys and unravelled sizes
    test_assert_int_equal (in_get_key (&p, 0), 5);
    test_assert_int_equal (in_get_key (&p, 1), 7);
    test_assert_int_equal (in_get_key (&p, 2), 9);

    // Leaves
    test_assert_type_equal (in_get_leaf (&p, 0), 100, pgno, PRpgno);
    test_assert_type_equal (in_get_leaf (&p, 1), 101, pgno, PRpgno);
    test_assert_type_equal (in_get_leaf (&p, 2), 102, pgno, PRpgno);

    // Derived macros
    test_assert_int_equal (in_get_right_most_key (&p), 9);
    test_assert_type_equal (in_get_right_most_leaf (&p), 102, pgno, PRpgno);
    test_assert_int_equal (in_get_nchildren (&p), 4);
    test_assert (in_full (&p) == false);
    test_assert_type_equal (in_get_first_leaf (&p), 100, pgno, PRpgno);
    test_assert_type_equal (in_get_last_leaf (&p), 102, pgno, PRpgno);
  }
}
#endif

// Setters
#ifndef NTEST
TEST (in_push_end)
{
  page in;

  TEST_CASE ("Happy path - filled")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 2);
    in_set_key_leaf (&in, 0, 1, 2);
    in_set_key_leaf (&in, 1, 2, 3);
    in_push_end (&in, 5, 6);

    test_assert_int_equal (in_get_len (&in), 3);
    test_assert_int_equal (in_get_leaf (&in, 2), 6);
    test_assert_int_equal (in_get_key (&in, 2), 5);
  }

  TEST_CASE ("Happy path - empty")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 0);
    in_push_end (&in, 5, 6);

    test_assert_int_equal (in_get_len (&in), 1);
    test_assert_int_equal (in_get_leaf (&in, 0), 6);
    test_assert_int_equal (in_get_key (&in, 0), 5);
  }
}
#endif

p_size
in_page_memcpy_right (pgno *dest, const page *src, const p_size ofst)
{
  ASSERT (ofst <= in_get_len (src));
  if (ofst == in_get_len (src))
  {
    return 0;
  }

  const p_size moving = in_get_len (src) - ofst;
  const pgno  *head   = in_get_leafs_imut (src);

  memcpy (dest, head + ofst, moving * sizeof *dest);
  return moving;
}

p_size
in_key_memcpy_right (b_size *dest, const page *src, const p_size ofst)
{
  ASSERT (ofst <= in_get_len (src));

  if (ofst == in_get_len (src))
  {
    return 0;
  }

  for (u32 i = ofst; i < in_get_len (src); ++i)
  {
    dest[i - ofst] = in_get_key (src, i);
  }

  return in_get_len (src) - ofst;
}

#ifndef NTEST
TEST (in_memcpy)
{
  page in;

  pgno   dest_page[NS_PAGE_SIZE];
  b_size dest_key[NS_PAGE_SIZE];

  TEST_CASE ("Size 0")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 1);
    in_set_key_leaf (&in, 0, 1, 2);

    // Page
    p_size copied = in_page_memcpy_right (dest_page, &in, 1);
    test_assert_int_equal (copied, 0);

    // key
    copied = in_key_memcpy_right (dest_key, &in, 1);
    test_assert_int_equal (copied, 0);
  }

  TEST_CASE ("Size 1")
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 1);
    in_set_key_leaf (&in, 0, 1, 2);

    // Page
    p_size copied = in_page_memcpy_right (dest_page, &in, 0);
    test_assert_int_equal (copied, 1);
    test_assert_int_equal (dest_page[0], 2);
    memset (dest_page, 0xFF, sizeof (dest_page));

    // key
    copied = in_key_memcpy_right (dest_key, &in, 0);
    test_assert_int_equal (copied, 1);
    test_assert_int_equal (dest_key[0], 1);
    memset (dest_key, 0xFF, sizeof (dest_key));
  }

  // Size > 1
  {
    page_init_empty (&in, PG_INNER_NODE);
    in_set_len (&in, 0);
    in_push_end (&in, 5, 0);
    in_push_end (&in, 6, 1);
    in_push_end (&in, 3, 2);
    in_push_end (&in, 1, 3);
    in_push_end (&in, 2, 4);
    in_push_end (&in, 10, 5);
    in_push_end (&in, 13, 6);
    in_push_end (&in, 8, 7);
    in_push_end (&in, 11, 8);
    in_push_end (&in, 12, 9);

    // Page
    const pgno expected_p[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    p_size     copied       = in_page_memcpy_right (dest_page, &in, 0);
    test_assert_int_equal (copied, 10);
    test_assert_int_equal (
        memcmp (dest_page, expected_p, sizeof (expected_p)),
        0
    );
    memset (dest_page, 0xFF, sizeof (dest_page));
    copied = in_page_memcpy_right (dest_page, &in, 3);
    test_assert_int_equal (copied, 7);
    test_assert_int_equal (
        memcmp (
            dest_page,
            &expected_p[3],
            sizeof (expected_p) - 3 * sizeof *expected_p
        ),
        0
    );
    memset (dest_page, 0xFF, sizeof (dest_page));

    // key
    const b_size expected_b1[] = {5, 6, 3, 1, 2, 10, 13, 8, 11, 12};
    copied                     = in_key_memcpy_right (dest_key, &in, 0);
    test_assert_int_equal (copied, 10);
    test_assert_int_equal (
        memcmp (dest_key, expected_b1, sizeof (expected_b1)),
        0
    );
    memset (dest_key, 0xFF, sizeof (dest_key));
    copied = in_key_memcpy_right (dest_key, &in, 3);
    test_assert_int_equal (copied, 7);
    test_assert_int_equal (
        memcmp (
            dest_key,
            &expected_b1[3],
            sizeof (expected_b1) - 3 * sizeof *expected_b1
        ),
        0
    );
    memset (dest_key, 0xFF, sizeof (dest_key));
  }
}
#endif

void
in_data_from_arrays (
    const struct in_data *dest,
    const pgno           *pgs,
    const b_size         *keys
)
{
  ASSERT (dest->len <= IN_MAX_KEYS);
  for (p_size i = 0; i < dest->len; ++i)
  {
    dest->nodes[i].pg  = pgs[i];
    dest->nodes[i].key = keys[i];
  }
}

void
in_set_data (page *p, const struct in_data data)
{
  ASSERT (data.len <= IN_MAX_KEYS);

  in_set_len (p, 0);

  for (p_size i = 0; i < data.len; ++i)
  {
    in_push_end (p, data.nodes[i].key, data.nodes[i].pg);
  }
}

struct in_data
in_get_data (const page *p, struct in_pair nodes[IN_MAX_KEYS])
{
  for (u32 i = 0; i < in_get_len (p); ++i)
  {
    nodes[i].pg  = in_get_leaf (p, i);
    nodes[i].key = in_get_key (p, i);
  }
  return (struct in_data){
      .nodes = nodes,
      .len   = in_get_len (p),
  };
}

// Data Movement

void
in_move_left (page *dest, page *src, const p_size len)
{
  ASSERT (len <= in_get_len (src));
  ASSERT (len <= in_get_avail (dest));

  for (p_size i = 0; i < len; ++i)
  {
    const pgno   pg  = in_get_leaf (src, i);
    const b_size key = in_get_key (src, i);
    in_push_end (dest, key, pg);
  }

  in_cut_left (src, len);
}

#ifndef NTEST
TEST (in_move_left)
{
  page left;
  page right;

  inner_node_init_for_testing (
      &left,
      (pgno[]){0, 1, 2, 3},
      (b_size[]){10, 20, 30, 40},
      4
  );

  inner_node_init_for_testing (
      &right,
      (pgno[]){4, 5, 6, 7},
      (b_size[]){5, 11, 18, 26},
      4
  );

  in_move_left (&left, &right, 1);

  test_assert_inner_node_equal (
      &left,
      (pgno[]){0, 1, 2, 3, 4},
      (b_size[]){10, 20, 30, 40, 5},
      5
  );

  test_assert_inner_node_equal (
      &right,
      (pgno[]){5, 6, 7},
      (b_size[]){11, 18, 26},
      3
  );
}

#  ifndef NTEST
TEST (in_move_left_two_keys)
{
  page left;
  page right;

  inner_node_init_for_testing (&left, (pgno[]){10}, (b_size[]){15}, 1);

  inner_node_init_for_testing (
      &right,
      (pgno[]){20, 21, 22},
      (b_size[]){5, 13, 22},
      3
  );

  in_move_left (&left, &right, 2);

  test_assert_inner_node_equal (
      &left,
      (pgno[]){10, 20, 21},
      (b_size[]){15, 5, 13},
      3
  );

  test_assert_inner_node_equal (&right, (pgno[]){22}, (b_size[]){22}, 1);
}
#  endif

#  ifndef NTEST
TEST (in_move_left_all_keys)
{
  page left;
  page right;

  inner_node_init_for_testing (&left, (pgno[]){1, 2}, (b_size[]){12, 28}, 2);

  inner_node_init_for_testing (&right, (pgno[]){3, 4}, (b_size[]){5, 10}, 2);

  in_move_left (&left, &right, 2);

  test_assert_inner_node_equal (
      &left,
      (pgno[]){1, 2, 3, 4},
      (b_size[]){12, 28, 5, 10},
      4
  );

  test_assert_inner_node_equal (&right, NULL, NULL, 0);
}
#  endif

#  ifndef NTEST
TEST (in_move_left_into_empty)
{
  page left;
  page right;

  inner_node_init_for_testing (&left, NULL, NULL, 0);

  inner_node_init_for_testing (
      &right,
      (pgno[]){5, 6, 7},
      (b_size[]){4, 10, 19},
      3
  );

  in_move_left (&left, &right, 2);

  test_assert_inner_node_equal (&left, (pgno[]){5, 6}, (b_size[]){4, 10}, 2);

  test_assert_inner_node_equal (&right, (pgno[]){7}, (b_size[]){19}, 1);
}
#  endif
#endif

void
in_push_left (page *in, const p_size len)
{
  ASSERT (len + in_get_len (in) <= IN_MAX_KEYS);

  if (len == 0)
  {
    return;
  }

  const p_size len0 = in_get_len (in);
  in_set_len (in, len + in_get_len (in));

  for (p_size i = len0; i > 0; --i)
  {
    const pgno   pg  = in_get_leaf (in, i - 1);
    const b_size key = in_get_key (in, i - 1);
    in_set_key_leaf (in, i + len - 1, key, pg);
  }
}

void
in_push_all_left (page *in)
{
  in_push_left (in, in_get_avail (in));
}

void
in_push_left_permissive (page *in, const p_size amnt)
{
  ASSERT (in_get_len (in) == IN_MAX_KEYS);
  ASSERT (amnt <= IN_MAX_KEYS);

  for (p_size i = amnt, k = IN_MAX_KEYS; i > 0; --i, --k)
  {
    const pgno   pg  = in_get_leaf (in, i - 1);
    const b_size key = in_get_key (in, i - 1);
    in_set_key_leaf (in, k - 1, key, pg);
  }
}

void
in_push_right_permissive (page *in, const p_size amnt)
{
  ASSERT (in_get_len (in) == IN_MAX_KEYS);
  ASSERT (amnt <= IN_MAX_KEYS);

  for (p_size i = amnt, k = 0; i < IN_MAX_KEYS; ++i, ++k)
  {
    const pgno   pg  = in_get_leaf (in, i);
    const b_size key = in_get_key (in, i);
    in_set_key_leaf (in, k, key, pg);
  }
}

#ifndef NTEST
TEST (in_push_left)
{
  page in;

  inner_node_init_for_testing (
      &in,
      (pgno[]){0, 1, 2, 3},
      (b_size[]){10, 21, 33, 46},
      4
  );

  in_push_left (&in, 1);

  test_assert_inner_node_equal (
      &in,
      (pgno[]){0, 0, 1, 2, 3},
      (b_size[]){10, 10, 21, 33, 46},
      5
  );

  in_push_left (&in, 2);

  test_assert_inner_node_equal (
      &in,
      (pgno[]){0, 0, 0, 0, 1, 2, 3},
      (b_size[]){10, 10, 10, 10, 21, 33, 46},
      7
  );
}

#  ifndef NTEST
TEST (in_push_left_into_empty)
{
  page in;

  inner_node_init_for_testing (&in, NULL, NULL, 0);

  in_push_left (&in, 2);

  test_assert_int_equal (in_get_len (&in), 2);
}
#  endif

#  ifndef NTEST
TEST (in_push_left_to_full)
{
  page in;

  // Start with IN_MAX_KEYS - 1 keys
  b_size keys[IN_MAX_KEYS - 1];
  pgno   pages[IN_MAX_KEYS - 1];

  for (u32 i = 0; i < IN_MAX_KEYS - 1; ++i)
  {
    keys[i]  = i;
    pages[i] = i;
  }

  inner_node_init_for_testing (&in, pages, keys, IN_MAX_KEYS - 1);

  in_push_left (&in, 1);

  b_size new_keys[IN_MAX_KEYS];
  pgno   new_pages[IN_MAX_KEYS];
  new_keys[0]  = 0;
  new_pages[0] = 0;
  for (u32 i = 1; i < IN_MAX_KEYS; ++i)
  {
    new_keys[i]  = i - 1;
    new_pages[i] = i - 1;
  }

  test_assert_inner_node_equal (&in, new_pages, new_keys, IN_MAX_KEYS);
}
#  endif

#endif

void
in_move_right (page *src, page *dest, const p_size len)
{
  ASSERT (len <= in_get_len (src));
  ASSERT (len <= in_get_avail (dest));

  if (len == 0)
  {
    return;
  }

  in_push_left (dest, len);

  b_size key = 0;
  for (p_size di = 0, si = in_get_len (src) - len; di < len; ++di, ++si)
  {
    const pgno pg = in_get_leaf (src, si);
    key           = in_get_key (src, si);

    in_set_key_leaf (dest, di, key, pg);
  }

  in_set_len (src, in_get_len (src) - len);
}

#ifndef NTEST
TEST (in_move_right)
{
  page left;
  page right;

  inner_node_init_for_testing (
      &left,
      (pgno[]){0, 1, 2, 3},
      (b_size[]){10, 20, 30, 40},
      4
  );

  inner_node_init_for_testing (
      &right,
      (pgno[]){4, 5, 6, 7},
      (b_size[]){5, 11, 18, 26},
      4
  );

  in_move_right (&left, &right, 1);

  test_assert_inner_node_equal (
      &left,
      (pgno[]){0, 1, 2},
      (b_size[]){10, 20, 30},
      3
  );

  test_assert_inner_node_equal (
      &right,
      (pgno[]){3, 4, 5, 6, 7},
      (b_size[]){40, 5, 11, 18, 26},
      5
  );
}

#  ifndef NTEST
TEST (in_move_right_two_keys)
{
  page left;
  page right;

  inner_node_init_for_testing (
      &left,
      (pgno[]){0, 1, 2, 3},
      (b_size[]){10, 20, 30, 40},
      4
  );

  inner_node_init_for_testing (&right, (pgno[]){4, 5}, (b_size[]){5, 11}, 2);

  in_move_right (&left, &right, 2);

  test_assert_inner_node_equal (&left, (pgno[]){0, 1}, (b_size[]){10, 20}, 2);

  test_assert_inner_node_equal (
      &right,
      (pgno[]){2, 3, 4, 5},
      (b_size[]){30, 40, 5, 11},
      4
  );
}
#  endif

#  ifndef NTEST
TEST (in_move_right_all_keys)
{
  page left;
  page right;

  inner_node_init_for_testing (&left, (pgno[]){0, 1}, (b_size[]){10, 25}, 2);

  inner_node_init_for_testing (&right, (pgno[]){2}, (b_size[]){5}, 1);

  in_move_right (&left, &right, 2);

  test_assert_inner_node_equal (&left, NULL, NULL, 0);

  test_assert_inner_node_equal (
      &right,
      (pgno[]){0, 1, 2},
      (b_size[]){10, 25, 5},
      3
  );
}
#  endif

#  ifndef NTEST
TEST (in_move_right_into_empty_right)
{
  page left;
  page right;

  inner_node_init_for_testing (
      &left,
      (pgno[]){42, 43, 44},
      (b_size[]){7, 15, 28},
      3
  );

  inner_node_init_for_testing (&right, NULL, NULL, 0); // empty page

  in_move_right (&left, &right, 1);

  test_assert_inner_node_equal (&left, (pgno[]){42, 43}, (b_size[]){7, 15}, 2);

  test_assert_inner_node_equal (
      &right,
      (pgno[]){44},
      (b_size[]){28},
      1
  ); // 28 - 15 = 13 unravel
}
#  endif

#endif

// Utils
void
in_choose_lidx (p_size *idx, b_size *nleft, const page *node, const b_size loc)
{
  const u32 n = in_get_len (node);
  ASSERT (n != 0);
  b_size key_total = 0;
  *nleft           = 0;

  p_size i = 0;
  for (; i < n - 1; ++i)
  {
    const b_size key = in_get_key (node, i);
    key_total += key;

    if (loc < key_total)
    {
      *idx = i;
      return;
    }

    *nleft += key;
  }

  *idx = i;
}

#ifndef NTEST
TEST (in_choose_lidx)
{
  page in;

  page_init_empty (&in, PG_INNER_NODE);
  in_set_len (&in, 5);
  // 2 4 5 10 11
  in_set_key_leaf (&in, 0, 2, 10);
  in_set_key_leaf (&in, 1, 2, 11);
  in_set_key_leaf (&in, 2, 1, 12);
  in_set_key_leaf (&in, 3, 5, 13);
  in_set_key_leaf (&in, 4, 1, 14);
  p_size idx;
  b_size nleft;
  in_choose_lidx (&idx, &nleft, &in, 0);
  test_assert_int_equal (idx, 0);
  test_assert_int_equal (nleft, 0);

  in_choose_lidx (&idx, &nleft, &in, 1);
  test_assert_int_equal (idx, 0);
  test_assert_int_equal (nleft, 0);

  in_choose_lidx (&idx, &nleft, &in, 2);
  test_assert_int_equal (idx, 1);
  test_assert_int_equal (nleft, 2);

  in_choose_lidx (&idx, &nleft, &in, 3);
  test_assert_int_equal (idx, 1);
  test_assert_int_equal (nleft, 2);

  in_choose_lidx (&idx, &nleft, &in, 4);
  test_assert_int_equal (idx, 2);
  test_assert_int_equal (nleft, 4);

  in_choose_lidx (&idx, &nleft, &in, 5);
  test_assert_int_equal (idx, 3);
  test_assert_int_equal (nleft, 5);

  in_choose_lidx (&idx, &nleft, &in, 6);
  test_assert_int_equal (idx, 3);
  test_assert_int_equal (nleft, 5);

  in_choose_lidx (&idx, &nleft, &in, 7);
  test_assert_int_equal (idx, 3);
  test_assert_int_equal (nleft, 5);

  in_choose_lidx (&idx, &nleft, &in, 8);
  test_assert_int_equal (idx, 3);
  test_assert_int_equal (nleft, 5);

  in_choose_lidx (&idx, &nleft, &in, 9);
  test_assert_int_equal (idx, 3);
  test_assert_int_equal (nleft, 5);

  in_choose_lidx (&idx, &nleft, &in, 10);
  test_assert_int_equal (idx, 4);
  test_assert_int_equal (nleft, 10);

  in_choose_lidx (&idx, &nleft, &in, 11);
  test_assert_int_equal (idx, 4);
  test_assert_int_equal (nleft, 10);

  in_choose_lidx (&idx, &nleft, &in, 12);
  test_assert_int_equal (idx, 4);
  test_assert_int_equal (nleft, 10);

  in_choose_lidx (&idx, &nleft, &in, 13);
  test_assert_int_equal (idx, 4);
  test_assert_int_equal (nleft, 10);

  in_choose_lidx (&idx, &nleft, &in, 5000);
  test_assert_int_equal (idx, 4);
  test_assert_int_equal (nleft, 10);
}
#endif

void
in_cut_left (page *in, const p_size end)
{
  ASSERT (end <= in_get_len (in));

  if (end == 0)
  {
    return;
  }

  for (p_size i = 0; i < in_get_len (in) - end; ++i)
  {
    const pgno   pg  = in_get_leaf (in, end + i);
    const b_size key = in_get_key (in, end + i);

    in_set_key_leaf (in, i, key, pg);
  }

  in_set_len (in, in_get_len (in) - end);
}

#ifndef NTEST
TEST (in_cut_left)
{
  page in;

  inner_node_init_for_testing (
      &in,
      (pgno[]){0, 1, 2, 3},
      (b_size[]){10, 21, 33, 46},
      4
  );

  in_cut_left (&in, 0);

  test_assert_inner_node_equal (
      &in,
      (pgno[]){0, 1, 2, 3},
      (b_size[]){10, 21, 33, 46},
      4
  );

  in_cut_left (&in, 1);

  test_assert_inner_node_equal (
      &in,
      (pgno[]){1, 2, 3},
      (b_size[]){21, 33, 46},
      3
  );

  in_cut_left (&in, 2);

  test_assert_inner_node_equal (&in, (pgno[]){3}, (b_size[]){46}, 1);

  in_cut_left (&in, 1);

  test_assert_inner_node_equal (&in, NULL, NULL, 0);
}

TEST (in_cut_left_all_at_once)
{
  page in;

  inner_node_init_for_testing (
      &in,
      (pgno[]){10, 20, 30},
      (b_size[]){5, 15, 30},
      3
  );

  in_cut_left (&in, 3);

  test_assert_inner_node_equal (&in, NULL, NULL, 0);
}
#endif

#ifndef NTEST
TEST (in_cut_left_from_empty)
{
  page in;

  inner_node_init_for_testing (&in, NULL, NULL, 0);

  in_cut_left (&in, 0);

  test_assert_inner_node_equal (&in, NULL, NULL, 0);
}
#endif

#ifndef NTEST
TEST (in_cut_left_to_one)
{
  page in;

  inner_node_init_for_testing (
      &in,
      (pgno[]){1, 2, 3, 4},
      (b_size[]){7, 14, 22, 31},
      4
  );

  in_cut_left (&in, 3);

  test_assert_inner_node_equal (&in, (pgno[]){4}, (b_size[]){31}, 1);
}
#endif

void
in_make_valid (page *d)
{
  in_set_len (d, IN_MAX_KEYS);
  for (p_size i = 0; i < IN_MAX_KEYS; ++i)
  {
    in_set_key_leaf (d, i, i, i);
  }
}

void
i_log_in (const int level, const page *in)
{
  i_log (level, "=== INNER NODE PAGE START ===\n");

  i_printf (level, "PGNO: %" PRpgno "\n", in->pg);
  if (in_get_next (in) == PGNO_NULL)
  {
    i_printf (level, "NEXT: NULL\n");
  }
  else
  {
    i_printf (level, "NEXT: %" PRpgno "\n", in_get_next (in));
  }
  if (in_get_prev (in) == PGNO_NULL)
  {
    i_printf (level, "PREV: NULL\n");
  }
  else
  {
    i_printf (level, "PREV: %" PRpgno "\n", in_get_prev (in));
  }
  i_printf (level, "SIZE: %" PRb_size "\n", in_get_size (in));
  i_printf (level, "LEN:  %u\n", in_get_len (in));

  const u32 len = in_get_len (in);
  for (u32 i = 0; i < len; ++i)
  {
    char line[128] = {0};
    int  pos       = 0;
    pos += snprintf (
        &line[pos],
        sizeof (line) - pos,
        "(%" PRb_size ", %" PRpgno ")",
        in_get_key (in, i),
        in_get_leaf (in, i)
    );
    i_printf (level, "%s ", line);
  }
  i_printf (level, "\n");

  i_log (level, "=== INNER NODE PAGE END ===\n");
}

#ifndef NTEST
TEST (i_log_in)
{
  page in;
  in.pg = 10;

  page_init_empty (&in, PG_INNER_NODE);
  in_make_valid (&in);
  in_set_prev (&in, 10);
  in_set_next (&in, 10);
  i_log_in (LOG_INFO, &in);

  in_set_prev (&in, PGNO_NULL);
  in_set_next (&in, PGNO_NULL);
  i_log_in (LOG_INFO, &in);
}
#endif

/******************************************************************************
 * SECTION: Var Hash Page
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
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
      i_printf (level, "[%" PRp_size "]: %" PRpgno "\n", i, p);
    }
  }

  if (empty)
  {
    i_printf (level, "Empty\n");
  }

  i_log (level, "=== VAR HASH TABLE PAGE END ===\n");
}

#ifndef NTEST
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

/******************************************************************************
 * SECTION: Var Page
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

DEFINE_DBG_ASSERT (page, vp_page, v, { ASSERT (v); })

////////////////////////////////////////////////////////////
// INITIALIZATION

void
vp_init_empty (page *p)
{
  ASSERT (page_get_type (p) == PG_VAR_PAGE);
  vp_set_next (p, PGNO_NULL);
  vp_set_ovnext (p, PGNO_NULL);
  vp_set_vlen (p, 0);
  vp_set_tlen (p, 0);
  vp_set_root (p, PGNO_NULL);
}

#ifndef NTEST
TEST (vp_init_empty)
{
  page p;

  rand_bytes (p.raw, NS_PAGE_SIZE);
  page_init_empty (&p, PG_VAR_PAGE);

  test_assert_equal (vp_get_next (&p), PGNO_NULL);
  test_assert_equal (vp_get_ovnext (&p), PGNO_NULL);
  test_assert_equal (vp_get_vlen (&p), 0);
  test_assert_equal (vp_get_tlen (&p), 0);
  test_assert_equal (vp_get_root (&p), PGNO_NULL);
  test_assert_equal (vp_calc_tofst (&p), VP_VNME_OFST);
  test_assert_equal (vp_is_overflow (&p), false);
}
#endif

////////////////////////////////////////////////////////////
// SETTERS

void
vp_set_next (page *p, const pgno pg)
{
  PAGE_SIMPLE_SET_IMPL (p, pg, VP_NEXT_OFST);
}

void
vp_set_ovnext (page *p, const pgno pg)
{
  PAGE_SIMPLE_SET_IMPL (p, pg, VP_OVNX_OFST);
}

void
vp_set_vlen (page *p, const u16 vlen)
{
  PAGE_SIMPLE_SET_IMPL (p, vlen, VP_VLEN_OFST);
}

void
vp_set_tlen (page *p, const u16 tlen)
{
  PAGE_SIMPLE_SET_IMPL (p, tlen, VP_TLEN_OFST);
}

void
vp_set_root (page *p, const pgno root)
{
  PAGE_SIMPLE_SET_IMPL (p, root, VP_ROOT_OFST);
}

void
vp_set_nbytes (page *p, const b_size nbytes)
{
  PAGE_SIMPLE_SET_IMPL (p, nbytes, VP_NBYT_OFST);
}

void vp_append_cbuffer (page *p, struct cbuffer src);

////////////////////////////////////////////////////////////
// GETTERS

pgno
vp_get_next (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, pgno, VP_NEXT_OFST);
}

pgno
vp_get_ovnext (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, pgno, VP_OVNX_OFST);
}

u16
vp_get_vlen (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, u16, VP_VLEN_OFST);
}

u16
vp_get_tlen (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, u16, VP_TLEN_OFST);
}

pgno
vp_get_root (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, pgno, VP_ROOT_OFST);
}

b_size
vp_get_nbytes (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, b_size, VP_NBYT_OFST);
}

b_size
vp_calc_tofst (const page *p)
{
  const b_size vlen = (b_size)vp_get_vlen (p);
  return (b_size)VP_VNME_OFST + vlen;
}

bool
vp_is_overflow (const page *p)
{
  return vp_calc_tofst (p) > NS_PAGE_SIZE;
}

struct bytes
vp_get_bytes (page *p)
{
  return (struct bytes){
      .head = (void *)&p->raw[VP_VNME_OFST],
      .len  = NS_PAGE_SIZE - VP_VNME_OFST,
  };
}

struct cbytes
vp_get_bytes_imut (const page *p)
{
  return (struct cbytes){
      .head = (void *)&p->raw[VP_VNME_OFST],
      .len  = NS_PAGE_SIZE - VP_VNME_OFST,
  };
}

////////////////////////////////////////////////////////////
// VALIDATION

err_t
vp_validate_for_db (const page *p, error *e)
{
  if (page_get_type (p) != PG_VAR_PAGE)
  {
    return error_causef (e, ERR_CORRUPT, "wrong page type for var page");
  }
  if (vp_get_vlen (p) == 0)
  {
    return error_causef (e, ERR_CORRUPT, "var length is 0");
  }
  if (vp_get_tlen (p) == 0)
  {
    return error_causef (e, ERR_CORRUPT, "type length is 0");
  }
  if (vp_is_overflow (p) && vp_get_ovnext (p) == PGNO_NULL)
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "overflow required but next pointer is null"
    );
  }
  return SUCCESS;
}

#ifndef NTEST
TEST (vp_validate)
{
  page  sut;
  error e = error_create ();

  TEST_CASE ("Invalid page type")
  {
    page_init_empty (&sut, PG_DATA_LIST);
    vp_set_vlen (&sut, 10);
    vp_set_tlen (&sut, 10);
    test_err_t_check (vp_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }

  TEST_CASE ("Empty variable length")
  {
    page_init_empty (&sut, PG_VAR_PAGE);
    vp_set_vlen (&sut, 0);
    vp_set_tlen (&sut, 10);
    test_err_t_check (vp_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }

  TEST_CASE ("Empty type string length")
  {
    page_init_empty (&sut, PG_VAR_PAGE);
    vp_set_vlen (&sut, 10);
    vp_set_tlen (&sut, 0);
    test_err_t_check (vp_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }

  TEST_CASE ("Overflow page requires next pointer")
  {
    page_init_empty (&sut, PG_VAR_PAGE);
    vp_set_vlen (&sut, NS_PAGE_SIZE);
    vp_set_tlen (&sut, 10);
    vp_set_ovnext (&sut, PGNO_NULL);
    test_err_t_check (vp_validate_for_db (&sut, &e), ERR_CORRUPT, &e);
  }

  TEST_CASE ("Valid minimal varpage")
  {
    page_init_empty (&sut, PG_VAR_PAGE);
    vp_set_vlen (&sut, 1);
    vp_set_tlen (&sut, 1);
    test_err_t_check (vp_validate_for_db (&sut, &e), SUCCESS, &e);
  }

  TEST_CASE ("Valid overflow with next pointer")
  {
    page_init_empty (&sut, PG_VAR_PAGE);
    vp_set_vlen (&sut, 100);
    vp_set_tlen (&sut, 10);
    vp_set_next (&sut, 42);
    test_err_t_check (vp_validate_for_db (&sut, &e), SUCCESS, &e);
  }
}
#endif

////////////////////////////////////////////////////////////
// UTILS

void
i_log_vp (const int level, const page *vp)
{
  i_log (level, "=== VARIABLE PAGE START ===\n");

  i_printf (level, "PGNO:   %" PRpgno "\n", vp->pg);
  i_printf (level, "VLEN:   %u\n", vp_get_vlen (vp));
  i_printf (level, "TLEN:   %u\n", vp_get_tlen (vp));
  if (vp_get_root (vp) == PGNO_NULL)
  {
    i_printf (level, "ROOT:  NULL\n");
  }
  else
  {
    i_printf (level, "ROOT:  %" PRpgno "\n", vp_get_root (vp));
  }
  if (vp_get_next (vp) == PGNO_NULL)
  {
    i_printf (level, "NEXT:  NULL\n");
  }
  else
  {
    i_printf (level, "NEXT:  %" PRpgno "\n", vp_get_next (vp));
  }
  if (vp_get_ovnext (vp) == PGNO_NULL)
  {
    i_printf (level, "OVNEXT:  NULL\n");
  }
  else
  {
    i_printf (level, "OVNEXT:  %" PRpgno "\n", vp_get_ovnext (vp));
  }
  i_printf (level, "TOFST:  %" PRb_size "\n", vp_calc_tofst (vp));

  i_log (level, "=== VARIABLE PAGE END ===\n");
}

#ifndef NTEST
TEST (i_log_vp)
{
  page vp;
  vp.pg = 10;

  page_init_empty (&vp, PG_VAR_PAGE);

  vp_set_next (&vp, 10);
  vp_set_ovnext (&vp, 10);
  vp_set_root (&vp, 10);
  i_log_vp (LOG_INFO, &vp);

  vp_set_next (&vp, PGNO_NULL);
  vp_set_ovnext (&vp, PGNO_NULL);
  vp_set_root (&vp, PGNO_NULL);
  i_log_vp (LOG_INFO, &vp);
}
#endif

/******************************************************************************
 * SECTION: Var Tail
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

DEFINE_DBG_ASSERT (page, vt_page, v, { ASSERT (v); })

////////////////////////////////////////////////////////////
// INITIALIZATION

#ifndef NTEST
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

#ifndef NTEST
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

  i_printf (level, "PGNO:   %" PRpgno "\n", vp->pg);
  if (vt_get_next (vp) == PGNO_NULL)
  {
    i_printf (level, "NEXT:   NULL\n");
  }
  else
  {
    i_printf (level, "NEXT:   %" PRpgno "\n", vt_get_next (vp));
  }

  i_log (level, "=== VARIABLE TAIL END ===\n");
}

#ifndef NTEST
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
