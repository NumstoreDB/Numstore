// Test double for libnumstore. Implements the numstore.h ABI so the C#
// bindings can be exercised without the real engine. Queries are not parsed;
// each call records what it received so the tests can check marshalling.
// Smart files are a real in-memory byte array so strided I/O can be verified.

#include "numstore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct nsdb
{
  char     err[256];
  char     last_query[512];
  int      open_txns;
  int      open_plans;
  uint8_t *data; // smart file contents
  b_size   len;
  int      closed_with; // 1 = close, 2 = crash
};

struct txn
{
  int id;
};

struct nsdb_var
{
  b_size len;
};

struct nsdb_plan
{
  nsdb_t *db;
  char    query[512];
  char    err[256];
};

// Observable from tests
int g_live_dbs, g_live_vars, g_commits, g_rollbacks, g_crashes, g_close_violations;

const char *
stub_last_query (nsdb_t *db)
{
  return db->last_query;
}

int
stub_counter (const char *name)
{
  if (!strcmp (name, "dbs")) {
    return g_live_dbs;
  }
  if (!strcmp (name, "vars")) {
    return g_live_vars;
  }
  if (!strcmp (name, "commits")) {
    return g_commits;
  }
  if (!strcmp (name, "rollbacks")) {
    return g_rollbacks;
  }
  if (!strcmp (name, "crashes")) {
    return g_crashes;
  }
  if (!strcmp (name, "violations")) {
    return g_close_violations;
  }
  return -1;
}

static void
capture (char *dst, size_t n, const char *fmt, va_list ap)
{
  vsnprintf (dst, n, fmt, ap);
}

static int
fails (const char *q)
{
  return strncmp (q, "bad", 3) == 0;
}

nsdb_t *
ns_open (const char *path)
{
  if (strstr (path, "missing")) {
    return NULL;
  }
  g_live_dbs++;
  return calloc (1, sizeof (nsdb_t));
}

nsdb_t *
ns_smfile_open (const char *path)
{
  return ns_open (path);
}

int
ns_cleanup (const char *path)
{
  return strstr (path, "missing") ? -2 : 0;
}

static int
do_close (nsdb_t *ns, int how)
{
  if (ns->open_txns || ns->open_plans) {
    g_close_violations++;
  }
  if (how == 2) {
    g_crashes++;
  }
  free (ns->data);
  free (ns);
  g_live_dbs--;
  return 0;
}

int
ns_close (nsdb_t *ns)
{
  return do_close (ns, 1);
}

int
ns_crash (nsdb_t *ns)
{
  return do_close (ns, 2);
}

b_size
ns_var_len (nsdb_var_t *var)
{
  return var->len;
}

void
ns_var_free (nsdb_var_t *var)
{
  free (var);
  g_live_vars--;
}

const char *
ns_strerror (nsdb_t *ns)
{
  return ns->err;
}

const char *
ns_plan_strerror (nsdb_plan_t *p)
{
  return p->err;
}

txn_t *
ns_begin (nsdb_t *ns)
{
  ns->open_txns++;
  return calloc (1, sizeof (txn_t));
}

int
ns_commit (nsdb_t *ns, txn_t *t)
{
  ns->open_txns--;
  g_commits++;
  free (t);
  return 0;
}

int
ns_rollback (nsdb_t *ns, txn_t *t)
{
  ns->open_txns--;
  g_rollbacks++;
  free (t);
  return 0;
}

//////////////////////////////////// direct queries

int
ns_execute (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  (void)tx;
  va_list ap;
  va_start (ap, fmt);
  capture (db->last_query, sizeof db->last_query, fmt, ap);
  va_end (ap);
  if (fails (db->last_query)) {
    snprintf (db->err, sizeof db->err, "syntax error in '%s'", db->last_query);
    return -7;
  }
  return 0;
}

static nsdb_var_t *
mkvar (const char *q)
{
  nsdb_var_t *v = malloc (sizeof *v);
  v->len        = strlen (q);
  g_live_vars++;
  return v;
}

nsdb_var_t *
ns_get_var (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  (void)tx;
  va_list ap;
  va_start (ap, fmt);
  capture (db->last_query, sizeof db->last_query, fmt, ap);
  va_end (ap);
  if (fails (db->last_query)) {
    strcpy (db->err, "no such variable");
    return NULL;
  }
  return mkvar (db->last_query);
}

// read: fills dest with 0,1,2,... and returns bytes
static sb_size
fill (void *dest, b_size n)
{
  for (b_size i = 0; i < n; i++) {
    ((uint8_t *)dest)[i] = (uint8_t)i;
  }
  return (sb_size)n;
}

// write: returns sum of bytes so the test can check the data arrived
static sb_size
sum (const void *src, b_size n)
{
  sb_size s = 0;
  for (b_size i = 0; i < n; i++) {
    s += ((const uint8_t *)src)[i];
  }
  return s;
}

sb_size
ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...)
{
  (void)txn;
  va_list ap;
  va_start (ap, fmt);
  capture (db->last_query, sizeof db->last_query, fmt, ap);
  va_end (ap);
  if (fails (db->last_query)) {
    strcpy (db->err, "read failed");
    return -3;
  }
  return fill (dest, dlen);
}

sb_size
ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...)
{
  (void)txn;
  va_list ap;
  va_start (ap, fmt);
  capture (db->last_query, sizeof db->last_query, fmt, ap);
  va_end (ap);
  if (fails (db->last_query)) {
    strcpy (db->err, "write failed");
    return -4;
  }
  return sum (src, dlen);
}

static void *
three_u32 (b_size *dlen)
{
  uint32_t *p = malloc (12);
  p[0]        = 10;
  p[1]        = 20;
  p[2]        = 30;
  *dlen       = 12;
  return p;
}

void *
ns_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...)
{
  (void)txn;
  va_list ap;
  va_start (ap, fmt);
  capture (db->last_query, sizeof db->last_query, fmt, ap);
  va_end (ap);
  if (fails (db->last_query)) {
    strcpy (db->err, "malloc failed");
    return NULL;
  }
  return three_u32 (dlen);
}

//////////////////////////////////// plans

nsdb_plan_t *
ns_plan_fcreate (nsdb_t *db, const char *fmt, ...)
{
  char    q[512];
  va_list ap;
  va_start (ap, fmt);
  capture (q, sizeof q, fmt, ap);
  va_end (ap);
  strcpy (db->last_query, q);
  if (fails (q)) {
    strcpy (db->err, "cannot plan");
    return NULL;
  }
  nsdb_plan_t *p = calloc (1, sizeof *p);
  p->db          = db;
  strcpy (p->query, q);
  db->open_plans++;
  return p;
}

void
ns_plan_free (nsdb_plan_t *p)
{
  p->db->open_plans--;
  free (p);
}

int
ns_plan_execute (nsdb_plan_t *p, struct txn *tx)
{
  (void)tx;
  if (strstr (p->query, "explode")) {
    strcpy (p->err, "plan exploded");
    return -9;
  }
  return 0;
}

nsdb_var_t *
ns_plan_get_var (nsdb_plan_t *p, struct txn *tx)
{
  (void)tx;
  return mkvar (p->query);
}

sb_size
ns_plan_read (nsdb_plan_t *p, txn_t *t, void *d, b_size n)
{
  (void)p;
  (void)t;
  return fill (d, n);
}

sb_size
ns_plan_write (nsdb_plan_t *p, txn_t *t, const void *s, b_size n)
{
  (void)p;
  (void)t;
  return sum (s, n);
}

void *
ns_plan_malloc (nsdb_plan_t *p, txn_t *t, b_size *n)
{
  (void)p;
  (void)t;
  return three_u32 (n);
}

//////////////////////////////////// smart files (real in-memory semantics)

sb_size
ns_smfile_size (nsdb_t *smf, txn_t *tx)
{
  (void)tx;
  return (sb_size)smf->len;
}

sb_size
ns_smfile_insert (nsdb_t *smf, txn_t *tx, const void *src, sb_size bofst, b_size slen)
{
  (void)tx;
  if (bofst < 0 || (b_size)bofst > smf->len) {
    strcpy (smf->err, "insert out of range");
    return -5;
  }
  smf->data = realloc (smf->data, smf->len + slen);
  memmove (smf->data + bofst + slen, smf->data + bofst, smf->len - bofst);
  memcpy (smf->data + bofst, src, slen);
  smf->len += slen;
  return (sb_size)slen;
}

static int
in_range (nsdb_t *smf, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  if (nelem == 0) {
    return 1;
  }
  b_size last = (b_size)bofst + (nelem - 1) * (b_size)stride * size + size;
  return bofst >= 0 && stride > 0 && last <= smf->len;
}

sb_size
ns_smfile_write (
    nsdb_t     *smf,
    txn_t      *tx,
    const void *src,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  (void)tx;
  if (!in_range (smf, size, bofst, stride, nelem)) {
    strcpy (smf->err, "write past end");
    return -6;
  }
  for (b_size i = 0; i < nelem; i++) {
    memcpy (smf->data + bofst + i * stride * size, (const uint8_t *)src + i * size, size);
  }
  return (sb_size)nelem;
}

sb_size
ns_smfile_read (
    nsdb_t *smf,
    txn_t  *tx,
    void   *dest,
    t_size  size,
    sb_size bofst,
    sb_size stride,
    b_size  nelem
)
{
  (void)tx;
  if (!in_range (smf, size, bofst, stride, nelem)) {
    strcpy (smf->err, "read past end");
    return -6;
  }
  for (b_size i = 0; i < nelem; i++) {
    memcpy ((uint8_t *)dest + i * size, smf->data + bofst + i * stride * size, size);
  }
  return (sb_size)nelem;
}

sb_size
ns_smfile_remove (
    nsdb_t *smf,
    txn_t  *tx,
    void   *dest,
    t_size  size,
    sb_size bofst,
    sb_size stride,
    b_size  nelem
)
{
  (void)tx;
  if (!in_range (smf, size, bofst, stride, nelem)) {
    strcpy (smf->err, "remove past end");
    return -6;
  }
  // remove from the back so earlier offsets stay valid
  for (b_size k = nelem; k-- > 0;) {
    b_size at = bofst + k * stride * size;
    if (dest) {
      memcpy ((uint8_t *)dest + k * size, smf->data + at, size);
    }
    memmove (smf->data + at, smf->data + at + size, smf->len - at - size);
    smf->len -= size;
  }
  return (sb_size)nelem;
}
