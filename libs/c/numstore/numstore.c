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

#include "numstore.h"

#include "ns_arena_alloc.h"
#include "ns_error.h"
#include "ns_nsdb.h"
#include "ns_utils.h"
#include "numstore/ns_numstore_algorithms.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"
#include "smartfiles/ns_smartfiles_algorithms.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static nsdb_t *
ns_open_common (const char *path, error *e)
{
  struct i_mem mem = default_mem ();

  // The public API owns the OS it hands to the database - ns_close/ns_crash
  // free it again
  struct i_os  os;
  if (system_os_create (mem, &os, e) < 0) {
    return NULL;
  }

  struct nsdb *db = nsdb_open (path, mem, os, e);

  if (db == NULL) {
    system_os_free (os);
    return NULL;
  }

  if (numstore_init_pager (db->p, e) < 0) {
    nsdb_close (db, e);
    system_os_free (os);
    return NULL;
  }

  nsdb_allow_auto_txn (db);

  return db;
}

// Copies the message of [e] into [erstr] and null terminates it
static void
ns_copy_errstr (char erstr[], unsigned elen, const error *e)
{
  if (elen == 0) {
    return;
  }

  const u32 len = MIN (e->cmlen, elen - 1);
  memcpy (erstr, e->cause_msg, len);
  erstr[len] = '\0';
}

nsdb_t *
ns_open (const char *path)
{
  error e = error_create ();
  return ns_open_common (path, &e);
}

nsdb_t *
ns_open_with_error (const char *path, char erstr[], unsigned elen)
{
  error   e   = error_create ();
  nsdb_t *ret = ns_open_common (path, &e);
  if (ret == NULL) {
    ns_copy_errstr (erstr, elen, &e);
    return NULL;
  }
  return ret;
}

static nsdb_t *
ns_smfile_open_common (const char *path, error *e)
{
  struct i_mem mem = default_mem ();

  // The public API owns the OS it hands to the database - ns_close/ns_crash
  // free it again
  struct i_os  os;
  if (system_os_create (mem, &os, e) < 0) {
    return NULL;
  }

  struct nsdb *db = nsdb_open (path, mem, os, e);

  if (db == NULL) {
    system_os_free (os);
    return NULL;
  }

  if (smartfiles_init_pager (db->p, e) < 0) {
    nsdb_close (db, e);
    system_os_free (os);
    return NULL;
  }

  nsdb_allow_auto_txn (db);

  return db;
}

nsdb_t *
ns_smfile_open (const char *path)
{
  error e = error_create ();
  return ns_smfile_open_common (path, &e);
}

nsdb_t *
ns_smfile_open_with_error (const char *path, char erstr[], unsigned elen)
{
  error   e   = error_create ();
  nsdb_t *ret = ns_smfile_open_common (path, &e);
  if (ret == NULL) {
    ns_copy_errstr (erstr, elen, &e);
    return NULL;
  }
  return ret;
}

int
ns_cleanup (const char *path)
{
  error        e   = error_create ();
  struct i_mem mem = default_mem ();

  struct i_os  os;
  if (system_os_create (mem, &os, &e) < 0) {
    return error_trace (&e);
  }

  const err_t ret = nsdb_cleanup (path, os, &e);
  system_os_free (os);
  return ret;
}

int
ns_close (nsdb_t *ns)
{
  // Copy out before closing - this lives inside ns
  const struct i_os os  = ns->os;

  error             e   = error_create ();
  const err_t       ret = nsdb_close (ns, &e);
  system_os_free (os);
  return ret;
}

int
ns_crash (nsdb_t *ns)
{
  // Copy out before crashing - this lives inside ns
  const struct i_os os  = ns->os;

  error             e   = error_create ();
  const err_t       ret = nsdb_crash (ns, &e);
  system_os_free (os);
  return ret;
}

// Variables
b_size
ns_var_len (nsdb_var_t *var)
{
  return nsdb_var_len (var);
}

void
ns_var_free (nsdb_var_t *var)
{
  nsdb_var_free (var);
}

// Errors
const char *
ns_strerror (nsdb_t *ns, bool copy)
{
  return nsdb_consume_error (ns, copy);
}

const char *
ns_plan_strerror (nsdb_plan_t *plan, bool copy)
{
  return nsdb_plan_consume_error (plan, copy);
}

const char *
ns_txn_strerror (nsdb_t *ns, nsdb_txn_t *txn, bool copy)
{
  return nsdb_txn_consume_error (ns, txn, copy);
}

// Transactions
nsdb_txn_t *
ns_begin (nsdb_t *ns)
{
  error       e   = error_create ();
  nsdb_txn_t *ret = nsdb_begin (ns, &e);
  if (ret == NULL) {
    (void)nsdb_report_error (ns, NULL, &e);
  }
  return ret;
}

int
ns_commit (nsdb_t *ns, nsdb_txn_t *txn)
{
  return nsdb_commit (ns, txn);
}

int
ns_rollback (nsdb_t *ns, nsdb_txn_t *txn)
{
  return nsdb_rollback (ns, txn);
}

struct nsdb_var_t_visit_ctx
{
  nsdb_var_consumer consumer;
  void             *parent_ctx;
};

static err_t
ns_visit_consumer (struct variable *_var, void *_ctx, MAYBE_UNUSED error *e)
{
  struct nsdb_var_t_visit_ctx *ctx = _ctx;
  struct nsdb_var              var = {
      .var   = *_var,
      .alloc = {0},
      .mem   = {0},
  };
  ctx->consumer (&var, ctx->parent_ctx);
  return SUCCESS;
}

int
ns_visit_nsdb_vars (nsdb_t *db, nsdb_var_consumer consumer, nsdb_txn_t *tx, void *ctx)
{
  // TODO: the walk runs outside any transaction and ignores [tx], except
  // that a failure is reported on it. Pass it through once
  // ns_visit_variables takes one, so the walk sees the caller's uncommitted
  // variables.
  error                       e       = error_create ();

  struct nsdb_var_t_visit_ctx wrapped = {
      .consumer   = consumer,
      .parent_ctx = ctx,
  };

  if (ns_visit_variables (db->p, ns_visit_consumer, &wrapped, &e) < 0) {
    return nsdb_report_error (db, tx, &e);
  }

  return SUCCESS;
}

static inline char *
query_vsnprintf (const char *fmt, va_list ap, struct arena_alloc *alloc, error *e)
{
  va_list ap2;
  va_copy (ap2, ap);

  // Compute the length the formatted query needs, without writing anything.
  i32 qlen = vsnprintf (NULL, 0, fmt, ap);
  if (qlen < 0) {
    va_end (ap2);
    error_causef (e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
    return NULL;
  }

  char *buf = arena_malloc (alloc, (size_t)qlen + 1, 1, e);
  if (buf == NULL) {
    va_end (ap2);
    return NULL;
  }

  // Actually write the formatted query into buf.
  qlen = vsnprintf (buf, (size_t)qlen + 1, fmt, ap2);
  ASSERT (qlen >= 0);
  va_end (ap2);

  return buf;
}

/////////////////////////////////////// Execution

// Formats the query of a one shot call into [alloc]. A failure is reported
// on [tx] and the database, and its code is in [code]
static char *
ns_format_query (
    nsdb_t             *db,
    nsdb_txn_t         *tx,
    const char         *fmt,
    va_list             ap,
    struct arena_alloc *alloc,
    err_t              *code
)
{
  error e     = error_create ();
  char *query = query_vsnprintf (fmt, ap, alloc, &e);
  if (query == NULL) {
    *code = nsdb_report_error (db, tx, &e);
  }
  return query;
}

nsdb_plan_t *
ns_plan_fcreate (nsdb_t *db, const char *fmt, ...)
{
  error   e = error_create ();

  va_list args;
  va_start (args, fmt);
  nsdb_plan_t *ret = nsdb_plan_vcreate (db, fmt, &e, args);
  va_end (args);

  if (ret == NULL) {
    (void)nsdb_report_error (db, NULL, &e);
  }

  return ret;
}

void
ns_plan_free (nsdb_plan_t *plan)
{
  nsdb_plan_free (plan);
}

int
ns_execute (nsdb_t *db, nsdb_txn_t *tx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  err_t code;
  char *query = ns_format_query (db, tx, fmt, ap, &temp, &code);
  va_end (ap);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return code;
  }

  err_t ret = nsdb_execute (db, tx, query);
  ALLOC_CLOSE (temp);
  return ret;
}

int
ns_plan_execute (nsdb_plan_t *plan, nsdb_txn_t *tx)
{
  // TODO: nsdb_plan_execute returns an sb_size count for QT_REMOVE, which
  // this int truncates past INT32_MAX. See the matching TODO on nsdb_execute.
  return (int)nsdb_plan_execute (plan, tx);
}

nsdb_var_t *
ns_get_var (nsdb_t *db, nsdb_txn_t *tx, const char *query, ...)
{
  va_list ap;
  va_start (ap, query);

  ALLOC_INIT (temp);
  err_t code;
  char *formatted = ns_format_query (db, tx, query, ap, &temp, &code);
  va_end (ap);
  if (formatted == NULL) {
    ALLOC_CLOSE (temp);
    return NULL;
  }

  // The returned var owns its own arena, so [temp] can go either way
  nsdb_var_t *ret = nsdb_get_var (db, tx, formatted);
  ALLOC_CLOSE (temp);

  return ret;
}

nsdb_var_t *
ns_plan_get_var (nsdb_plan_t *plan, nsdb_txn_t *tx)
{
  return nsdb_plan_get_var (plan, tx);
}

sb_size
ns_read (nsdb_t *db, nsdb_txn_t *txn, void *dest, b_size dlen, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  err_t code;
  char *query = ns_format_query (db, txn, fmt, ap, &temp, &code);
  va_end (ap);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return code;
  }

  sb_size ret = nsdb_read (db, txn, dest, dlen, query);
  ALLOC_CLOSE (temp);
  return ret;
}

sb_size
ns_plan_read (nsdb_plan_t *plan, nsdb_txn_t *tx, void *dest, b_size dlen)
{
  return nsdb_plan_read (plan, tx, dest, dlen);
}

void *
ns_malloc (nsdb_t *db, nsdb_txn_t *txn, b_size *dlen, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  err_t code;
  char *query = ns_format_query (db, txn, fmt, ap, &temp, &code);
  va_end (ap);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return NULL;
  }

  // The result comes from the database, not from [temp].
  void *ret = nsdb_read_malloc (db, txn, dlen, query);
  ALLOC_CLOSE (temp);
  return ret;
}

void *
ns_plan_malloc (nsdb_plan_t *plan, nsdb_txn_t *tx, b_size *dlen)
{
  return nsdb_plan_read_malloc (plan, tx, dlen);
}

sb_size
ns_write (nsdb_t *db, nsdb_txn_t *txn, const void *src, b_size dlen, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  err_t code;
  char *query = ns_format_query (db, txn, fmt, ap, &temp, &code);
  va_end (ap);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return code;
  }

  sb_size ret = nsdb_write (db, txn, src, dlen, query);
  ALLOC_CLOSE (temp);
  return ret;
}

sb_size
ns_plan_write (nsdb_plan_t *plan, nsdb_txn_t *tx, const void *src, b_size dlen)
{
  return nsdb_plan_write (plan, tx, src, dlen);
}

sb_size
ns_smfile_size (nsdb_t *smf, nsdb_txn_t *tx)
{
  return nsdb_smfile_size (smf, tx);
}

sb_size
ns_smfile_insert (nsdb_t *smf, nsdb_txn_t *tx, const void *src, sb_size bofst, b_size slen)
{
  return nsdb_smfile_insert (smf, tx, src, bofst, slen);
}

sb_size
ns_smfile_read (
    nsdb_t     *smf,
    nsdb_txn_t *tx,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  return nsdb_smfile_read (smf, tx, dest, size, bofst, stride, nelem);
}

sb_size
ns_smfile_remove (
    nsdb_t     *smf,
    nsdb_txn_t *tx,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  return nsdb_smfile_remove (smf, tx, dest, size, bofst, stride, nelem);
}

sb_size
ns_smfile_write (
    nsdb_t     *smf,
    nsdb_txn_t *tx,
    const void *src,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  return nsdb_smfile_write (smf, tx, src, size, bofst, stride, nelem);
}

#ifndef NDEBUG
#  include "ns_testing.h"

#  include <stdlib.h>
#  include <string.h>

#  define NS_T_DB    "ns_api_test_db"
#  define NS_T_DB2   "ns_api_test_db2"
#  define NS_T_SMF   "ns_api_test_smf"
#  define NS_T_CRASH "ns_api_test_crash"

TEST (ns_api_open_and_close)
{
  TEST_CASE ("ns_cleanup on an absent path succeeds")
  {
    test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
  }

  TEST_CASE ("ns_open then ns_close")
  {
    nsdb_t *db = ns_open (NS_T_DB);
    test_assert (db != NULL);

    test_assert_ptr_equal (ns_strerror (db, false), NULL);

    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("ns_cleanup removes what ns_open created")
  {
    test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
  }
}

TEST (ns_api_open_with_error)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);

  TEST_CASE ("ns_open_with_error on a good path leaves the buffer alone")
  {
    char    erstr[256] = {0};
    nsdb_t *db         = ns_open_with_error (NS_T_DB, erstr, sizeof erstr);
    test_assert (db != NULL);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("ns_open_with_error reports why a bad path failed")
  {
    char    erstr[256] = {0};
    nsdb_t *db = ns_open_with_error ("ns_api_no_such_directory_xyz/db", erstr, sizeof erstr);
    test_assert_ptr_equal (db, NULL);
    test_assert (strnlen (erstr, MAX_ERRSTR_LEN) > 0);
  }

  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
}

TEST (ns_api_execute_write_read)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);

  nsdb_t *db = ns_open (NS_T_DB);
  test_assert (db != NULL);

  const int src[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  int       dest[8];

  TEST_CASE ("ns_execute creates a variable")
  {
    test_assert_int_equal (ns_execute (db, NULL, "create v i32"), 0);
  }

  // ns_write and ns_read count ELEMENTS, not bytes
  TEST_CASE ("ns_write inserts and reports the elements written")
  {
    const sb_size n = ns_write (db, NULL, src, sizeof src, "insert v 0 %d", 8);
    test_assert_int_equal ((int)n, 8);
  }

  TEST_CASE ("ns_read reads the same bytes back")
  {
    memset (dest, 0, sizeof dest);
    const sb_size n = ns_read (db, NULL, dest, sizeof dest, "read v[0:]");
    test_assert_int_equal ((int)n, 8);
    test_assert_memequal (dest, src, sizeof src);
  }

  TEST_CASE ("ns_malloc allocates the result for you")
  {
    b_size dlen = 0;
    void  *got  = ns_malloc (db, NULL, &dlen, "read v[0:]");
    test_assert (got != NULL);
    test_assert_int_equal ((int)dlen, (int)sizeof src);
    test_assert_memequal (got, src, sizeof src);

    // The contract is the system free, not an ns_ call
    free (got);
  }

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
}

TEST (ns_api_plans)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);

  nsdb_t *db = ns_open (NS_T_DB);
  test_assert (db != NULL);

  const int src[4] = {10, 20, 30, 40};
  int       dest[4];

  TEST_CASE ("ns_plan_fcreate then ns_plan_execute runs a create")
  {
    nsdb_plan_t *plan = ns_plan_fcreate (db, "create p i32");
    test_assert (plan != NULL);

    test_assert_ptr_equal (ns_plan_strerror (plan, false), NULL);

    test_assert_int_equal (ns_plan_execute (plan, NULL), 0);
    ns_plan_free (plan);
  }

  TEST_CASE ("ns_plan_write runs an insert")
  {
    nsdb_plan_t *plan = ns_plan_fcreate (db, "insert p 0 4");
    test_assert (plan != NULL);

    const sb_size n = ns_plan_write (plan, NULL, src, sizeof src);
    test_assert_int_equal ((int)n, 4);

    ns_plan_free (plan);
  }

  TEST_CASE ("ns_plan_read runs a read")
  {
    nsdb_plan_t *plan = ns_plan_fcreate (db, "read p[0:]");
    test_assert (plan != NULL);

    memset (dest, 0, sizeof dest);
    const sb_size n = ns_plan_read (plan, NULL, dest, sizeof dest);
    test_assert_int_equal ((int)n, 4);
    test_assert_memequal (dest, src, sizeof src);

    ns_plan_free (plan);
  }

  TEST_CASE ("ns_plan_malloc allocates the result")
  {
    nsdb_plan_t *plan = ns_plan_fcreate (db, "read p[0:]");
    test_assert (plan != NULL);

    b_size dlen = 0;
    void  *got  = ns_plan_malloc (plan, NULL, &dlen);
    test_assert (got != NULL);
    test_assert_int_equal ((int)dlen, (int)sizeof src);
    free (got);

    ns_plan_free (plan);
  }

  TEST_CASE ("ns_plan_get_var resolves the variable")
  {
    nsdb_plan_t *plan = ns_plan_fcreate (db, "get p");
    test_assert (plan != NULL);

    nsdb_var_t *var = ns_plan_get_var (plan, NULL);
    test_assert (var != NULL);
    // ns_var_len is an element count too
    test_assert_int_equal ((int)ns_var_len (var), 4);
    ns_var_free (var);

    ns_plan_free (plan);
  }

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
}

struct ns_visit_count
{
  unsigned seen;
  b_size   total_len;
};

static void
ns_t_count_vars (nsdb_var_t *v, void *vctx)
{
  struct ns_visit_count *c = vctx;
  c->seen += 1;
  c->total_len += ns_var_len (v);
}

TEST (ns_api_get_var)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);

  nsdb_t *db = ns_open (NS_T_DB);
  test_assert (db != NULL);

  test_assert_int_equal (ns_execute (db, NULL, "create a i32"), 0);

  const int src[2] = {7, 8};
  test_assert_int_equal ((int)ns_write (db, NULL, src, sizeof src, "insert a 0 %d", 2), 2);

  TEST_CASE ("ns_get_var and ns_var_len report the stored length")
  {
    nsdb_var_t *var = ns_get_var (db, NULL, "get a");
    test_assert (var != NULL);
    test_assert_int_equal ((int)ns_var_len (var), 2);
    ns_var_free (var);
  }

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
}

TEST (ns_api_get_var_empty)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB2), 0);

  nsdb_t *db = ns_open (NS_T_DB2);
  test_assert (db != NULL);

  test_assert_int_equal (ns_execute (db, NULL, "create b f32"), 0);

  nsdb_var_t *var = ns_get_var (db, NULL, "get b");
  test_assert (var != NULL);
  ns_var_free (var);

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB2), 0);
}

TEST (ns_api_variables)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB2), 0);

  nsdb_t *db = ns_open (NS_T_DB2);
  test_assert (db != NULL);

  TEST_CASE ("A fresh database has no variables to visit")
  {
    struct ns_visit_count c = {0};
    test_assert_int_equal (ns_visit_nsdb_vars (db, ns_t_count_vars, NULL, &c), 0);
    test_assert_int_equal ((int)c.seen, 0);
  }

  test_assert_int_equal (ns_execute (db, NULL, "create v1 i32"), 0);
  test_assert_int_equal (ns_execute (db, NULL, "create v2 f64"), 0);

  const int src[3] = {1, 2, 3};
  test_assert_int_equal ((int)ns_write (db, NULL, src, sizeof src, "insert v1 0 %d", 3), 3);

  TEST_CASE ("ns_visit_nsdb_vars sees every variable")
  {
    struct ns_visit_count c = {0};
    test_assert_int_equal (ns_visit_nsdb_vars (db, ns_t_count_vars, NULL, &c), 0);
    test_assert_int_equal ((int)c.seen, 2);

    // Three elements in v1, none in v2
    test_assert_int_equal ((int)c.total_len, 3);
  }

  // Reads don't join a transaction, so passing one changes nothing
  TEST_CASE ("ns_visit_nsdb_vars ignores the transaction it is handed")
  {
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);

    struct ns_visit_count c = {0};
    test_assert_int_equal (ns_visit_nsdb_vars (db, ns_t_count_vars, tx, &c), 0);
    test_assert_int_equal ((int)c.seen, 2);

    test_assert_int_equal (ns_commit (db, tx), 0);
  }

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB2), 0);
}

TEST (ns_api_transactions)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);

  nsdb_t *db = ns_open (NS_T_DB);
  test_assert (db != NULL);

  test_assert_int_equal (ns_execute (db, NULL, "create t i32"), 0);

  const int src[4] = {1, 2, 3, 4};

  TEST_CASE ("ns_begin then ns_commit keeps the write")
  {
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);

    test_assert_int_equal ((int)ns_write (db, tx, src, sizeof src, "insert t 0 %d", 4), 4);
    test_assert_int_equal (ns_commit (db, tx), 0);

    int dest[4] = {0};
    test_assert_int_equal ((int)ns_read (db, NULL, dest, sizeof dest, "read t[0:]"), 4);
    test_assert_memequal (dest, src, sizeof src);
  }

  TEST_CASE ("ns_begin then ns_rollback discards the write")
  {
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);

    test_assert_int_equal ((int)ns_write (db, tx, src, sizeof src, "insert t 0 %d", 4), 4);
    test_assert_int_equal (ns_rollback (db, tx), 0);

    // Still just the four committed elements
    int dest[8] = {0};
    test_assert_int_equal ((int)ns_read (db, NULL, dest, sizeof dest, "read t[0:]"), 4);
  }

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB), 0);
}

TEST (ns_api_crash)
{
  test_assert_int_equal (ns_cleanup (NS_T_CRASH), 0);

  const int src[4] = {5, 6, 7, 8};

  {
    nsdb_t *db = ns_open (NS_T_CRASH);
    test_assert (db != NULL);

    test_assert_int_equal (ns_execute (db, NULL, "create c i32"), 0);
    test_assert_int_equal ((int)ns_write (db, NULL, src, sizeof src, "insert c 0 %d", 4), 4);

    test_assert_int_equal (ns_crash (db), 0);
  }

  {
    nsdb_t *db = ns_open (NS_T_CRASH);
    test_assert (db != NULL);

    int dest[4] = {0};
    test_assert_int_equal ((int)ns_read (db, NULL, dest, sizeof dest, "read c[0:]"), 4);
    test_assert_memequal (dest, src, sizeof src);

    test_assert_int_equal (ns_close (db), 0);
  }

  test_assert_int_equal (ns_cleanup (NS_T_CRASH), 0);
}

TEST (ns_api_smfile_open_and_close)
{
  test_assert_int_equal (ns_cleanup (NS_T_SMF), 0);

  TEST_CASE ("ns_smfile_open then ns_close")
  {
    nsdb_t *smf = ns_smfile_open (NS_T_SMF);
    test_assert (smf != NULL);
    test_assert_int_equal (ns_close (smf), 0);
  }

  TEST_CASE ("ns_smfile_open_with_error on a good path")
  {
    char    erstr[256] = {0};
    nsdb_t *smf        = ns_smfile_open_with_error (NS_T_SMF, erstr, sizeof erstr);
    test_assert (smf != NULL);
    test_assert_int_equal (ns_close (smf), 0);
  }

  TEST_CASE ("ns_smfile_open_with_error reports why a bad path failed")
  {
    char    erstr[256] = {0};
    nsdb_t *smf        = ns_smfile_open_with_error (
        "ns_api_no_such_directory_xyz/smf",
        erstr,
        sizeof erstr
    );
    test_assert_ptr_equal (smf, NULL);
    test_assert (strnlen (erstr, MAX_ERRSTR_LEN) > 0);
  }

  test_assert_int_equal (ns_cleanup (NS_T_SMF), 0);
}

TEST (ns_api_smfile_operations)
{
  test_assert_int_equal (ns_cleanup (NS_T_SMF), 0);

  nsdb_t *smf = ns_smfile_open (NS_T_SMF);
  test_assert (smf != NULL);

  TEST_CASE ("A fresh smart file is empty")
  {
    test_assert_int_equal ((int)ns_smfile_size (smf, NULL), 0);
  }

  TEST_CASE ("ns_smfile_insert appends bytes")
  {
    test_assert_int_equal ((int)ns_smfile_insert (smf, NULL, "hello world", 0, 11), 11);
    test_assert_int_equal ((int)ns_smfile_size (smf, NULL), 11);
  }

  TEST_CASE ("ns_smfile_read reads the whole file")
  {
    char got[16] = {0};
    test_assert_int_equal ((int)ns_smfile_read (smf, NULL, got, 1, 0, 1, NS_END), 11);
    test_assert_memequal (got, "hello world", 11);
  }

  TEST_CASE ("ns_smfile_write overwrites in place")
  {
    test_assert_int_equal ((int)ns_smfile_write (smf, NULL, "WORLD", 1, 6, 1, 5), 5);

    char got[16] = {0};
    test_assert_int_equal ((int)ns_smfile_read (smf, NULL, got, 1, 0, 1, NS_END), 11);
    test_assert_memequal (got, "hello WORLD", 11);
  }

  TEST_CASE ("ns_smfile_remove takes bytes out and hands them back")
  {
    char evicted[8] = {0};
    test_assert_int_equal ((int)ns_smfile_remove (smf, NULL, evicted, 1, 0, 1, 6), 6);
    test_assert_memequal (evicted, "hello ", 6);

    test_assert_int_equal ((int)ns_smfile_size (smf, NULL), 5);

    char got[8] = {0};
    test_assert_int_equal ((int)ns_smfile_read (smf, NULL, got, 1, 0, 1, NS_END), 5);
    test_assert_memequal (got, "WORLD", 5);
  }

  TEST_CASE ("Smart file writes take part in transactions")
  {
    nsdb_txn_t *tx = ns_begin (smf);
    test_assert (tx != NULL);

    test_assert_int_equal ((int)ns_smfile_insert (smf, tx, "!!", 5, 2), 2);
    test_assert_int_equal (ns_commit (smf, tx), 0);

    test_assert_int_equal ((int)ns_smfile_size (smf, NULL), 7);
  }

  test_assert_int_equal (ns_close (smf), 0);
  test_assert_int_equal (ns_cleanup (NS_T_SMF), 0);
}

TEST (ns_api_error_reporting)
{
  test_assert_int_equal (ns_cleanup (NS_T_DB2), 0);

  nsdb_t *db = ns_open (NS_T_DB2);
  test_assert (db != NULL);

  TEST_CASE ("ns_strerror describes a failed query")
  {
    test_assert (ns_execute (db, NULL, "this is not a query") != 0);

    const char *msg = ns_strerror (db, false);
    test_assert (msg != NULL);
    test_assert (strnlen (msg, MAX_ERRSTR_LEN) > 0);
  }

  TEST_CASE ("ns_plan_fcreate rejects a malformed query")
  {
    nsdb_plan_t *plan = ns_plan_fcreate (db, "also not a query");
    if (plan == NULL) {
      // The parse failed outright, and the reason is on the database
      const char *msg = ns_strerror (db, false);
      test_assert (msg != NULL);
    } else {
      test_assert (ns_plan_execute (plan, NULL) != 0);
      test_assert (ns_plan_strerror (plan, false) != NULL);
      ns_plan_free (plan);
    }
  }

  TEST_CASE ("an error stays until it is taken, and is taken once")
  {
    test_assert (ns_execute (db, NULL, "this is not a query") != 0);

    // Calls that work don't clear it
    test_assert_int_equal (ns_execute (db, NULL, "create kept u32"), 0);
    test_assert_int_equal (ns_execute (db, NULL, "delete kept"), 0);

    test_assert (ns_strerror (db, false) != NULL);
    test_assert_ptr_equal (ns_strerror (db, false), NULL);
    test_assert_ptr_equal (ns_strerror (db, true), NULL);
  }

  TEST_CASE ("a later failure replaces the error")
  {
    u32 dest[1];
    test_assert (ns_execute (db, NULL, "this is not a query") != 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read gone1") < 0);

    const char *msg = ns_strerror (db, false);
    test_assert (msg != NULL);
    test_assert (strstr (msg, "gone1") != NULL);
    test_assert_ptr_equal (ns_strerror (db, false), NULL);
  }

  TEST_CASE ("copy hands out a string of the caller's")
  {
    u32 dest[1];
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read one_gone") < 0);

    char *msg = (char *)ns_strerror (db, true);
    test_assert (msg != NULL);
    test_assert (strstr (msg, "one_gone") != NULL);

    // The next failure doesn't touch the copy
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read two_gone") < 0);
    test_assert (strstr (msg, "one_gone") != NULL);
    test_assert (strstr (msg, "two_gone") == NULL);
    free (msg);

    char *next = (char *)ns_strerror (db, true);
    test_assert (next != NULL);
    test_assert (strstr (next, "two_gone") != NULL);
    free (next);
  }

  TEST_CASE ("a failure in a transaction is on the transaction and the database")
  {
    u32         dest[1];
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);

    // Nothing failed in it yet
    test_assert_ptr_equal (ns_txn_strerror (db, tx, false), NULL);

    test_assert (ns_read (db, tx, dest, sizeof dest, "read intxn") < 0);

    // Taking one leaves the other
    char *on_txn = (char *)ns_txn_strerror (db, tx, true);
    test_assert (on_txn != NULL);
    test_assert (strstr (on_txn, "intxn") != NULL);
    free (on_txn);
    test_assert_ptr_equal (ns_txn_strerror (db, tx, false), NULL);

    const char *on_db = ns_strerror (db, false);
    test_assert (on_db != NULL);
    test_assert (strstr (on_db, "intxn") != NULL);

    test_assert_int_equal (ns_rollback (db, tx), 0);
  }

  TEST_CASE ("the error of a transaction is not replaced by other calls")
  {
    u32         dest[1];
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    test_assert (ns_read (db, tx, dest, sizeof dest, "read mine") < 0);

    // A call outside the transaction fails with something else. That
    // replaces the database's error and not the transaction's.
    //
    // It fails before it begins a transaction of its own - an auto
    // transaction would wait for [tx] to end
    test_assert (ns_execute (db, NULL, "theirs is not a query") != 0);

    const char *on_db = ns_strerror (db, false);
    test_assert (on_db != NULL);
    test_assert (strstr (on_db, "mine") == NULL);

    const char *on_txn = ns_txn_strerror (db, tx, false);
    test_assert (on_txn != NULL);
    test_assert (strstr (on_txn, "mine") != NULL);

    test_assert_int_equal (ns_rollback (db, tx), 0);
  }

  TEST_CASE ("a plan reports through its database")
  {
    u32          dest[1];
    nsdb_plan_t *plan = ns_plan_fcreate (db, "read inplan");
    test_assert (plan != NULL);
    test_assert (ns_plan_read (plan, NULL, dest, sizeof dest) < 0);

    char *msg = (char *)ns_plan_strerror (plan, true);
    test_assert (msg != NULL);
    test_assert (strstr (msg, "inplan") != NULL);
    free (msg);

    // The plan's error was the database's error
    test_assert_ptr_equal (ns_strerror (db, false), NULL);
    ns_plan_free (plan);
  }

  test_assert_int_equal (ns_close (db), 0);
  test_assert_int_equal (ns_cleanup (NS_T_DB2), 0);
}
#endif
