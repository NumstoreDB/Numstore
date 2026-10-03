#include "numstore.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/smartfiles/ns_smartfiles_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"

#include <stdarg.h>

nsdb_t *
ns_open (const char *path)
{
  error        e  = error_create ();
  struct nsdb *db = nsdb_open (path, default_mem (), default_filesystem (), &e);

  if (db == NULL) {
    return NULL;
  }

  // Initialize pager
  if (numstore_init_pager (db->p, &e)) {
    nsdb_close (db, &e);
    return NULL;
  }

  // Allocate an error for the database
  error *persistent_e = i_malloc (default_mem (), 1, sizeof *persistent_e, &e);
  *persistent_e       = error_create ();
  if (persistent_e == NULL) {
    nsdb_close (db, &e);
    return NULL;
  }

  nsdb_set_error (db, persistent_e);
  nsdb_allow_auto_txn (db);

  return db;
}

nsdb_t *
ns_smfile_open (const char *path)
{
  error        e  = error_create ();
  struct nsdb *db = nsdb_open (path, default_mem (), default_filesystem (), &e);

  if (db == NULL) {
    return NULL;
  }

  // Initialize pager
  if (smartfiles_init_pager (db->p, &e)) {
    nsdb_close (db, &e);
    return NULL;
  }

  // Allocate an error for the database
  error *persistent_e = i_malloc (default_mem (), 1, sizeof *persistent_e, &e);
  *persistent_e       = error_create ();
  if (persistent_e == NULL) {
    nsdb_close (db, &e);
    return NULL;
  }

  nsdb_set_error (db, persistent_e);
  nsdb_allow_auto_txn (db);

  return db;
}

int
ns_cleanup (const char *path)
{
  error e = error_create ();
  return nsdb_cleanup (path, &e);
}

int
ns_close (nsdb_t *ns)
{
  error_reset (ns->e);
  return nsdb_close (ns, ns->e);
}

int
ns_crash (nsdb_t *ns)
{
  error_reset (ns->e);
  return nsdb_crash (ns, ns->e);
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
  return nsdb_var_free (var);
}

// Errors
const char *
ns_strerror (nsdb_t *ns)
{
  if (ns->e->cause_code < 0) {
    error_reset (ns->e);
    return ns->e->cause_msg;
  }
  return NULL;
}

const char *
ns_plan_strerror (nsdb_plan_t *plan)
{
  return ns_strerror (plan->parent);
}

// Transactions
txn_t *
ns_begin (nsdb_t *ns)
{
  error_reset (ns->e);
  return nsdb_begin (ns, ns->e);
}

int
ns_commit (nsdb_t *ns, txn_t *txn)
{
  error_reset (ns->e);
  return nsdb_commit (ns, txn, ns->e);
}

int
ns_rollback (nsdb_t *ns, txn_t *txn)
{
  error_reset (ns->e);
  return nsdb_rollback (ns, txn, ns->e);
}

static inline char *
query_vsnprintf (const char *fmt, va_list ap, struct arena_alloc *alloc, error *e)
{
  va_list ap2;
  va_copy (ap2, ap);

  // Compute the length the formatted query needs, without writing anything.
  i32 qlen = vsnprintf (NULL, 0, fmt, ap);
  if (qlen < 0) {
    va_end (ap);
    error_causef (e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
    return NULL;
  }

  // Allocate buffer for the query
  char *buf = arena_malloc (alloc, (size_t)qlen + 1, 1, e);
  if (!buf) {
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

nsdb_plan_t *
ns_plan_fcreate (nsdb_t *db, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  nsdb_plan_t *ret = nsdb_plan_vcreate (db, fmt, NULL, args);
  va_end (args);
  return ret;
}

void
ns_plan_free (nsdb_plan_t *plan)
{
  nsdb_plan_free (plan);
}

int
ns_execute (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  error_reset (db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, NULL);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return error_trace (db->e);
  }

  err_t ret = nsdb_execute (db, tx, query, NULL);
  ALLOC_CLOSE (temp);
  return ret;
}

int
ns_plan_execute (nsdb_plan_t *plan, struct txn *tx)
{
  return nsdb_plan_execute (plan, tx, plan->parent->e);
}

nsdb_var_t *
ns_get_var (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  error_reset (db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, NULL);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return NULL;
  }

  nsdb_var_t *ret = nsdb_get_var (db, tx, query, NULL);
  if (ret == NULL) {
    ALLOC_CLOSE (temp);
    return NULL;
  }

  return ret;
}

nsdb_var_t *
ns_plan_get_var (nsdb_plan_t *plan, struct txn *tx)
{
  return nsdb_plan_get_var (plan, tx, plan->parent->e);
}

sb_size
ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...)
{
  error_reset (db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, NULL);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return error_trace (db->e);
  }

  err_t ret = nsdb_read (db, txn, dest, dlen, query, NULL);
  if (ret < 0) {
    ALLOC_CLOSE (temp);
    return ret;
  }

  ALLOC_CLOSE (temp);
  return ret;
}

sb_size
ns_plan_read (nsdb_plan_t *plan, txn_t *tx, void *dest, b_size dlen)
{
  return nsdb_plan_read (plan, tx, dest, dlen, plan->parent->e);
}

void *
ns_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...)
{
  error_reset (db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, NULL);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return NULL;
  }

  nsdb_var_t *ret = nsdb_read_malloc (db, txn, dlen, query, NULL);
  if (ret == NULL) {
    ALLOC_CLOSE (temp);
    return NULL;
  }

  return ret;
}

void *
ns_plan_malloc (nsdb_plan_t *plan, struct txn *tx, b_size *dlen)
{
  return nsdb_plan_read_malloc (plan, tx, dlen, plan->parent->e);
}

sb_size
ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...)
{
  error_reset (db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, NULL);
  if (query == NULL) {
    ALLOC_CLOSE (temp);
    return error_trace (db->e);
  }

  err_t ret = nsdb_write (db, txn, src, dlen, query, NULL);
  ALLOC_CLOSE (temp);
  return ret;
}

sb_size
ns_plan_write (nsdb_plan_t *plan, txn_t *tx, const void *src, b_size dlen)
{
  return nsdb_plan_write (plan, tx, src, dlen, plan->parent->e);
}

sb_size
ns_smfile_size (nsdb_t *db, struct txn *tx)
{
  error_reset (db->e);
  return nsdb_smfile_size (db, tx, NULL);
}

sb_size
ns_smfile_insert (nsdb_t *db, struct txn *tx, const void *src, sb_size bofst, b_size slen)
{
  error_reset (db->e);
  return nsdb_smfile_insert (db, tx, src, bofst, slen, NULL);
}

sb_size
ns_smfile_read (nsdb_t *db, struct txn *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  error_reset (db->e);
  return nsdb_smfile_read (db, tx, dest, size, bofst, stride, nelem, NULL);
}

sb_size
ns_smfile_remove (nsdb_t *db, struct txn *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  error_reset (db->e);
  return nsdb_smfile_remove (db, tx, dest, size, bofst, stride, nelem, NULL);
}

sb_size
ns_smfile_write (nsdb_t *db, struct txn *tx, const void *src, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  error_reset (db->e);
  return nsdb_smfile_write (db, tx, src, size, bofst, stride, nelem, NULL);
}
