#include "numstore/numstore.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"

// Lifecycle
nsdb_t *
ns_open (const char *path)
{
  error                e       = error_create ();

  // Allocate wrapper
  struct nsdb_wrapper *wrapper = i_malloc (default_mem (), 1, sizeof *wrapper, &e);
  if (wrapper == NULL) {
    return NULL;
  }

  // Allocate database
  wrapper->db = nsdb_open_with_resources (path, default_mem (), default_filesystem (), &e);
  if (wrapper->db == NULL) {
    i_free (default_mem (), wrapper);
    return NULL;
  }

  // Initialize pager
  if (numstore_init_pager (wrapper->db->p, &e)) {
    nsdb_close (wrapper->db, &e);
    i_free (default_mem (), wrapper);
    return NULL;
  }

  wrapper->e = error_create ();

  return wrapper;
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
  error_reset (&ns->e);
  err_t ret = nsdb_close (ns->db, &ns->e);
  i_free (default_mem (), ns);
  return ret;
}

int
ns_crash (nsdb_t *ns)
{
  error_reset (&ns->e);
  err_t ret = nsdb_crash (ns->db, &ns->e);
  i_free (default_mem (), ns);
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
  return nsdb_var_free (var);
}

// Errors
const char *
ns_strerror (nsdb_t *ns)
{
  if (ns->e.cause_code < 0) {
    error_reset (&ns->e);
    return ns->e.cause_msg;
  }
  return NULL;
}

int
ns_perror (nsdb_t *ns, const char *prefix)
{
  const char *err = ns_strerror (ns);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, err);
  }
  return fprintf (stderr, "%s: success\n", prefix);
}

// Transactions
txn_t *
ns_begin (nsdb_t *ns)
{
  error_reset (&ns->e);
  return nsdb_begin (ns->db, &ns->e);
}

int
ns_commit (nsdb_t *ns, txn_t *txn)
{
  error_reset (&ns->e);
  return nsdb_commit (ns->db, txn, &ns->e);
}

int
ns_rollback (nsdb_t *ns, txn_t *txn)
{
  error_reset (&ns->e);
  return nsdb_rollback (ns->db, txn, &ns->e);
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

int
ns_exec (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  error_reset (&db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, &db->e);
  if (query == NULL) {
    goto failed;
  }

  err_t ret;
  WITH_AUTO_TXN (ret, db->db, tx, nsdb_exec (db->db, tx, query, &db->e), &db->e);

  if (ret < 0) {
    goto failed;
  }

failed:
  ALLOC_CLOSE (temp);
  return error_trace (&db->e);
}

nsdb_var_t *
ns_get_var (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  error_reset (&db->e);

  nsdb_var_t *ret = NULL;
  va_list     ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, &db->e);
  if (query == NULL) {
    goto failed;
  }

  WITH_AUTO_TXN_PTR (ret, db->db, tx, nsdb_get_var (db->db, tx, query, &db->e), &db->e);
  if (ret == NULL) {
    goto failed;
  }

failed:
  ALLOC_CLOSE (temp);
  return ret;
}

sb_size
ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...)
{
  error_reset (&db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, &db->e);
  if (query == NULL) {
    goto failed;
  }

  err_t ret;
  WITH_AUTO_TXN (ret, db->db, txn, nsdb_read (db->db, txn, dest, dlen, query, &db->e), &db->e);

  if (ret < 0) {
    goto failed;
  }

  return ret;

failed:
  ALLOC_CLOSE (temp);
  return error_trace (&db->e);
}

void *
ns_read_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...)
{
  error_reset (&db->e);

  nsdb_var_t *ret = NULL;
  va_list     ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, &db->e);
  if (query == NULL) {
    goto theend;
  }

  WITH_AUTO_TXN_PTR (ret, db->db, txn, nsdb_read_malloc (db->db, txn, dlen, query, &db->e), &db->e);
  if (ret == NULL) {
    goto theend;
  }

theend:
  ALLOC_CLOSE (temp);
  return ret;
}

sb_size
ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...)
{
  error_reset (&db->e);

  va_list ap;
  va_start (ap, fmt);

  ALLOC_INIT (temp);
  char *query = query_vsnprintf (fmt, ap, &temp, &db->e);
  if (query == NULL) {
    goto failed;
  }

  err_t ret;
  WITH_AUTO_TXN (ret, db->db, txn, nsdb_write (db->db, txn, src, dlen, query, &db->e), &db->e);

  if (ret < 0) {
    goto failed;
  }

  return ret;

failed:
  ALLOC_CLOSE (temp);
  return error_trace (&db->e);
}
