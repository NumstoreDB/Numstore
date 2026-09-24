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

#include "nscore/nsdb/ns_nsdb.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_slab_alloc.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"

#include <stdio.h>

struct nsdb *
nsdb_open_with_resources (const char *path, struct i_mem mem, struct i_file_system fs, error *e)
{
  struct nsdb *ret = i_malloc (mem, 1, sizeof *ret, e);

  if (ret == NULL) {
    return NULL;
  }

  // Trivial initializers
  slab_alloc_init (&ret->txn_alloc, mem, sizeof (struct txn), 512);
  ret->mem       = mem;
  ret->fs        = fs;
  ret->path.data = NULL;
  ret->p         = NULL;

  // Path
  ret->path.len  = strlen (path);
  ret->path.data = i_malloc (mem, ret->path.len, 1, e);
  if (ret->path.data == NULL) {
    goto failed;
  }

  // Pager
  ret->p = pgr_open (path, mem, fs, e);
  if (ret->p == NULL) {
    goto failed;
  }

  // Launch the checkpoint writer thread
  if (pgr_launch_checkpoint_thread (ret->p, 5000, e)) {
    goto failed;
  }

  return ret;

failed:
  if (ret->p) {
    pgr_close (ret->p, e);
  }
  i_free (mem, (void *)ret->path.data);
  i_free (mem, ret);
  pgr_delete_single_file (path, e);
  return NULL;
}

int
nsdb_cleanup (const char *path, error *e)
{
  pgr_delete_single_file (path, e);
  return error_trace (e);
}

err_t
nsdb_close (struct nsdb *n, error *e)
{
  err_t ret = pgr_close (n->p, e);
  slab_alloc_destroy (&n->txn_alloc);

  struct i_mem mem = n->mem;
  i_free (mem, (void *)n->path.data);
  i_free (mem, n);

  return ret;
}

err_t
nsdb_crash (struct nsdb *n, error *e)
{
  err_t err = pgr_crash (n->p, e);
  slab_alloc_destroy (&n->txn_alloc);

  struct i_mem mem = n->mem;
  i_free (mem, (void *)n->path.data);
  i_free (mem, n);

  return err;
}

struct txn *
nsdb_begin (struct nsdb *db)
{
  struct txn *tx = slab_alloc_alloc (&db->txn_alloc, &db->e);
  if (tx == NULL) {
    return NULL;
  }

  if (pgr_begin_txn (tx, db->p, &db->e)) {
    slab_alloc_free (&db->txn_alloc, tx);
    return NULL;
  }

  return tx;
}

err_t
nsdb_commit (struct nsdb *db, struct txn *tx)
{
  if (pgr_commit (db->p, tx, &db->e)) {
    slab_alloc_free (&db->txn_alloc, tx);
    return error_trace (&db->e);
  }

  slab_alloc_free (&db->txn_alloc, tx);
  return SUCCESS;
}

err_t
nsdb_rollback (struct nsdb *db, struct txn *tx)
{
  if (pgr_rollback (db->p, tx, 0, &db->e)) {
    slab_alloc_free (&db->txn_alloc, tx);
    return error_trace (&db->e);
  }

  slab_alloc_free (&db->txn_alloc, tx);
  return SUCCESS;
}

struct nsdb_var *
nsdb_var_create (struct i_mem mem, error *e)
{
  struct nsdb_var *ret = i_malloc (mem, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }
  arena_alloc_create_default (&ret->alloc);

  ret->var = (struct variable){0};
  ret->mem = mem;

  return ret;
}

void
nsdb_var_free (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);

  // Then free everything in the arena allocator
  arena_alloc_free_all (&var->alloc);

  // Free the container
  i_free (var->mem, var);
}

struct arena_alloc *
nsdb_var_alloc (struct nsdb_var *var)
{
  return &var->alloc;
}

struct variable *
nsdb_var_var (struct nsdb_var *var)
{
  return &var->var;
}

b_size
nsdb_var_len (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);
  return var->var.nbytes / type_byte_size (var->var.dtype);
}

b_size
nsdb_var_nbytes (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);
  return var->var.nbytes;
}

pgno
nsdb_var_var_root (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);
  return var->var.var_root;
}

pgno
nsdb_var_rpt_root (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);
  return var->var.rpt_root;
}

struct string
nsdb_var_name (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);
  return var->var.vname;
}

struct type *
nsdb_var_type (struct nsdb_var *var)
{
  DBG_ASSERT (nsdb_variable, var);
  return var->var.dtype;
}

struct ns_plan *
ns_plan_create (struct nsdb *db, const char *query)
{
  DBG_ASSERT (nsdb, db);

  // Allocate return value
  struct ns_plan *ret = slab_alloc_alloc (&db->plan_alloc, &db->e);
  if (ret == NULL) {
    return NULL;
  }

  ret->p = db->p;
  arena_alloc_create_default (&ret->alloc);

  // Compile the query
  if (compile_query (&ret->q, query, &ret->alloc, &db->e) < 0) {
    slab_alloc_free (&db->plan_alloc, ret);
    return NULL;
  }

  DBG_ASSERT (ns_plan, ret);

  return ret;
}

void
ns_plan_free (struct nsdb *db, struct ns_plan *plan)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_plan, plan);

  arena_alloc_free_all (&plan->alloc);
  slab_alloc_free (&db->plan_alloc, plan);
}

#ifdef TESTING
TEST (ns_plan_create)
{
  error e = error_create ();
  nsdb_cleanup ("./test.db", &e);
  struct nsdb *db = nsdb_open_with_resources ("./test.db", mem, fs, &e);

  TEST_CASE ("Successfully create a plan")
  {
    struct ns_plan *plan = ns_plan_create (db, "create foo u32");
    DBG_ASSERT (ns_plan, plan);
    ns_plan_free (db, plan);
  }

  TEST_CASE ("Fail to create a plan form invalid query")
  {
    struct ns_plan *plan = ns_plan_create (db, "create foo INVALID");
    test_assert (plan == NULL);
  }

  nsdb_close (db, &e);
}
#endif

struct nsdb_var *
ns_plan_get_var (struct ns_plan *ns, struct txn *tx)
{
  DBG_ASSERT (ns_plan, ns);
  DBG_ASSERT (ns_txn, tx);

  // Get the variable name of interest
  struct string name;
  if (query_vname_of_interest (&name, &ns->q, ns->e) < 0) {
    return NULL;
  }

  struct nsdb_var *var = nsdb_var_create (ns->mem, ns->e);
  if (var == NULL) {
    return NULL;
  }

  // Get the variable
  struct arena_alloc *alloc = nsdb_var_alloc (var);
  struct variable    *dest  = nsdb_var_var (var);
  err_t               err   = numstore_get (ns->p, tx, false, name, alloc, dest, ns->e);

  if (err < 0) {
    nsdb_var_free (var);
    return NULL;
  }

  return var;
}

#ifdef TESTING
TEST (ns_plan_get_var)
{
  error e = error_create ();
  nsdb_cleanup ("./test.db", &e);
  struct nsdb *db = nsdb_open_with_resources ("./test.db", mem, fs, &e);
  struct txn  *tx = nsdb_begin (db);

  TEST_CASE ("Successfully get a variable")
  {
    test_assert (nsdb_exec (db, tx, "create foo u32") == SUCCESS);

    struct nsdb_var *var = nsdb_get_var (db, tx, "get foo");
    test_assert (var != NULL);
    test_assert (type_equal (nsdb_var_type (var), &TU32));

    nsdb_var_free (var);
  }

  TEST_CASE ("Randomly create and get variables")
  {
    ALLOC_INIT (temp);

    int i = 0;
    while (i < 100) {
      struct type *t = type_random (&temp, 5, 4096, &e);
      if (t == NULL) {
        continue;
      }

      const char *tstr = type_tostr (&temp, t, &e);
      test_assert (tstr != NULL);

      int n = snprintf (NULL, 0, "create foo %s", tstr);
      test_assert (n > 0);

      char *buffer = i_malloc (mem, n, 1, &e);
      test_assert (buffer != NULL);

      // Create variable
      snprintf (buffer, 0, "create foo %s", tstr);
      switch (nsdb_exec (db, tx, buffer)) {
        case SUCCESS: {
          break;
        }
        case ERR_DUPLICATE_VARIABLE: {
          i_free (mem, buffer);
        }
      }

      // Get the variable
      struct nsdb_var *var = nsdb_get_var (db, tx, "get foo");
      test_assert (var != NULL);
      test_assert (type_equal (nsdb_var_type (var), t));

      nsdb_var_free (var);
      i_free (mem, buffer);
    }

    ALLOC_CLOSE (temp);
  }

  nsdb_commit (db, tx);
  nsdb_close (db, &e);
}
#endif

sb_size
ns_plan_read (struct ns_plan *st, struct txn *tx, void *dest, b_size dlen)
{
  DBG_ASSERT (ns_plan, st);
  DBG_ASSERT (ns_txn, tx);

  switch (st->q.type) {
    case QT_READ: {
      // Validate inputs
      if (dest == NULL || dlen == 0) {
        return error_causef (
            st->e,
            ERR_INVALID_ARGUMENT,
            "destination buffer is reqiured for read query"
        );
      }

      // Get interested variable
      struct nsdb_var *_var = ns_plan_get_var (st, tx);
      if (_var == NULL) {
        return error_trace (st->e);
      }
      struct variable       *var = nsdb_var_var (_var);

      // construct output stream
      struct stream          stream;
      struct stream_obuf_ctx octx;
      stream_obuf_init (&stream, &octx, dest, dlen);

      // Execute read
      return numstore_read (st->p, tx, var, st->q.read.ustr, &stream, st->e);
    }
    case QT_REMOVE: {
      // Get interested variable
      struct nsdb_var *_var = ns_plan_get_var (st, tx);
      if (_var == NULL) {
        return error_trace (st->e);
      }
      struct variable *var = nsdb_var_var (_var);

      // Optionally create a stream
      struct stream    _stream;
      struct stream   *stream = NULL;
      if (dest != NULL) {
        struct stream_obuf_ctx octx;
        stream_obuf_init (&_stream, &octx, dest, dlen);
        stream = &_stream;
      }

      // Execute read
      return numstore_remove (st->p, tx, var, st->q.remove.ustr, stream, st->e);
    }
    default: {
      return error_causef (
          st->e,
          ERR_INVALID_ARGUMENT,
          "Can only read query types of read and remove"
      );
    }
  }
}

#ifdef TESTING
TEST (ns_plan_read)
{
  error e = error_create ();
  nsdb_cleanup ("./test.db", &e);
  struct nsdb *db = nsdb_open_with_resources ("./test.db", mem, fs, &e);
  struct txn  *tx = nsdb_begin (db);

  // Seed database
  u32          src[10];
  rand_bytes (src, sizeof (src));
  nsdb_exec (db, tx, "create foo u32");
  nsdb_write (db, tx, src, sizeof (src), "insert foo 0 10");

  //////////// READ

  TEST_CASE ("Read a variable successfully")
  {
    u32 dest[10];
    nsdb_read (db, tx, dest, sizeof (dest), "read foo[0:]");
    test_assert_memequal (src, dest, sizeof (src));
  }

  TEST_CASE ("Read request more in a smaller buffer")
  {
    u32     dest[2];
    sb_size len = nsdb_read (db, tx, dest, sizeof (dest), "read foo[0:]");
    test_assert_int_equal (len, 2);
    test_assert_memequal (src, dest, sizeof (dest));
  }

  TEST_CASE ("Read a non existent variable")
  {
    u32     dest[10];
    sb_size ret = nsdb_read (db, tx, dest, sizeof (dest), "read biz[0:]");
    test_assert_int_equal (ret, ERR_VARIABLE_NE);
    error_reset (&e);
  }

  //////////// REMOVE

  TEST_CASE ("Remove a variable successfully")
  {
    u32     removed[5];
    u32     remaining[5];
    u32     removed_expected[]   = {src[0], src[2], src[4], src[6], src[8]};
    u32     remaining_expected[] = {src[1], src[3], src[5], src[7], src[9]};

    sb_size len_removed   = nsdb_read (db, tx, removed, sizeof (removed), "remove foo[0::2]");
    sb_size len_remaining = nsdb_read (db, tx, remaining, sizeof (remaining), "read foo[0:]");

    test_assert_int_equal (len_removed, 5);
    test_assert_int_equal (len_remaining, 5);

    test_assert_memequal (removed_expected, removed, sizeof (removed));
    test_assert_memequal (remaining_expected, remaining, sizeof (remaining));
  }

  TEST_CASE ("Remove request more in a smaller buffer")
  {
    u32     removed[2];
    u32     remaining[3];
    u32     removed_expected[]   = {src[1], src[5]};
    u32     remaining_expected[] = {src[3], src[7], src[9]};

    sb_size len_removed   = nsdb_read (db, tx, removed, sizeof (removed), "remove foo[0::2]");
    sb_size len_remaining = nsdb_read (db, tx, remaining, sizeof (remaining), "read foo[0:]");

    test_assert_int_equal (len_removed, 2);
    test_assert_int_equal (len_remaining, 3);

    test_assert_memequal (removed_expected, removed, sizeof (removed));
    test_assert_memequal (remaining_expected, remaining, sizeof (remaining));
  }

  TEST_CASE ("Remove a non existent variable")
  {
    u32     dest[10];
    sb_size ret = nsdb_read (db, tx, dest, sizeof (dest), "remove biz[0:]");
    test_assert_int_equal (ret, ERR_VARIABLE_NE);
    error_reset (&e);
  }

  nsdb_commit (db, tx);
  nsdb_close (db, &e);
}
#endif

void *
ns_plan_read_malloc (struct ns_plan *st, struct txn *tx, b_size *dlen)
{
  DBG_ASSERT (ns_plan, st);
  DBG_ASSERT (ns_txn, tx);

  switch (st->q.type) {
    case QT_READ: {
      // Get interested variable
      struct nsdb_var *_var = ns_plan_get_var (st, tx);
      if (_var == NULL) {
        return NULL;
      }
      struct variable *var = nsdb_var_var (_var);

      return numstore_read_malloc (st->p, tx, var, st->q.read.ustr, dlen, st->mem, st->e);
    }
    case QT_REMOVE: {
      // Get interested variable
      struct nsdb_var *_var = ns_plan_get_var (st, tx);
      if (_var == NULL) {
        return NULL;
      }
      struct variable *var = nsdb_var_var (_var);

      return numstore_remove_malloc (st->p, tx, var, st->q.remove.ustr, dlen, st->mem, st->e);
    }
    default: {
      error_causef (st->e, ERR_INVALID_ARGUMENT, "Can only read query types of read and remove");
      return NULL;
    }
  }
}

#ifdef TESTING
TEST (ns_plan_read_malloc)
{
  error e = error_create ();
  nsdb_cleanup ("./test.db", &e);
  struct nsdb *db = nsdb_open_with_resources ("./test.db", mem, fs, &e);
  struct txn  *tx = nsdb_begin (db);

  // Seed database
  u32          src[10];
  rand_bytes (src, sizeof (src));
  nsdb_exec (db, tx, "create foo u32");
  nsdb_write (db, tx, src, sizeof (src), "insert foo 0 10");

  //////////// READ

  TEST_CASE ("Read a variable successfully")
  {
    b_size len;
    void  *dest = nsdb_read_malloc (db, tx, &len, "read foo[0:]");
    test_assert_memequal (src, dest, sizeof (src));
    test_assert_int_equal (len, 10);
    i_free (mem, dest);
  }

  TEST_CASE ("Read a non existent variable")
  {
    b_size len  = 123;
    void  *dest = nsdb_read_malloc (db, tx, &len, "read biz[0:]");
    test_err_t_check (e.cause_code, ERR_VARIABLE_NE, &e);
    test_assert (dest == NULL);
    test_assert_int_equal (len, 123);
  }

  //////////// REMOVE

  TEST_CASE ("Remove a variable successfully")
  {
    u32    removed_expected[]   = {src[0], src[2], src[4], src[6], src[8]};
    u32    remaining_expected[] = {src[1], src[3], src[5], src[7], src[9]};

    b_size len_removed;
    b_size len_remaining;
    void  *removed   = nsdb_read_malloc (db, tx, &len_removed, "remove foo[0::2]");
    void  *remaining = nsdb_read_malloc (db, tx, &len_remaining, "read foo[0:]");

    test_assert_int_equal (len_removed, 5);
    test_assert_int_equal (len_remaining, 5);

    test_assert_memequal (removed_expected, removed, sizeof (removed));
    test_assert_memequal (remaining_expected, remaining, sizeof (remaining));

    i_free (mem, removed);
    i_free (mem, remaining);
  }

  TEST_CASE ("Remove a non existent variable")
  {
    b_size len  = 123;
    void  *dest = nsdb_read_malloc (db, tx, &len, "remove biz[0:]");
    test_err_t_check (e.cause_code, ERR_VARIABLE_NE, &e);
    test_assert (dest == NULL);
    test_assert_int_equal (len, 123);
  }

  nsdb_commit (db, tx);
  nsdb_close (db, &e);
}
#endif

sb_size
ns_plan_write (struct ns_plan *st, struct txn *tx, const void *src, b_size dlen)
{
  DBG_ASSERT (ns_plan, st);
  DBG_ASSERT (ns_txn, tx);

  if (src == NULL || dlen == 0) {
    return error_causef (st->e, ERR_INVALID_ARGUMENT, "source buffer is required for write query");
  }

  // Get variable of interest
  struct nsdb_var *_var = ns_plan_get_var (st, tx);
  if (_var == NULL) {
    return error_trace (st->e);
  }
  struct variable       *var = nsdb_var_var (_var);

  struct stream          stream;
  struct stream_ibuf_ctx ictx;
  stream_ibuf_init (&stream, &ictx, src, dlen);

  switch (st->q.type) {
    case QT_INSERT: {
      return numstore_insert (st->p, tx, var, st->q.insert.ofst, st->q.insert.len, &stream, st->e);
    }
    case QT_WRITE: {
      return numstore_write (st->p, tx, var, st->q.write.ustr, &stream, st->e);
    }
    default: {
      return error_causef (
          st->e,
          ERR_INVALID_ARGUMENT,
          "Can only write query types of write and insert"
      );
    }
  }
}

#ifdef TESTING

#  define check_nbytes(db, tx, vname, expected_bytes)                \
    do {                                                             \
      struct nsdb_var *var = nsdb_get_var (db, tx, "get " vname);    \
      test_assert (var != NULL);                                     \
      test_assert_int_equal (nsdb_var_nbytes (var), expected_bytes); \
      nsdb_var_free (var);                                           \
    }                                                                \
    while (0)

TEST (ns_plan_write)
{
  error e = error_create ();
  nsdb_cleanup ("./test.db", &e);
  struct nsdb *db = nsdb_open_with_resources ("./test.db", mem, fs, &e);
  struct txn  *tx = nsdb_begin (db);

  // Seed database
  nsdb_exec (db, tx, "create foo u32");
  check_nbytes (db, tx, "foo", 0);

  //////////// Insert

  TEST_CASE ("Insert a variable successfully")
  {
    u32 src[10];
    arr_range (src);

    nsdb_write (db, tx, src, sizeof (src), "insert foo 0 10");
    check_nbytes (db, tx, "foo", sizeof (src));

    nsdb_write (db, tx, src, sizeof (src), "insert foo 0 10");
    check_nbytes (db, tx, "foo", 2 * sizeof (src));

    u32 dest[20];
    nsdb_read (db, tx, dest, sizeof (dest), "read foo[0:]");
    test_assert_memequal (src, dest, sizeof (src));
    test_assert_memequal (src, &dest[10], sizeof (src));
  }

  TEST_CASE ("Insert a non existent variable")
  {
    u32 src[10];
    arr_range (src);
    nsdb_write (db, tx, src, sizeof (src), "insert bar 0 10");
    test_err_t_check (e.cause_code, ERR_VARIABLE_NE, &e);
  }

  //////////// WRITE

  TEST_CASE ("Insert a non existent variable")
  {
    // Overwrite with random data
    u32 src[20];
    rand_bytes (src, sizeof (src));
    nsdb_write (db, tx, src, sizeof (src), "write bar[0:]");

    u32     dest[40]; // Bigger buffer to show that only 20 are read
    sb_size len = nsdb_read (db, tx, dest, sizeof (dest), "read foo[0:]");
    test_assert_int_equal (len, 20);
    test_assert_memequal (src, dest, sizeof (src));
  }

  nsdb_commit (db, tx);
  nsdb_close (db, &e);
}
#endif

////////////////////////////////// Execute in console

/**
static inline sb_size
console_qt_read (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  sb_size ret = nsdb_read_and_print (ns, &q->read, alc, e);
  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

static inline sb_size
console_qt_write (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_remove (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_insert (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_create (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  struct txn *tx = nsdb_begin (ns, e);
  if (tx == NULL) {
    return error_trace (e);
  }

  if (numstore_create (ns->p, tx, q->create.name, q->create.type, alc, NULL, e) < 0) {
    return error_trace (e);
  }

  if (nsdb_commit (ns, tx, e) < 0) {
    return error_trace (e);
  }

  printf ("{ \"Status\" : \"Ok\" }\n");

  return SUCCESS;
}

static inline sb_size
console_qt_delete (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_get (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  sb_size ret = nsdb_get_and_print (ns, &q->get, alc, e);
  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

static inline sb_size
console_qt_exit (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_help (void)
{
  return SUCCESS;
}
*/

err_t
ns_planute_in_console (struct ns_plan *ns, struct txn *tx, error *e)
{
  /**
  switch (q->type) {
    case QT_READ: {
      return console_qt_read (ns, q, alc, e);
    }
    case QT_WRITE: {
      return console_qt_write ();
    }
    case QT_REMOVE: {
      return console_qt_remove ();
    }
    case QT_INSERT: {
      return console_qt_insert ();
    }
    case QT_CREATE: {
      return console_qt_create (ns, q, alc, e);
    }
    case QT_DELETE: {
      return console_qt_delete ();
    }
    case QT_GET: {
      return console_qt_get (ns, q, alc, e);
    }
    case QT_EXIT: {
      return console_qt_exit ();
    }
    case QT_HELP: {
      return console_qt_help ();
    }
  }
  */

  return SUCCESS;
}

err_t
nsdb_exec (struct nsdb *db, struct txn *tx, const char *query)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, tx);
  ASSERT (query);

  struct ns_plan *plan = ns_plan_create (db, query);
  if (plan == NULL) {
    return error_trace (&db->e);
  }

  sb_size ret = ns_plan_execute (plan, tx);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

struct nsdb_var *
nsdb_get_var (struct nsdb *db, struct txn *tx, const char *query)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, tx);
  ASSERT (query);

  struct ns_plan *plan = ns_plan_create (db, query);
  if (plan == NULL) {
    return NULL;
  }

  struct nsdb_var *var = ns_plan_get_var (plan, tx);
  if (var == NULL) {
    ns_plan_free (db, plan);
    return NULL;
  }

  ns_plan_free (db, plan);

  return var;
}

sb_size
nsdb_read (struct nsdb *db, struct txn *txn, void *dest, b_size dlen, const char *query)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  ASSERT (dest);
  ASSERT (dlen > 0);

  struct ns_plan *plan = ns_plan_create (db, query);
  if (plan == NULL) {
    return error_trace (&db->e);
  }

  sb_size ret = ns_plan_read (plan, txn, dest, dlen);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

void *
nsdb_read_malloc (struct nsdb *db, struct txn *txn, b_size *dlen, const char *query)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  ASSERT (dlen);

  struct ns_plan *plan = ns_plan_create (db, query);
  if (plan == NULL) {
    return NULL;
  }

  void *data = ns_plan_read_malloc (plan, txn, dlen);
  if (data == NULL) {
    ns_plan_free (db, plan);
    return NULL;
  }

  ns_plan_free (db, plan);

  return data;
}

sb_size
nsdb_write (struct nsdb *db, struct txn *txn, const void *src, b_size dlen, const char *query)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  ASSERT (src);
  ASSERT (dlen > 0);

  struct ns_plan *plan = ns_plan_create (db, query);
  if (plan == NULL) {
    return error_trace (&db->e);
  }

  sb_size ret = ns_plan_write (plan, txn, src, dlen);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

err_t
nsdb_console (struct nsdb *db, struct txn *txn, const char *query)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);

  struct ns_plan *plan = ns_plan_create (db, query);
  if (plan == NULL) {
    return error_trace (&db->e);
  }

  err_t ret = ns_planute_in_console (plan, txn, &db->e);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}
