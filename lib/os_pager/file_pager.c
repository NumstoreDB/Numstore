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

#include "os_pager/file_pager.h"

#include "c_specx.h"
#include "compile_config.h"
#include "errors.h"

#include <string.h>

/*
 * Concrete page-I/O implementation backed by a single OS file.
 *
 * The os_pager base must be the first member so that a struct file_pager *
 * can be freely cast to struct os_pager * and back, as required by the
 * vtable dispatch pattern.
 */
struct file_pager
{
  struct os_pager base;
  _Atomic pgno npages;
  i_file f;

  latch l;
};

DEFINE_DBG_ASSERT (struct file_pager, file_pager, p, { ASSERT (p); })

////////////////////////////////////////////////////////////
/// Interface assignment

static err_t
fp_close_impl (struct os_pager *self, error *e)
{
  struct file_pager *fp = (struct file_pager *)self;
  return fpgr_close (fp, e);
}

static err_t
fp_reset_impl (struct os_pager *self, error *e)
{
  struct file_pager *fp = (struct file_pager *)self;
  return fpgr_reset (fp, e);
}

static p_size
fp_get_npages_impl (const struct os_pager *self)
{
  const struct file_pager *fp = (const struct file_pager *)self;
  return fpgr_get_npages (fp);
}

static err_t
fp_extend_impl (struct os_pager *self, const pgno dest, error *e)
{
  struct file_pager *fp = (struct file_pager *)self;
  return fpgr_extend (fp, dest, e);
}

static err_t
fp_read_impl (struct os_pager *self, u8 *dest, const pgno pg, error *e)
{
  struct file_pager *fp = (struct file_pager *)self;
  return fpgr_read (fp, dest, pg, e);
}

static err_t
fp_write_impl (struct os_pager *self, const u8 *src, const pgno pg, error *e)
{
  struct file_pager *fp = (struct file_pager *)self;
  return fpgr_write (fp, src, pg, e);
}

static err_t
fp_crash_impl (struct os_pager *self, error *e)
{
  struct file_pager *fp = (struct file_pager *)self;
  return fpgr_crash (fp, e);
}

static const struct os_pager_vtable fp_vtable = {
  .close = fp_close_impl,
  .reset = fp_reset_impl,
  .get_npages = fp_get_npages_impl,
  .extend = fp_extend_impl,
  .read = fp_read_impl,
  .write = fp_write_impl,
  .crash_fn = fp_crash_impl,
};

struct os_pager *
fpgr_open_os (const char *dbname, error *e)
{
  return (struct os_pager *)fpgr_open (dbname, e);
}

////////////////////////////////////////////////////////////
/// Regular functions

/**
 * TODO:
 *   - Lock the file on new file pager creation
 */
struct file_pager *
fpgr_open (const char *dbname, error *e)
{
  struct file_pager *dest = i_malloc (1, sizeof *dest, e);
  if (dest == NULL)
    {
      return NULL;
    }

  dest->base.vtable = &fp_vtable;
  latch_init (&dest->l);

  // Open the database in read write mode
  if (i_open_rw (&dest->f, dbname, e))
    {
      goto failed;
    }

  i64 size = i_file_size (&dest->f, e);

  // Failed
  if (size < 0)
    {
      goto fp_failed;
    }

  // Corrupt database - not a multiple of PAGE_SIZE
  if (size % PAGE_SIZE != 0)
    {
      error_causef (e, ERR_CORRUPT,
                    "file size %" PRId64
                    " is not a multiple of PAGE_SIZE (%" PRp_size ")",
                    size, PAGE_SIZE);
      goto fp_failed;
    }

  atomic_store (&dest->npages, size / PAGE_SIZE);

  DBG_ASSERT (file_pager, dest);

  return dest;

fp_failed:
  i_close (&dest->f, e);
  i_free (dest);

failed:
  return NULL;
}

#ifndef NTEST
TEST (fpgr_open)
{
  error e = error_create ();
  _Static_assert(PAGE_SIZE > 2,
                 "PAGE_SIZE should be > 2 for file_pager test");

  i_file fp = { 0 };
  i_open_rw (&fp, "test.db", &e);

  // edge case: file shorter than header
  test_fail_if (i_truncate (&fp, PAGE_SIZE - 1, &e));
  struct file_pager *pager = fpgr_open ("test.db", &e);
  test_err_t_check (e.cause_code, ERR_CORRUPT, &e);

  // edge case: file size = half a page
  test_fail_if (i_truncate (&fp, PAGE_SIZE / 2, &e));
  pager = fpgr_open ("test.db", &e);
  test_err_t_check (e.cause_code, ERR_CORRUPT, &e);

  // happy path: file exactly header size, zero pages
  test_fail_if (i_truncate (&fp, 0, &e));
  pager = fpgr_open ("test.db", &e);
  test_assert_int_equal ((int)atomic_load (&pager->npages), 0);
  test_fail_if (fpgr_close (pager, &e));

  // happy path: file exactly header size, more pages
  test_fail_if (i_truncate (&fp, 3 * PAGE_SIZE, &e));
  pager = fpgr_open ("test.db", &e);
  test_assert_equal (atomic_load (&pager->npages), 3);
  test_fail_if (fpgr_close (pager, &e));

  // There were 2 refs to file - close it here too
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink ("test.db", &e));
}
#endif

err_t
fpgr_close (struct file_pager *f, error *e)
{
  DBG_ASSERT (file_pager, f);
  latch_lock (&f->l); // Never release this
  i_close (&f->f, e);
  i_free (f);
  return error_trace (e);
}

err_t
fpgr_reset (struct file_pager *f, error *e)
{
  DBG_ASSERT (file_pager, f);

  latch_lock (&f->l);
  if (i_truncate (&f->f, 0, e))
    {
      latch_unlock (&f->l);
      return error_trace (e);
    }
  atomic_store (&f->npages, 0);
  latch_unlock (&f->l);

  return error_trace (e);
}

p_size
fpgr_get_npages (const struct file_pager *fp)
{
  DBG_ASSERT (file_pager, fp);
  return atomic_load (&fp->npages);
}

err_t
fpgr_extend (struct file_pager *p, pgno dest, error *e)
{
  DBG_ASSERT (file_pager, p);
  ASSERT (dest);

  latch_lock (&p->l);

  if (dest < atomic_load (&p->npages))
    {
      latch_unlock (&p->l);
      return SUCCESS;
    }

  if (i_truncate (&p->f, PAGE_SIZE * (dest), e))
    {
      latch_unlock (&p->l);
      goto failed;
    }

  atomic_store (&p->npages, dest);
  latch_unlock (&p->l);

  return SUCCESS;

failed:
  return error_trace (e);
}

#ifndef NTEST
TEST (fpgr_new)
{
  i_file fp = { 0 };
  error e = error_create ();
  test_fail_if (i_open_rw (&fp, "test.db", &e));

  test_fail_if (i_truncate (&fp, 0, &e));

  struct file_pager *pager = fpgr_open ("test.db", &e);

  // Create a new page
  test_fail_if (fpgr_extend (pager, 1, &e));
  test_assert_int_equal (atomic_load (&pager->npages), 1);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * atomic_load (&pager->npages));

  // Add two more pages and do the same thing
  test_fail_if (fpgr_extend (pager, 2, &e));
  test_assert_int_equal (atomic_load (&pager->npages), 2);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * atomic_load (&pager->npages));

  test_fail_if (fpgr_extend (pager, 3, &e));
  test_assert_int_equal (atomic_load (&pager->npages), 3);
  test_assert_int_equal (i_file_size (&fp, &e), PAGE_SIZE * atomic_load (&pager->npages));

  test_fail_if (fpgr_close (pager, &e));

  // There were 2 refs to file - close it here too
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink ("test.db", &e));
}
#endif

err_t
fpgr_read (struct file_pager *p, u8 *dest, pgno pg, error *e)
{
  ASSERT (dest);

  latch_lock (&p->l);

  DBG_ASSERT (file_pager, p);

  if (pg >= atomic_load (&p->npages))
    {
      error_causef (
          e, ERR_PG_OUT_OF_RANGE,
          "page %" PRpgno " out of range (npages=%" PRpgno ")", pg, atomic_load (&p->npages));
      goto theend;
    }

  // Read all from file
  const i64 nread = i_pread_all (&p->f, dest, PAGE_SIZE, pg * PAGE_SIZE, e);

  if (nread == 0)
    {
      error_causef (e, ERR_CORRUPT, "pread returned 0 bytes at page %" PRpgno, pg);
      goto theend;
    }

  if (nread < 0)
    {
      goto theend;
    }

theend:

  latch_unlock (&p->l);

  return error_trace (e);
  ;
}

err_t
fpgr_write (struct file_pager *p, const u8 *src, const pgno pg, error *e)
{
  ASSERT (src);

  latch_lock (&p->l);

  DBG_ASSERT (file_pager, p);
  ASSERT (pg < atomic_load (&p->npages));

  if (i_pwrite_all (&p->f, src, PAGE_SIZE, pg * PAGE_SIZE, e))
    {
      goto theend;
    }

theend:

  latch_unlock (&p->l);

  return error_trace (e);
}

#ifndef NTEST
TEST (fpgr_read_write)
{
  // The raw page bytes
  u8 _page[PAGE_SIZE];

  // Create a temporary file
  i_file fp = { 0 };
  error e = error_create ();
  test_fail_if (i_open_rw (&fp, "test.db", &e));

  // File should be size 0
  test_fail_if (i_truncate (&fp, 0, &e));

  // Open a new pager
  struct file_pager *pager = fpgr_open ("test.db", &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  // happy path: new page, write, then read back
  test_fail_if (fpgr_extend (pager, 2, &e));

  // Write 0 : PAGE_SIZE to each byte (overflow fine, it's just data)
  for (u32 i = 0; i < PAGE_SIZE; i++)
    {
      _page[i] = (u8)i;
    }
  // Write it out
  test_fail_if (fpgr_write (pager, _page, 1, &e));

  // Scramble page so we can read it back in
  memset (_page, 0xFF, PAGE_SIZE);
  test_fail_if (fpgr_read (pager, _page, 1, &e));

  // Iterate and check that it matches what we expect
  for (u32 i = 0; i < PAGE_SIZE; i++)
    {
      test_assert_int_equal (_page[i], (u8)i);
    }

  // There's 2 refs to this file, close the other one
  test_fail_if (fpgr_close (pager, &e));
  test_fail_if (i_close (&fp, &e));
  test_fail_if (i_unlink ("test.db", &e));
}
#endif

err_t
fpgr_crash (struct file_pager *p, error *e)
{
  DBG_ASSERT (file_pager, p);
  latch_lock (&p->l);
  i_close (&p->f, e);
  i_free (p);
  return error_trace (e);
}
