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

#include "core/os/test/ns_dst.h"

#include "core/ns_numerics.h" // randf
#include "core/os/ns_file.h"
#include "core/testing/ns_testing.h"

#include <string.h> // memset

#ifdef TESTING

/******************************************************************************
 * SECTION: File System
 ******************************************************************************/

static err_t
dst_open_rw (void *vfs, i_file *dest, const char *fname, error *e)
{
  struct dst_data *d = vfs;

  if (randf () < d->filesystem.open_rw_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated open_rw failure");
  }

  const err_t rc = d->delegate->open_rw (NULL, dest, fname, e);
  if (rc == SUCCESS) {
    dest->table = &d->vtable;
  }
  return rc;
}

static err_t
dst_open_r (void *vfs, i_file *dest, const char *fname, error *e)
{
  struct dst_data *d = vfs;

  if (randf () < d->filesystem.open_r_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated open_r failure");
  }

  const err_t rc = d->delegate->open_r (NULL, dest, fname, e);
  if (rc == SUCCESS) {
    dest->table = &d->vtable;
  }
  return rc;
}

static err_t
dst_open_w (void *vfs, i_file *dest, const char *fname, error *e)
{
  struct dst_data *d = vfs;

  if (randf () < d->filesystem.open_w_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated open_w failure");
  }

  const err_t rc = d->delegate->open_w (NULL, dest, fname, e);
  if (rc == SUCCESS) {
    dest->table = &d->vtable;
  }
  return rc;
}

static err_t
dst_remove_quiet (void *vfs, const char *fname, error *e)
{
  struct dst_data *d = vfs;

  if (randf () < d->filesystem.remove_quiet_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated remove_quiet failure");
  }

  return d->delegate->remove_quiet (NULL, fname, e);
}

static err_t
dst_unlink (void *vfs, const char *fname, error *e)
{
  struct dst_data *d = vfs;

  if (randf () < d->filesystem.unlink_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated unlink failure");
  }

  return d->delegate->unlink (NULL, fname, e);
}

static err_t
dst_file_exists (void *vfs, const char *fname, bool *dest, error *e)
{
  struct dst_data *d = vfs;

  if (randf () < d->filesystem.file_exists_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated file_exists failure");
  }

  return d->delegate->file_exists (NULL, fname, dest, e);
}

/******************************************************************************
 * SECTION: Memory
 ******************************************************************************/

static void *
dst_malloc (void *v, u32 nelem, u32 size, error *e)
{
  struct dst_data *d = v;

  if (randf () < d->memory.malloc_fail_prob) {
    error_causef (e, ERR_NOMEM, "dst: simulated malloc failure");
    return NULL;
  }

  return d->delegate->malloc (NULL, nelem, size, e);
}

static void *
dst_calloc (void *v, u32 nelem, u32 size, error *e)
{
  struct dst_data *d = v;

  if (randf () < d->memory.calloc_fail_prob) {
    error_causef (e, ERR_NOMEM, "dst: simulated calloc failure");
    return NULL;
  }

  return d->delegate->calloc (NULL, nelem, size, e);
}

static void *
dst_realloc (void *v, void *ptr, u32 nelem, u32 size, error *e)
{
  struct dst_data *d = v;

  if (randf () < d->memory.realloc_fail_prob) {
    error_causef (e, ERR_NOMEM, "dst: simulated realloc failure");
    return NULL;
  }

  return d->delegate->realloc (NULL, ptr, nelem, size, e);
}

/******************************************************************************
 * SECTION: Threading
 ******************************************************************************/

static err_t
dst_thread_create (void *t, i_thread *th, void *(*start_routine) (void *), void *arg, error *e)
{
  struct dst_data *d = t;

  if (randf () < d->threading.thread_create_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated thread_create failure");
  }

  return d->delegate->thread_create (NULL, th, start_routine, arg, e);
}

static err_t
dst_mutex_create (void *t, i_mutex *m, error *e)
{
  struct dst_data *d = t;

  if (randf () < d->threading.mutex_create_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated mutex_create failure");
  }

  return d->delegate->mutex_create (NULL, m, e);
}

static err_t
dst_cond_create (void *t, i_cond *c, error *e)
{
  struct dst_data *d = t;

  if (randf () < d->threading.cond_create_fail_prob) {
    return error_causef (e, ERR_IO, "dst: simulated cond_create failure");
  }

  return d->delegate->cond_create (NULL, c, e);
}

/******************************************************************************
 * SECTION: Construction
 ******************************************************************************/

void
dst_data_init (struct dst_data *d, const struct os_vtable *delegate)
{
  memset (d, 0, sizeof (*d));
  d->delegate = delegate ? delegate : &default_os_vtable;

  d->vtable   = (struct os_vtable){
      // File ops delegate straight through to the real implementation - the
      // fault injection for these lives inside impl_* itself (see
      // I_FILE_FAULT / I_FILE_CONDITION_AMOUNT in ns_posix_file.c /
      // ns_win32_file.c), keyed off `test_data` below since i_file has no
      // separate data pointer to carry a struct dst_data* directly.
      .close           = d->delegate->close,
      .fsync           = d->delegate->fsync,
      .file_size       = d->delegate->file_size,
      .read_all        = d->delegate->read_all,
      .pread_all       = d->delegate->pread_all,
      .write_all       = d->delegate->write_all,
      .pwrite_all      = d->delegate->pwrite_all,
      .writev_all      = d->delegate->writev_all,
      .truncate        = d->delegate->truncate,
      .fallocate       = d->delegate->fallocate,
      .seek            = d->delegate->seek,
      .test_data       = d,

      .open_rw         = dst_open_rw,
      .open_r          = dst_open_r,
      .open_w          = dst_open_w,
      .remove_quiet    = dst_remove_quiet,
      .unlink          = dst_unlink,
      .file_exists     = dst_file_exists,

      .malloc          = dst_malloc,
      .calloc          = dst_calloc,
      .realloc         = dst_realloc,
      .free            = d->delegate->free,

      .thread_create   = dst_thread_create,
      .thread_join     = d->delegate->thread_join,
      .mutex_create    = dst_mutex_create,
      .mutex_free      = d->delegate->mutex_free,
      .mutex_lock      = d->delegate->mutex_lock,
      .mutex_unlock    = d->delegate->mutex_unlock,
      .cond_create     = dst_cond_create,
      .cond_free       = d->delegate->cond_free,
      .cond_wait       = d->delegate->cond_wait,
      .cond_timed_wait = d->delegate->cond_timed_wait,
      .cond_signal     = d->delegate->cond_signal,
      .cond_broadcast  = d->delegate->cond_broadcast,
  };
}

struct i_file_system
dst_filesystem (struct dst_data *d)
{
  return (struct i_file_system){
      .table = &d->vtable,
      .data  = d,
  };
}

struct i_mem
dst_mem (struct dst_data *d)
{
  return (struct i_mem){
      .table = &d->vtable,
      .data  = d,
  };
}

struct i_threading
dst_threading (struct dst_data *d)
{
  return (struct i_threading){
      .table = &d->vtable,
      .data  = d,
  };
}

/******************************************************************************
 * SECTION: Tests
 ******************************************************************************/

TEST (dst_malloc_passthrough_when_prob_zero)
{
  (void)mem;
  (void)fs;
  error           e = error_create ();

  struct dst_data d;
  dst_data_init (&d, NULL);
  struct i_mem dmem = dst_mem (&d);

  u32         *p    = i_malloc (dmem, 4, sizeof *p, &e);
  test_assert (p != NULL);
  test_assert_int_equal (e.cause_code, SUCCESS);

  i_free (dmem, p);
}

TEST (dst_malloc_always_fails_when_prob_one)
{
  (void)mem;
  (void)fs;
  error           e = error_create ();

  struct dst_data d;
  dst_data_init (&d, NULL);
  d.memory.malloc_fail_prob = 1.0f;
  struct i_mem dmem         = dst_mem (&d);

  void        *p            = i_malloc (dmem, 4, sizeof (u32), &e);
  test_assert_equal (p, NULL);
  test_assert_int_equal (e.cause_code, ERR_NOMEM);
}

TEST (dst_filesystem_open_r_always_fails_when_prob_one)
{
  (void)mem;
  (void)fs;
  error           e = error_create ();

  struct dst_data d;
  dst_data_init (&d, NULL);
  d.filesystem.open_r_fail_prob = 1.0f;
  struct i_file_system dfs      = dst_filesystem (&d);

  i_file               fp       = {0};
  test_assert (i_open_r (dfs, &fp, "dst_test_nonexistent", &e) != SUCCESS);
  test_assert_int_equal (e.cause_code, ERR_IO);
}

TEST (dst_read_short_still_reads_everything)
{
  error e = error_create ();

  test_fail_if (i_remove_quiet (fs, "dst_test_short_read", &e));

  u8 written[64];
  for (u32 i = 0; i < sizeof (written); i++) {
    written[i] = (u8)i;
  }

  i_file wfp = {0};
  test_fail_if (i_open_w (fs, &wfp, "dst_test_short_read", &e));
  test_fail_if (i_write_all (&wfp, written, sizeof (written), &e));
  test_fail_if (i_close (&wfp, &e));

  struct dst_data d;
  dst_data_init (&d, NULL);
  d.file.read_short_prob   = 1.0f; // every underlying read() is short
  struct i_file_system dfs = dst_filesystem (&d);

  i_file               rfp = {0};
  test_fail_if (i_open_r (dfs, &rfp, "dst_test_short_read", &e));

  u8        got[64] = {0};
  // read_all retries until everything requested has arrived, even though
  // every individual syscall underneath is short.
  const i64 n       = i_read_all (&rfp, got, sizeof (got), &e);
  test_assert_int_equal ((int)n, (int)sizeof (got));
  test_assert (memcmp (got, written, sizeof (got)) == 0);

  test_fail_if (i_close (&rfp, &e));
  test_fail_if (i_remove_quiet (fs, "dst_test_short_read", &e));
}

TEST (dst_read_always_fails_when_prob_one)
{
  error e = error_create ();

  test_fail_if (i_remove_quiet (fs, "dst_test_read_fail", &e));

  i_file wfp = {0};
  test_fail_if (i_open_w (fs, &wfp, "dst_test_read_fail", &e));
  u8 byte = 1;
  test_fail_if (i_write_all (&wfp, &byte, 1, &e));
  test_fail_if (i_close (&wfp, &e));

  struct dst_data d;
  dst_data_init (&d, NULL);
  d.file.read_fail_prob    = 1.0f;
  struct i_file_system dfs = dst_filesystem (&d);

  i_file               rfp = {0};
  test_fail_if (i_open_r (dfs, &rfp, "dst_test_read_fail", &e));

  u8        got[1] = {0};
  const i64 n      = i_read_all (&rfp, got, 1, &e);
  test_assert (n < 0);
  test_assert_int_equal (e.cause_code, ERR_IO);

  test_fail_if (i_close (&rfp, &e));
  test_fail_if (i_remove_quiet (fs, "dst_test_read_fail", &e));
}

#else

typedef int make_compiler_happy_empty_translation;

#endif // TESTING
