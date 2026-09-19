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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stream.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/smartfiles/ns_smartfiles_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "smartfiles/smartfiles.h"

#include <stdbool.h>
#include <string.h>

struct smfile
{
  struct nsdb *db;
  error        e;
};

int
smfile_perror (smfile_t *smf, const char *prefix)
{
  const char *err = smfile_strerror (smf);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, err);
  }
  return fprintf (stderr, "%s: success\n", prefix);
}

#ifdef TESTING
TEST (smfile_perror)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  // stride == 0 => ERROR
  test_assert (smfile_read (s, NULL, buffer, 10, 0, 0, 10) < 0);
  test_assert (smfile_perror (s, "bar") > 0);

  smfile_close (s);
}
#endif

const char *
smfile_strerror (smfile_t *smf)
{
  if (smf->e.cause_code < 0) {
    // Consume
    error_reset (&smf->e);
    return smf->e.cause_msg;
  }
  return NULL;
}

#ifdef TESTING
TEST (smfile_strerror)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  // stride == 0 => ERROR
  test_assert (smfile_read (s, NULL, buffer, 10, 0, 0, 10) < 0);
  test_assert (string_contains (strfcstr (smfile_strerror (s)), strfcstr ("stride == 0")));

  smfile_close (s);
}
#endif

int
smfile_cleanup (const char *path)
{
  error e = error_create ();
  return nsdb_cleanup (path, &e);
}

#ifdef TESTING
TEST (smfile_cleanup)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  smfile_close (s);
  error e = error_create ();

  bool  exists;
  i_file_exists (fs, "test", &exists, &e);
  test_assert (exists);

  smfile_cleanup ("test");
  i_file_exists (fs, "test", &exists, &e);
  test_assert (!exists);
}
#endif

sb_size
smfile_size (smfile_t *smf, sm_txn_t *tx)
{
  CHECK_UNHANDLED_ERROR (&smf->e);

  ALLOC_INIT (temp);
  sb_size ret;
  WITH_AUTO_TXN (ret, smf->db, tx, smartfiles_size (smf->db->p, tx, &temp, &smf->e), &smf->e);
  ALLOC_CLOSE (temp);

  return ret;
}

int
smfile_close (smfile_t *smf)
{
  int ret = nsdb_close (smf->db, &smf->e);
  i_free (default_mem (), smf);
  return ret;
}

#ifdef TESTING
TEST (smfile_close)
{
  smfile_cleanup ("test");

  error          e = error_create ();
  struct smfile *s = smfile_open ("test");
  smfile_close (s);

  bool exists;
  i_file_exists (fs, "test", &exists, &e);
  test_assert (exists);
  i_file_exists (fs, "test.wal", &exists, &e);
  test_assert (!exists);
}
#endif

int
smfile_crash (smfile_t *smf)
{
  int ret = nsdb_crash (smf->db, &smf->e);
  i_free (default_mem (), smf);
  return ret;
}

#ifdef TESTING
TEST (smfile_crash)
{
  smfile_cleanup ("test");

  error          e = error_create ();
  struct smfile *s = smfile_open ("test");
  smfile_crash (s);

  bool exists;
  i_file_exists (fs, "test", &exists, &e);
  test_assert (exists);
  i_file_exists (fs, "test.wal", &exists, &e);
  test_assert (exists);
}
#endif

struct ns_txn *
smfile_begin (smfile_t *smf)
{
  return nsdb_begin (smf->db, &smf->e);
}

int
smfile_commit (smfile_t *smf, struct ns_txn *tx)
{
  return nsdb_commit (smf->db, tx, &smf->e);
}

int
smfile_rollback (smfile_t *smf, struct ns_txn *tx)
{
  return nsdb_rollback (smf->db, tx, &smf->e);
}

#ifdef TESTING
TEST (smfile_txns)
{
  smfile_cleanup ("test");

  u8             buffer[2048];
  struct smfile *s = smfile_open ("test");

  test_assert_equal (smfile_size (s, NULL), 0);

  struct ns_txn *tx = smfile_begin (s);
  smfile_insert (s, tx, buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_size (s, tx), sizeof (buffer));
  smfile_commit (s, tx);
  test_assert_equal (smfile_size (s, NULL), sizeof (buffer));

  tx = smfile_begin (s);
  smfile_insert (s, tx, buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_size (s, tx), 2 * sizeof (buffer));
  smfile_rollback (s, tx);
  test_assert_equal (smfile_size (s, NULL), sizeof (buffer));

  smfile_close (s);
}
#endif

smfile_t *
smfile_open (const char *path)
{
  error     e   = error_create ();
  smfile_t *ret = i_malloc (default_mem (), 1, sizeof *ret, &e);
  if (ret == NULL) {
    return NULL;
  }

  ret->e  = error_create ();
  ret->db = nsdb_open_with_resources (path, default_mem (), default_filesystem (), &ret->e);
  if (ret->db == NULL) {
    i_free (default_mem (), ret);
    return NULL;
  }

  if (smartfiles_init_pager (ret->db->p, &ret->e)) {
    goto failed;
  }

  return ret;

failed:
  nsdb_close (ret->db, &ret->e);
  i_free (default_mem (), ret);

  return NULL;
}

#ifdef TESTING
TEST (smfile_open)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  test_assert (s != NULL);
  test_assert_equal (smfile_size (s, NULL), 0);

  smfile_close (s);

  // Reopening an existing file should succeed and preserve its data.
  struct smfile *s2 = smfile_open ("test");
  test_assert (s2 != NULL);
  test_assert_equal (smfile_size (s2, NULL), 0);

  smfile_close (s2);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Insert

sb_size
smfile_insert (smfile_t *smf, struct ns_txn *tx, const void *src, sb_size bofst, b_size slen)
{
  CHECK_UNHANDLED_ERROR (&smf->e);

  ALLOC_INIT (temp);
  sb_size ret;
  istream_create_from (input, src, slen);
  WITH_AUTO_TXN (
      ret,
      smf->db,
      tx,
      smartfiles_insert (smf->db->p, tx, &input, bofst, slen, &temp, &smf->e),
      &smf->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}

sb_size
smfile_read (
    smfile_t      *smf,
    struct ns_txn *tx,
    void          *dest,
    t_size         size,
    sb_size        bofst,
    sb_size        stride,
    b_size         nelem
)
{
  CHECK_UNHANDLED_ERROR (&smf->e);

  ALLOC_INIT (temp);
  sb_size ret;
  ostream_create_from (output, dest, size * nelem);
  WITH_AUTO_TXN (
      ret,
      smf->db,
      tx,
      smartfiles_read (smf->db->p, tx, &output, size, bofst, stride, nelem, &temp, &smf->e),
      &smf->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}

sb_size
smfile_remove (
    smfile_t      *smf,
    struct ns_txn *tx,
    void          *dest,
    t_size         size,
    sb_size        bofst,
    sb_size        stride,
    b_size         nelem
)
{
  CHECK_UNHANDLED_ERROR (&smf->e);

  ALLOC_INIT (temp);
  sb_size ret;
  ostream_create_from (output, dest, size * nelem);
  WITH_AUTO_TXN (
      ret,
      smf->db,
      tx,
      smartfiles_remove (smf->db->p, tx, &output, size, bofst, stride, nelem, &temp, &smf->e),
      &smf->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}

sb_size
smfile_write (
    smfile_t      *smf,
    struct ns_txn *tx,
    const void    *src,
    t_size         size,
    sb_size        bofst,
    sb_size        stride,
    b_size         nelem
)
{
  CHECK_UNHANDLED_ERROR (&smf->e);

  ALLOC_INIT (temp);
  sb_size ret;
  istream_create_from (input, src, nelem * size);
  WITH_AUTO_TXN (
      ret,
      smf->db,
      tx,
      smartfiles_write (smf->db->p, tx, &input, size, bofst, stride, nelem, &temp, &smf->e),
      &smf->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}
