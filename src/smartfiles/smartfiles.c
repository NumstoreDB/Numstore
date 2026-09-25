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

#include "smartfiles/smartfiles.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stream.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/smartfiles/ns_smartfiles_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/txn_table/ns_txn_table.h"

#include <stdbool.h>
#include <string.h>

int
smfile_perror (smfile_t *db, const char *prefix)
{
  const char *err = smfile_strerror (db);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, err);
  }
  return fprintf (stderr, "%s: success\n", prefix);
}

#ifdef TESTING
TEST (smfile_perror)
{
  smfile_cleanup ("test");

  smfile_t *s = smfile_open ("test");
  u8        buffer[2048];

  // stride == 0 => ERROR
  test_assert (smfile_read (s, NULL, buffer, 10, 0, 0, 10) < 0);
  test_assert (smfile_perror (s, "bar") > 0);

  smfile_close (s);
}
#endif

const char *
smfile_strerror (smfile_t *db)
{
  if (db->e.cause_code < 0) {
    // Consume
    error_reset (&db->e);
    return db->e.cause_msg;
  }
  return NULL;
}

#ifdef TESTING
TEST (smfile_strerror)
{
  smfile_cleanup ("test");

  smfile_t *s = smfile_open ("test");
  u8        buffer[2048];

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

  smfile_t *s = smfile_open ("test");
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
smfile_size (smfile_t *db, sm_txn_t *tx)
{
  CHECK_UNHANDLED_ERROR (&db->e);

  ALLOC_INIT (temp);
  sb_size ret;
  WITH_AUTO_TXN (ret, db->db, tx, smartfiles_size (db->db->p, tx, &temp, &db->e), &db->e);
  ALLOC_CLOSE (temp);

  return ret;
}

int
smfile_close (smfile_t *db)
{
  int ret = nsdb_close (db->db, &db->e);
  i_free (default_mem (), db);
  return ret;
}

#ifdef TESTING
TEST (smfile_close)
{
  smfile_cleanup ("test");

  error     e = error_create ();
  smfile_t *s = smfile_open ("test");
  smfile_close (s);

  bool exists;
  i_file_exists (fs, "test", &exists, &e);
  test_assert (exists);
  i_file_exists (fs, "test.wal", &exists, &e);
  test_assert (!exists);
}
#endif

int
smfile_crash (smfile_t *db)
{
  int ret = nsdb_crash (db->db, &db->e);
  i_free (default_mem (), db);
  return ret;
}

#ifdef TESTING
TEST (smfile_crash)
{
  smfile_cleanup ("test");

  error     e = error_create ();
  smfile_t *s = smfile_open ("test");
  smfile_crash (s);

  bool exists;
  i_file_exists (fs, "test", &exists, &e);
  test_assert (exists);
  i_file_exists (fs, "test.wal", &exists, &e);
  test_assert (exists);
}
#endif

struct txn *
smfile_begin (smfile_t *db)
{
  return nsdb_begin (db->db, &db->e);
}

int
smfile_commit (smfile_t *db, struct txn *tx)
{
  return nsdb_commit (db->db, tx, &db->e);
}

int
smfile_rollback (smfile_t *db, struct txn *tx)
{
  return nsdb_rollback (db->db, tx, &db->e);
}

#ifdef TESTING
TEST (smfile)
{
  smfile_cleanup ("test");

  u8        buffer[2048];
  smfile_t *s = smfile_open ("test");

  test_assert_equal (smfile_size (s, NULL), 0);

  struct txn *tx = smfile_begin (s);
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

  ret->db = nsdb_open_with_resources (path, default_mem (), default_filesystem (), &ret->e);
  if (ret == NULL) {
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

  smfile_t *s = smfile_open ("test");
  test_assert (s != NULL);
  test_assert_equal (smfile_size (s, NULL), 0);

  smfile_close (s);

  // Reopening an existing file should succeed and preserve its data.
  smfile_t *s2 = smfile_open ("test");
  test_assert (s2 != NULL);
  test_assert_equal (smfile_size (s2, NULL), 0);

  smfile_close (s2);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Insert

sb_size
smfile_insert (smfile_t *db, struct txn *tx, const void *src, sb_size bofst, b_size slen)
{
  CHECK_UNHANDLED_ERROR (&db->e);

  ALLOC_INIT (temp);
  sb_size ret;
  istream_create_from (input, src, slen);
  WITH_AUTO_TXN (
      ret,
      db->db,
      tx,
      smartfiles_insert (db->db->p, tx, &input, bofst, slen, &temp, &db->e),
      &db->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}

sb_size
smfile_read (
    smfile_t   *db,
    struct txn *tx,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  CHECK_UNHANDLED_ERROR (&db->e);

  ALLOC_INIT (temp);
  sb_size ret;
  ostream_create_from (output, dest, size * nelem);
  WITH_AUTO_TXN (
      ret,
      db->db,
      tx,
      smartfiles_read (db->db->p, tx, &output, size, bofst, stride, nelem, &temp, &db->e),
      &db->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}

sb_size
smfile_remove (
    smfile_t   *db,
    struct txn *tx,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  CHECK_UNHANDLED_ERROR (&db->e);

  ALLOC_INIT (temp);
  sb_size ret;
  ostream_create_from (output, dest, size * nelem);
  WITH_AUTO_TXN (
      ret,
      db->db,
      tx,
      smartfiles_remove (db->db->p, tx, &output, size, bofst, stride, nelem, &temp, &db->e),
      &db->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}

sb_size
smfile_write (
    smfile_t   *db,
    struct txn *tx,
    const void *src,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  CHECK_UNHANDLED_ERROR (&db->e);

  ALLOC_INIT (temp);
  sb_size ret;
  istream_create_from (input, src, nelem * size);
  WITH_AUTO_TXN (
      ret,
      db->db,
      tx,
      smartfiles_write (db->db->p, tx, &input, size, bofst, stride, nelem, &temp, &db->e),
      &db->e
  );
  ALLOC_CLOSE (temp);

  return ret;
}
