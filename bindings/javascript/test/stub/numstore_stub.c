/**
 * TEST-ONLY in-memory stand-in for libnumstore.
 *
 * Implements the public numstore.h API closely enough to exercise the
 * JavaScript bindings: a tiny query language (create/delete/get/insert/
 * write/read/remove), snapshot-based rollback for a single transaction,
 * and smart files with the strided semantics documented in the header.
 * Nothing is persisted. This is NOT numstore.
 */

#include "numstore.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  char     name[64];
  t_size   tsize;
  uint8_t *data;
  b_size   nbytes;
} svar;

typedef struct
{
  svar    *vars;
  size_t   nvars;
  uint8_t *bytes; // smart file contents
  b_size   nbytes;
} sstate;

struct nsdb
{
  bool   smf;
  sstate cur, snap;
  int    ntx;
  char   err[256];
};

struct txn
{
  nsdb_t *db;
};

struct nsdb_var
{
  b_size len;
};

struct nsdb_plan
{
  nsdb_t *db;
  char    q[512];
  char    err[256];
};

/////////////////////////////////////////// state helpers

static void
state_free (sstate *s)
{
  for (size_t i = 0; i < s->nvars; i++)
    free (s->vars[i].data);
  free (s->vars);
  free (s->bytes);
  memset (s, 0, sizeof *s);
}

static void
state_copy (sstate *dst, const sstate *src)
{
  memset (dst, 0, sizeof *dst);
  dst->nvars = src->nvars;
  dst->vars  = calloc (src->nvars ? src->nvars : 1, sizeof (svar));
  for (size_t i = 0; i < src->nvars; i++)
    {
      dst->vars[i]      = src->vars[i];
      dst->vars[i].data = malloc (src->vars[i].nbytes ? src->vars[i].nbytes : 1);
      memcpy (dst->vars[i].data, src->vars[i].data, src->vars[i].nbytes);
    }
  dst->nbytes = src->nbytes;
  dst->bytes  = malloc (src->nbytes ? src->nbytes : 1);
  memcpy (dst->bytes, src->bytes, src->nbytes);
}

static svar *
find (nsdb_t *db, const char *name)
{
  for (size_t i = 0; i < db->cur.nvars; i++)
    if (strcmp (db->cur.vars[i].name, name) == 0)
      return &db->cur.vars[i];
  return NULL;
}

static void
seterr (char *buf, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vsnprintf (buf, 256, fmt, ap);
  va_end (ap);
}

static bool
check_tx (nsdb_t *db, txn_t *tx, char *err)
{
  if (!tx || tx->db != db)
    {
      seterr (err, "invalid transaction");
      return false;
    }
  return true;
}

/////////////////////////////////////////// lifecycle

static nsdb_t *
open_common (const char *path, bool smf)
{
  if (!path || strstr (path, "/nonexistent-dir/"))
    return NULL;
  nsdb_t *db = calloc (1, sizeof *db);
  db->smf    = smf;
  return db;
}

nsdb_t *ns_open (const char *path) { return open_common (path, false); }
nsdb_t *ns_smfile_open (const char *path) { return open_common (path, true); }
int ns_cleanup (const char *path) { (void)path; return 0; }

int
ns_close (nsdb_t *ns)
{
  state_free (&ns->cur);
  state_free (&ns->snap);
  free (ns);
  return 0;
}

int ns_crash (nsdb_t *ns) { return ns_close (ns); }

b_size ns_var_len (nsdb_var_t *var) { return var->len; }
void ns_var_free (nsdb_var_t *var) { free (var); }
const char *ns_strerror (nsdb_t *ns) { return ns->err; }
const char *ns_plan_strerror (nsdb_plan_t *plan) { return plan->err; }

/////////////////////////////////////////// transactions

txn_t *
ns_begin (nsdb_t *ns)
{
  if (ns->ntx++ == 0)
    state_copy (&ns->snap, &ns->cur);
  txn_t *t = malloc (sizeof *t);
  t->db    = ns;
  return t;
}

int
ns_commit (nsdb_t *ns, txn_t *txn)
{
  if (!check_tx (ns, txn, ns->err))
    return -1;
  if (--ns->ntx == 0)
    state_free (&ns->snap);
  free (txn);
  return 0;
}

int
ns_rollback (nsdb_t *ns, txn_t *txn)
{
  if (!check_tx (ns, txn, ns->err))
    return -1;
  if (--ns->ntx == 0)
    {
      state_free (&ns->cur);
      ns->cur = ns->snap;
      memset (&ns->snap, 0, sizeof ns->snap);
    }
  free (txn);
  return 0;
}

/////////////////////////////////////////// query language

typedef enum
{
  Q_CREATE,
  Q_DELETE,
  Q_GET,
  Q_INSERT,
  Q_WRITE,
  Q_READ,
  Q_REMOVE
} qkind;

typedef struct
{
  qkind  k;
  char   name[64];
  char   type[16];
  b_size a, b;
  bool   has_b;
} query;

static bool
parse_range (const char *p, query *q, char *err)
{
  // NAME[a:b] or NAME[a:]
  const char *lb = strchr (p, '[');
  if (!lb || (size_t)(lb - p) >= sizeof q->name || lb == p)
    goto bad;
  memcpy (q->name, p, lb - p);
  q->name[lb - p] = 0;
  char *end;
  q->a = strtoull (lb + 1, &end, 10);
  if (end == lb + 1 || *end != ':')
    goto bad;
  const char *bp = end + 1;
  if (*bp == ']')
    {
      q->has_b = false;
      end      = (char *)bp;
    }
  else
    {
      q->b     = strtoull (bp, &end, 10);
      q->has_b = true;
      if (end == bp || *end != ']')
        goto bad;
    }
  if (end[1] != 0)
    goto bad;
  return true;
bad:
  seterr (err, "syntax error: expected NAME[start:end]");
  return false;
}

static bool
parse (const char *s, query *q, char *err)
{
  memset (q, 0, sizeof *q);
  char kw[16], rest[256];
  int  n = sscanf (s, "%15s %255[^\n]", kw, rest);
  if (n < 2)
    {
      seterr (err, "syntax error in '%s'", s);
      return false;
    }
  if (!strcmp (kw, "create"))
    {
      q->k = Q_CREATE;
      if (sscanf (rest, "%63s %15s", q->name, q->type) != 2)
        goto bad;
      return true;
    }
  if (!strcmp (kw, "delete") || !strcmp (kw, "get"))
    {
      q->k = kw[0] == 'd' ? Q_DELETE : Q_GET;
      if (sscanf (rest, "%63s", q->name) != 1)
        goto bad;
      return true;
    }
  if (!strcmp (kw, "insert"))
    {
      q->k = Q_INSERT;
      unsigned long long a, b;
      if (sscanf (rest, "%63s %llu %llu", q->name, &a, &b) != 3)
        goto bad;
      q->a = a, q->b = b, q->has_b = true;
      return true;
    }
  if (!strcmp (kw, "write"))
    return q->k = Q_WRITE, parse_range (rest, q, err);
  if (!strcmp (kw, "read"))
    return q->k = Q_READ, parse_range (rest, q, err);
  if (!strcmp (kw, "remove"))
    return q->k = Q_REMOVE, parse_range (rest, q, err);
bad:
  seterr (err, "syntax error in '%s'", s);
  return false;
}

static t_size
type_size (const char *t)
{
  static const struct
  {
    const char *n;
    t_size      s;
  } tbl[] = { { "u8", 1 },  { "i8", 1 },  { "u16", 2 }, { "i16", 2 }, { "u32", 4 },
              { "i32", 4 }, { "u64", 8 }, { "i64", 8 }, { "f32", 4 }, { "f64", 8 } };
  for (size_t i = 0; i < sizeof tbl / sizeof tbl[0]; i++)
    if (!strcmp (tbl[i].n, t))
      return tbl[i].s;
  return 0;
}

enum
{
  M_EXEC,
  M_READ,
  M_WRITE,
  M_MALLOC,
};

// Core: perform query [qs]. Returns bytes processed or -1.
static sb_size
run (nsdb_t *db, txn_t *tx, const char *qs, int mode, const void *src, b_size slen, void *dest,
     b_size dlen, void **mout, char *err)
{
  if (db->smf)
    {
      seterr (err, "queries are not supported on smart files");
      return -1;
    }
  if (!check_tx (db, tx, err))
    return -1;
  query q;
  if (!parse (qs, &q, err))
    return -1;

  bool readable = q.k == Q_READ || q.k == Q_REMOVE;
  bool writable = q.k == Q_INSERT || q.k == Q_WRITE;
  if ((mode == M_READ || mode == M_MALLOC) && !readable)
    return seterr (err, "query is not readable (READ/REMOVE only)"), -1;
  if (mode == M_WRITE && !writable)
    return seterr (err, "query is not writable (INSERT/WRITE only)"), -1;
  if (mode == M_EXEC && (writable || q.k == Q_READ || q.k == Q_GET))
    return seterr (err, "query needs a buffer; use read/write instead"), -1;

  if (q.k == Q_CREATE)
    {
      t_size ts = type_size (q.type);
      if (!ts)
        return seterr (err, "unknown type '%s'", q.type), -1;
      if (find (db, q.name))
        return seterr (err, "variable '%s' already exists", q.name), -1;
      db->cur.vars = realloc (db->cur.vars, (db->cur.nvars + 1) * sizeof (svar));
      svar *v      = &db->cur.vars[db->cur.nvars++];
      memset (v, 0, sizeof *v);
      snprintf (v->name, sizeof v->name, "%s", q.name);
      v->tsize = ts;
      return 0;
    }

  svar *v = find (db, q.name);
  if (!v)
    return seterr (err, "variable '%s' does not exist", q.name), -1;
  b_size nelem = v->nbytes / v->tsize;

  switch (q.k)
    {
    case Q_DELETE:
      {
        free (v->data);
        size_t idx = v - db->cur.vars;
        memmove (v, v + 1, (db->cur.nvars - idx - 1) * sizeof (svar));
        db->cur.nvars--;
        return 0;
      }
    case Q_INSERT:
      {
        if (q.a > nelem)
          return seterr (err, "insert offset %llu out of range", (unsigned long long)q.a), -1;
        b_size nb = q.b * v->tsize;
        if (slen != nb)
          return seterr (err, "insert of %llu elements needs %llu bytes, got %llu", (unsigned long long)q.b,
                         (unsigned long long)nb, (unsigned long long)slen),
                 -1;
        v->data = realloc (v->data, v->nbytes + nb + 1);
        b_size at = q.a * v->tsize;
        memmove (v->data + at + nb, v->data + at, v->nbytes - at);
        memcpy (v->data + at, src, nb);
        v->nbytes += nb;
        return (sb_size)nb;
      }
    default:
      break;
    }

  // Ranged ops
  b_size b = q.has_b ? q.b : nelem;
  if (q.a > b || b > nelem)
    return seterr (err, "range [%llu:%llu] out of bounds (length %llu)", (unsigned long long)q.a,
                   (unsigned long long)b, (unsigned long long)nelem),
           -1;
  b_size at = q.a * v->tsize, nb = (b - q.a) * v->tsize;

  if (q.k == Q_WRITE)
    {
      if (slen != nb)
        return seterr (err, "write needs %llu bytes, got %llu", (unsigned long long)nb, (unsigned long long)slen), -1;
      memcpy (v->data + at, src, nb);
      return (sb_size)nb;
    }

  // READ / REMOVE: copy out
  if (mode == M_READ)
    {
      if (dlen < nb)
        return seterr (err, "destination too small: need %llu bytes", (unsigned long long)nb), -1;
      memcpy (dest, v->data + at, nb);
    }
  else if (mode == M_MALLOC)
    {
      *mout = nb ? malloc (nb) : NULL;
      if (nb)
        memcpy (*mout, v->data + at, nb);
    }
  if (q.k == Q_REMOVE)
    {
      memmove (v->data + at, v->data + at + nb, v->nbytes - at - nb);
      v->nbytes -= nb;
    }
  return (sb_size)nb;
}

/////////////////////////////////////////// direct API

#define VFMT(buf, fmt)                    \
  char    buf[512];                       \
  va_list ap;                             \
  va_start (ap, fmt);                     \
  vsnprintf (buf, sizeof buf, fmt, ap);   \
  va_end (ap)

int
ns_execute (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  VFMT (q, fmt);
  return run (db, tx, q, M_EXEC, NULL, 0, NULL, 0, NULL, db->err) < 0 ? -1 : 0;
}

static nsdb_var_t *
get_var (nsdb_t *db, txn_t *tx, const char *qs, char *err)
{
  if (!check_tx (db, tx, err))
    return NULL;
  query q;
  if (!parse (qs, &q, err))
    return NULL;
  svar *v = find (db, q.name);
  if (!v)
    return seterr (err, "variable '%s' does not exist", q.name), NULL;
  nsdb_var_t *out = malloc (sizeof *out);
  out->len        = v->nbytes / v->tsize;
  return out;
}

nsdb_var_t *
ns_get_var (nsdb_t *db, struct txn *tx, const char *fmt, ...)
{
  VFMT (q, fmt);
  return get_var (db, tx, q, db->err);
}

sb_size
ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...)
{
  VFMT (q, fmt);
  return run (db, txn, q, M_READ, NULL, 0, dest, dlen, NULL, db->err);
}

sb_size
ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...)
{
  VFMT (q, fmt);
  return run (db, txn, q, M_WRITE, src, dlen, NULL, 0, NULL, db->err);
}

void *
ns_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...)
{
  VFMT (q, fmt);
  void   *out = NULL;
  sb_size n   = run (db, txn, q, M_MALLOC, NULL, 0, NULL, 0, &out, db->err);
  if (n < 0)
    return NULL;
  *dlen = (b_size)n;
  return out;
}

/////////////////////////////////////////// plans

nsdb_plan_t *
ns_plan_fcreate (nsdb_t *db, const char *fmt, ...)
{
  VFMT (q, fmt);
  query parsed;
  if (db->smf)
    return seterr (db->err, "plans are not supported on smart files"), NULL;
  if (!parse (q, &parsed, db->err))
    return NULL;
  nsdb_plan_t *p = calloc (1, sizeof *p);
  p->db          = db;
  snprintf (p->q, sizeof p->q, "%s", q);
  return p;
}

void ns_plan_free (nsdb_plan_t *plan) { free (plan); }

int
ns_plan_execute (nsdb_plan_t *p, struct txn *tx)
{
  return run (p->db, tx, p->q, M_EXEC, NULL, 0, NULL, 0, NULL, p->err) < 0 ? -1 : 0;
}

nsdb_var_t *
ns_plan_get_var (nsdb_plan_t *p, struct txn *tx)
{
  return get_var (p->db, tx, p->q, p->err);
}

sb_size
ns_plan_read (nsdb_plan_t *p, txn_t *tx, void *dest, b_size dlen)
{
  return run (p->db, tx, p->q, M_READ, NULL, 0, dest, dlen, NULL, p->err);
}

sb_size
ns_plan_write (nsdb_plan_t *p, txn_t *tx, const void *src, b_size dlen)
{
  return run (p->db, tx, p->q, M_WRITE, src, dlen, NULL, 0, NULL, p->err);
}

void *
ns_plan_malloc (nsdb_plan_t *p, txn_t *tx, b_size *dlen)
{
  void   *out = NULL;
  sb_size n   = run (p->db, tx, p->q, M_MALLOC, NULL, 0, NULL, 0, &out, p->err);
  if (n < 0)
    return NULL;
  *dlen = (b_size)n;
  return out;
}

/////////////////////////////////////////// smart files

static bool
smf_ok (nsdb_t *db, txn_t *tx)
{
  if (!db->smf)
    return seterr (db->err, "not a smart file"), false;
  return check_tx (db, tx, db->err);
}

sb_size
ns_smfile_size (nsdb_t *smf, txn_t *tx)
{
  if (!smf_ok (smf, tx))
    return -1;
  return (sb_size)smf->cur.nbytes;
}

sb_size
ns_smfile_insert (nsdb_t *smf, txn_t *tx, const void *src, sb_size bofst, b_size slen)
{
  if (!smf_ok (smf, tx))
    return -1;
  sstate *s = &smf->cur;
  if (bofst < 0 || (b_size)bofst > s->nbytes)
    return seterr (smf->err, "insert offset %lld out of range", (long long)bofst), -1;
  s->bytes = realloc (s->bytes, s->nbytes + slen + 1);
  memmove (s->bytes + bofst + slen, s->bytes + bofst, s->nbytes - bofst);
  memcpy (s->bytes + bofst, src, slen);
  s->nbytes += slen;
  return (sb_size)slen;
}

// Validates and returns the byte position of every element
static sb_size *
positions (nsdb_t *smf, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  sb_size *pos = malloc ((nelem ? nelem : 1) * sizeof *pos);
  for (b_size i = 0; i < nelem; i++)
    {
      pos[i] = bofst + (sb_size)i * stride * (sb_size)size;
      if (pos[i] < 0 || (b_size)pos[i] + size > smf->cur.nbytes)
        {
          free (pos);
          seterr (smf->err, "element %llu (byte %lld) is past the end of the file", (unsigned long long)i,
                  (long long)bofst + (long long)i * stride * size);
          return NULL;
        }
    }
  return pos;
}

sb_size
ns_smfile_write (nsdb_t *smf, txn_t *tx, const void *src, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  if (!smf_ok (smf, tx))
    return -1;
  sb_size *pos = positions (smf, size, bofst, stride, nelem);
  if (!pos)
    return -1;
  for (b_size i = 0; i < nelem; i++)
    memcpy (smf->cur.bytes + pos[i], (const uint8_t *)src + i * size, size);
  free (pos);
  return (sb_size)nelem;
}

sb_size
ns_smfile_read (nsdb_t *smf, txn_t *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  if (!smf_ok (smf, tx))
    return -1;
  sb_size *pos = positions (smf, size, bofst, stride, nelem);
  if (!pos)
    return -1;
  for (b_size i = 0; i < nelem; i++)
    memcpy ((uint8_t *)dest + i * size, smf->cur.bytes + pos[i], size);
  free (pos);
  return (sb_size)nelem;
}

sb_size
ns_smfile_remove (nsdb_t *smf, txn_t *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)
{
  if (!smf_ok (smf, tx))
    return -1;
  if (stride < 1)
    return seterr (smf->err, "stub only supports stride >= 1 for remove"), -1;
  sb_size *pos = positions (smf, size, bofst, stride, nelem);
  if (!pos)
    return -1;
  if (dest)
    for (b_size i = 0; i < nelem; i++)
      memcpy ((uint8_t *)dest + i * size, smf->cur.bytes + pos[i], size);
  sstate *s = &smf->cur;
  for (b_size i = nelem; i-- > 0;) // back to front keeps earlier positions valid
    {
      memmove (s->bytes + pos[i], s->bytes + pos[i] + size, s->nbytes - pos[i] - size);
      s->nbytes -= size;
    }
  free (pos);
  return (sb_size)nelem;
}
