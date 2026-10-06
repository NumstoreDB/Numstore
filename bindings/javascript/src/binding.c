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

/**
 * Node-API bindings for numstore.
 *
 * This layer is deliberately thin: it exposes one JS function per C entry
 * point and leaves ergonomics (classes, defaults, auto-transactions) to
 * lib/index.js. What it *does* own is memory safety:
 *
 *  - Every native pointer lives behind a type-tagged JS object, so a txn
 *    handle can never be passed where a db handle is expected.
 *  - A db wrapper is reference counted by its JS object and by every
 *    transaction/plan created from it, so finalizers can run in any order.
 *  - Plans are freed before their database is closed (as the C API requires).
 *  - Closing a database with open transactions throws instead of blocking
 *    the event loop forever (ns_close would wait on a txn that only this
 *    same thread could finish).
 *  - Query strings are always passed as ("%s", query) so user input is
 *    never interpreted as a printf format string.
 *  - Buffer lengths are checked against (nelem * size) before calling into
 *    the strided smart file APIs.
 */

#include <node_api.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "numstore.h"

#define ERR_LIB   "ENUMSTORE"       // the library reported an error
#define ERR_STATE "ENUMSTORE_STATE" // misuse: closed db, finished txn, freed plan

////////////////////////////////////////////////////////////// Handle wrappers

typedef struct db_w   db_w;
typedef struct txn_w  txn_w;
typedef struct plan_w plan_w;

struct txn_w
{
  txn_t *txn; // NULL once committed, rolled back or the db crashed
  db_w  *db;
  txn_w *prev, *next;
};

struct plan_w
{
  nsdb_plan_t *plan; // NULL once freed (explicitly or by db close)
  db_w        *db;
  plan_w      *prev, *next;
};

struct db_w
{
  nsdb_t *ns;    // NULL once closed / crashed
  bool    smf;   // opened with ns_smfile_open
  int     refs;  // 1 for the JS handle + 1 per txn_w / plan_w
  txn_w  *txns;  // active transactions
  plan_w *plans; // live plans
};

typedef struct
{
  nsdb_var_t *var; // NULL once freed
} var_w;

static const napi_type_tag DB_TAG   = { 0x6e756d73746f7265ULL, 0x0000000000000001ULL };
static const napi_type_tag TXN_TAG  = { 0x6e756d73746f7265ULL, 0x0000000000000002ULL };
static const napi_type_tag PLAN_TAG = { 0x6e756d73746f7265ULL, 0x0000000000000003ULL };
static const napi_type_tag VAR_TAG  = { 0x6e756d73746f7265ULL, 0x0000000000000004ULL };

static void
txn_link (txn_w *t)
{
  t->prev = NULL;
  t->next = t->db->txns;
  if (t->next)
    t->next->prev = t;
  t->db->txns = t;
}

static void
txn_unlink (txn_w *t)
{
  if (t->prev)
    t->prev->next = t->next;
  else
    t->db->txns = t->next;
  if (t->next)
    t->next->prev = t->prev;
  t->prev = t->next = NULL;
}

static void
plan_link (plan_w *p)
{
  p->prev = NULL;
  p->next = p->db->plans;
  if (p->next)
    p->next->prev = p;
  p->db->plans = p;
}

static void
plan_unlink (plan_w *p)
{
  if (p->prev)
    p->prev->next = p->next;
  else
    p->db->plans = p->next;
  if (p->next)
    p->next->prev = p->prev;
  p->prev = p->next = NULL;
}

static void
db_release (db_w *d)
{
  if (--d->refs == 0)
    free (d);
}

static void
free_all_plans (db_w *d)
{
  while (d->plans)
    {
      plan_w *p = d->plans;
      ns_plan_free (p->plan);
      p->plan = NULL;
      plan_unlink (p);
    }
}

// rollback=false is used after ns_crash, when the handles are already gone
static void
drop_all_txns (db_w *d, bool rollback)
{
  while (d->txns)
    {
      txn_w *t = d->txns;
      if (rollback)
        ns_rollback (d->ns, t->txn);
      t->txn = NULL;
      txn_unlink (t);
    }
}

////////////////////////////////////////////////////////////// Finalizers

static void
db_finalize (napi_env env, void *data, void *hint)
{
  (void)env;
  (void)hint;
  db_w *d = data;
  if (d->ns)
    {
      // Garbage collected without close(): shut down gracefully. Roll back
      // anything still open first so ns_close can't block.
      drop_all_txns (d, true);
      free_all_plans (d);
      ns_close (d->ns);
      d->ns = NULL;
    }
  db_release (d);
}

static void
txn_finalize (napi_env env, void *data, void *hint)
{
  (void)env;
  (void)hint;
  txn_w *t = data;
  if (t->txn)
    {
      // An abandoned transaction is never committed implicitly
      ns_rollback (t->db->ns, t->txn);
      t->txn = NULL;
      txn_unlink (t);
    }
  db_release (t->db);
  free (t);
}

static void
plan_finalize (napi_env env, void *data, void *hint)
{
  (void)env;
  (void)hint;
  plan_w *p = data;
  if (p->plan)
    {
      ns_plan_free (p->plan);
      p->plan = NULL;
      plan_unlink (p);
    }
  db_release (p->db);
  free (p);
}

static void
var_finalize (napi_env env, void *data, void *hint)
{
  (void)env;
  (void)hint;
  var_w *v = data;
  if (v->var)
    ns_var_free (v->var);
  free (v);
}

////////////////////////////////////////////////////////////// Error helpers

static void
throw_napi (napi_env env)
{
  const napi_extended_error_info *info = NULL;
  napi_get_last_error_info (env, &info);
  bool pending = false;
  napi_is_exception_pending (env, &pending);
  if (!pending)
    napi_throw_error (env, NULL,
                      info && info->error_message ? info->error_message
                                                  : "Node-API call failed");
}

#define NAPI_CALL(env, call)       \
  do                               \
    {                              \
      if ((call) != napi_ok)       \
        {                          \
          throw_napi (env);        \
          return NULL;             \
        }                          \
    }                              \
  while (0)

static void
throw_db (napi_env env, nsdb_t *ns, const char *fallback)
{
  const char *m = ns ? ns_strerror (ns) : NULL;
  napi_throw_error (env, ERR_LIB, (m && *m) ? m : fallback);
}

static void
throw_plan (napi_env env, nsdb_plan_t *plan, const char *fallback)
{
  const char *m = plan ? ns_plan_strerror (plan) : NULL;
  napi_throw_error (env, ERR_LIB, (m && *m) ? m : fallback);
}

static void
throw_oom (napi_env env)
{
  napi_throw_error (env, "ENOMEM", "out of memory");
}

////////////////////////////////////////////////////////////// Argument helpers

static bool
get_args (napi_env env, napi_callback_info info, size_t n, napi_value *argv)
{
  size_t argc = n;
  if (napi_get_cb_info (env, info, &argc, argv, NULL, NULL) != napi_ok)
    {
      throw_napi (env);
      return false;
    }
  if (argc < n)
    {
      napi_throw_type_error (env, NULL, "not enough arguments");
      return false;
    }
  return true;
}

static void *
unwrap_tagged (napi_env env, napi_value v, const napi_type_tag *tag, const char *what)
{
  napi_valuetype t;
  bool           ok   = false;
  void          *data = NULL;

  if (napi_typeof (env, v, &t) == napi_ok && t == napi_object
      && napi_check_object_type_tag (env, v, tag, &ok) == napi_ok && ok
      && napi_unwrap (env, v, &data) == napi_ok && data)
    return data;

  char msg[96];
  snprintf (msg, sizeof msg, "expected a numstore %s handle", what);
  napi_throw_type_error (env, NULL, msg);
  return NULL;
}

enum
{
  KIND_ANY      = -1,
  KIND_NUMSTORE = 0,
  KIND_SMARTFILE = 1,
};

static db_w *
get_db (napi_env env, napi_value v, int kind)
{
  db_w *d = unwrap_tagged (env, v, &DB_TAG, "database");
  if (!d)
    return NULL;
  if (!d->ns)
    {
      napi_throw_error (env, ERR_STATE, "database is closed");
      return NULL;
    }
  if (kind == KIND_NUMSTORE && d->smf)
    {
      napi_throw_type_error (env, NULL, "operation requires a numstore database, not a smart file");
      return NULL;
    }
  if (kind == KIND_SMARTFILE && !d->smf)
    {
      napi_throw_type_error (env, NULL, "operation requires a smart file, not a numstore database");
      return NULL;
    }
  return d;
}

static txn_w *
get_txn (napi_env env, napi_value v, db_w *owner)
{
  txn_w *t = unwrap_tagged (env, v, &TXN_TAG, "transaction");
  if (!t)
    return NULL;
  if (!t->txn)
    {
      napi_throw_error (env, ERR_STATE, "transaction is no longer active");
      return NULL;
    }
  if (owner && t->db != owner)
    {
      napi_throw_error (env, ERR_STATE, "transaction belongs to a different database");
      return NULL;
    }
  return t;
}

static plan_w *
get_plan (napi_env env, napi_value v)
{
  plan_w *p = unwrap_tagged (env, v, &PLAN_TAG, "plan");
  if (!p)
    return NULL;
  if (!p->plan)
    {
      napi_throw_error (env, ERR_STATE, "plan has been freed (or its database was closed)");
      return NULL;
    }
  return p;
}

// Caller frees
static char *
get_string (napi_env env, napi_value v, const char *what)
{
  size_t len = 0;
  if (napi_get_value_string_utf8 (env, v, NULL, 0, &len) != napi_ok)
    {
      char msg[64];
      snprintf (msg, sizeof msg, "%s must be a string", what);
      napi_throw_type_error (env, NULL, msg);
      return NULL;
    }
  char *s = malloc (len + 1);
  if (!s)
    {
      throw_oom (env);
      return NULL;
    }
  if (napi_get_value_string_utf8 (env, v, s, len + 1, &len) != napi_ok)
    {
      free (s);
      throw_napi (env);
      return NULL;
    }
  if (strlen (s) != len)
    {
      free (s);
      napi_throw_type_error (env, NULL, "strings passed to numstore must not contain NUL characters");
      return NULL;
    }
  return s;
}

// The JS layer normalises every buffer-like into a Uint8Array view, so the
// native side only has to deal with one shape. null/undefined is accepted
// (yielding data=NULL, len=0) when allow_null is set.
static bool
get_bytes (napi_env env, napi_value v, bool allow_null, void **data, size_t *len)
{
  napi_valuetype vt;
  if (napi_typeof (env, v, &vt) != napi_ok)
    {
      throw_napi (env);
      return false;
    }
  if (allow_null && (vt == napi_null || vt == napi_undefined))
    {
      *data = NULL;
      *len  = 0;
      return true;
    }

  bool is_ta = false;
  napi_is_typedarray (env, v, &is_ta);
  napi_typedarray_type type;
  napi_value           ab;
  size_t               ofst;
  if (!is_ta
      || napi_get_typedarray_info (env, v, &type, len, data, &ab, &ofst) != napi_ok
      || type != napi_uint8_array)
    {
      napi_throw_type_error (env, NULL, "expected a Uint8Array");
      return false;
    }
  // napi_get_typedarray_info already adjusts data by byte_offset
  return true;
}

static bool
get_i64 (napi_env env, napi_value v, const char *what, int64_t *out)
{
  napi_valuetype vt;
  napi_typeof (env, v, &vt);
  if (vt == napi_number)
    {
      double d;
      napi_get_value_double (env, v, &d);
      if (d != d || d < -9223372036854775808.0 || d >= 9223372036854775808.0 || (double)(int64_t)d != d)
        goto bad;
      *out = (int64_t)d;
      return true;
    }
  if (vt == napi_bigint)
    {
      bool lossless = false;
      if (napi_get_value_bigint_int64 (env, v, out, &lossless) == napi_ok && lossless)
        return true;
    }
bad:;
  char msg[96];
  snprintf (msg, sizeof msg, "%s must be an integer that fits in 64 bits", what);
  napi_throw_range_error (env, NULL, msg);
  return false;
}

static napi_value
make_i64 (napi_env env, int64_t n)
{
  napi_value out;
  NAPI_CALL (env, napi_create_int64 (env, n, &out));
  return out;
}

// Numbers when exactly representable, BigInt beyond 2^53
static napi_value
make_u64 (napi_env env, uint64_t n)
{
  napi_value out;
  if (n <= 9007199254740992ULL)
    NAPI_CALL (env, napi_create_double (env, (double)n, &out));
  else
    NAPI_CALL (env, napi_create_bigint_uint64 (env, n, &out));
  return out;
}

static napi_value
make_bool (napi_env env, bool b)
{
  napi_value out;
  NAPI_CALL (env, napi_get_boolean (env, b, &out));
  return out;
}

static napi_value
undefined (napi_env env)
{
  napi_value out;
  NAPI_CALL (env, napi_get_undefined (env, &out));
  return out;
}

// Creates a tagged JS object wrapping [ptr]. On failure nothing is attached
// (the caller still owns [ptr]) and an exception is pending.
static napi_value
make_handle (napi_env env, void *ptr, const napi_type_tag *tag, napi_finalize fin)
{
  napi_value obj;
  if (napi_create_object (env, &obj) != napi_ok)
    goto fail;
  if (napi_type_tag_object (env, obj, tag) != napi_ok)
    goto fail;
  if (napi_wrap (env, obj, ptr, fin, NULL, NULL) != napi_ok)
    goto fail;
  return obj;
fail:
  throw_napi (env);
  return NULL;
}

////////////////////////////////////////////////////////////// Lifecycle

static napi_value
open_common (napi_env env, napi_callback_info info, bool smf)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  char *path = get_string (env, argv[0], "path");
  if (!path)
    return NULL;

  db_w *d = calloc (1, sizeof *d);
  if (!d)
    {
      free (path);
      throw_oom (env);
      return NULL;
    }

  d->ns = smf ? ns_smfile_open (path) : ns_open (path);
  if (!d->ns)
    {
      size_t n   = strlen (path) + 64;
      char  *msg = malloc (n);
      if (msg)
        snprintf (msg, n, "failed to open %s '%s'", smf ? "smart file" : "database", path);
      napi_throw_error (env, ERR_LIB, msg ? msg : "failed to open");
      free (msg);
      free (path);
      free (d);
      return NULL;
    }
  free (path);
  d->smf  = smf;
  d->refs = 1;

  napi_value h = make_handle (env, d, &DB_TAG, db_finalize);
  if (!h)
    {
      ns_close (d->ns);
      free (d);
    }
  return h;
}

static napi_value
js_open (napi_env env, napi_callback_info info)
{
  return open_common (env, info, false);
}

static napi_value
js_smfile_open (napi_env env, napi_callback_info info)
{
  return open_common (env, info, true);
}

static napi_value
js_cleanup (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  char *path = get_string (env, argv[0], "path");
  if (!path)
    return NULL;
  int rc = ns_cleanup (path);
  if (rc < 0)
    {
      size_t n   = strlen (path) + 48;
      char  *msg = malloc (n);
      if (msg)
        snprintf (msg, n, "failed to clean up '%s'", path);
      napi_throw_error (env, ERR_LIB, msg ? msg : "ns_cleanup failed");
      free (msg);
      free (path);
      return NULL;
    }
  free (path);
  return undefined (env);
}

static napi_value
js_close (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_ANY);
  if (!d)
    return NULL;

  if (d->txns)
    {
      size_t n = 0;
      for (txn_w *t = d->txns; t; t = t->next)
        n++;
      char msg[160];
      snprintf (msg, sizeof msg,
                "cannot close: %zu transaction%s still open (commit or roll back first, "
                "or use crash())",
                n, n == 1 ? " is" : "s are");
      napi_throw_error (env, ERR_STATE, msg);
      return NULL;
    }

  free_all_plans (d);
  int rc = ns_close (d->ns);
  d->ns  = NULL; // the handle is gone either way
  if (rc < 0)
    {
      napi_throw_error (env, ERR_LIB, "ns_close failed");
      return NULL;
    }
  return undefined (env);
}

static napi_value
js_crash (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_ANY);
  if (!d)
    return NULL;

  free_all_plans (d);
  drop_all_txns (d, false); // rolled back on next open
  int rc = ns_crash (d->ns);
  d->ns  = NULL;
  if (rc < 0)
    {
      napi_throw_error (env, ERR_LIB, "ns_crash failed");
      return NULL;
    }
  return undefined (env);
}

static napi_value
js_db_is_open (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  db_w *d = unwrap_tagged (env, argv[0], &DB_TAG, "database");
  return d ? make_bool (env, d->ns != NULL) : NULL;
}

////////////////////////////////////////////////////////////// Transactions

static napi_value
js_begin (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_ANY);
  if (!d)
    return NULL;

  txn_w *t = calloc (1, sizeof *t);
  if (!t)
    {
      throw_oom (env);
      return NULL;
    }
  t->txn = ns_begin (d->ns);
  if (!t->txn)
    {
      free (t);
      throw_db (env, d->ns, "ns_begin failed");
      return NULL;
    }
  t->db = d;
  d->refs++;
  txn_link (t);

  napi_value h = make_handle (env, t, &TXN_TAG, txn_finalize);
  if (!h)
    {
      ns_rollback (d->ns, t->txn);
      txn_unlink (t);
      db_release (d);
      free (t);
    }
  return h;
}

static napi_value
js_commit (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  txn_w *t = get_txn (env, argv[0], NULL);
  if (!t)
    return NULL;

  if (ns_commit (t->db->ns, t->txn) < 0)
    {
      // Left active so the caller can still roll back
      throw_db (env, t->db->ns, "ns_commit failed");
      return NULL;
    }
  t->txn = NULL;
  txn_unlink (t);
  return undefined (env);
}

static napi_value
js_rollback (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  txn_w *t = get_txn (env, argv[0], NULL);
  if (!t)
    return NULL;

  int rc = ns_rollback (t->db->ns, t->txn);
  t->txn = NULL; // nothing more we can do with it either way
  txn_unlink (t);
  if (rc < 0)
    {
      throw_db (env, t->db->ns, "ns_rollback failed");
      return NULL;
    }
  return undefined (env);
}

static napi_value
js_txn_is_active (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  txn_w *t = unwrap_tagged (env, argv[0], &TXN_TAG, "transaction");
  return t ? make_bool (env, t->txn != NULL) : NULL;
}

////////////////////////////////////////////////////////////// Plans

static napi_value
js_plan_create (napi_env env, napi_callback_info info)
{
  napi_value argv[2];
  if (!get_args (env, info, 2, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_NUMSTORE);
  if (!d)
    return NULL;
  char *q = get_string (env, argv[1], "query");
  if (!q)
    return NULL;

  plan_w *p = calloc (1, sizeof *p);
  if (!p)
    {
      free (q);
      throw_oom (env);
      return NULL;
    }
  p->plan = ns_plan_fcreate (d->ns, "%s", q);
  free (q);
  if (!p->plan)
    {
      free (p);
      throw_db (env, d->ns, "ns_plan_fcreate failed");
      return NULL;
    }
  p->db = d;
  d->refs++;
  plan_link (p);

  napi_value h = make_handle (env, p, &PLAN_TAG, plan_finalize);
  if (!h)
    {
      ns_plan_free (p->plan);
      plan_unlink (p);
      db_release (d);
      free (p);
    }
  return h;
}

static napi_value
js_plan_free (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  plan_w *p = unwrap_tagged (env, argv[0], &PLAN_TAG, "plan");
  if (!p)
    return NULL;
  if (p->plan) // idempotent
    {
      ns_plan_free (p->plan);
      p->plan = NULL;
      plan_unlink (p);
    }
  return undefined (env);
}

static napi_value
js_plan_is_live (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  plan_w *p = unwrap_tagged (env, argv[0], &PLAN_TAG, "plan");
  return p ? make_bool (env, p->plan != NULL) : NULL;
}

////////////////////////////////////////////////////////////// Variables

static napi_value
wrap_var (napi_env env, nsdb_var_t *var)
{
  var_w *v = malloc (sizeof *v);
  if (!v)
    {
      ns_var_free (var);
      throw_oom (env);
      return NULL;
    }
  v->var       = var;
  napi_value h = make_handle (env, v, &VAR_TAG, var_finalize);
  if (!h)
    {
      ns_var_free (var);
      free (v);
    }
  return h;
}

static napi_value
js_var_len (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  var_w *v = unwrap_tagged (env, argv[0], &VAR_TAG, "variable");
  if (!v)
    return NULL;
  if (!v->var)
    {
      napi_throw_error (env, ERR_STATE, "variable has been freed");
      return NULL;
    }
  return make_u64 (env, ns_var_len (v->var));
}

static napi_value
js_var_free (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  var_w *v = unwrap_tagged (env, argv[0], &VAR_TAG, "variable");
  if (!v)
    return NULL;
  if (v->var)
    {
      ns_var_free (v->var);
      v->var = NULL;
    }
  return undefined (env);
}

static napi_value
js_var_is_live (napi_env env, napi_callback_info info)
{
  napi_value argv[1];
  if (!get_args (env, info, 1, argv))
    return NULL;
  var_w *v = unwrap_tagged (env, argv[0], &VAR_TAG, "variable");
  return v ? make_bool (env, v->var != NULL) : NULL;
}

////////////////////////////////////////////////////////////// Query execution
//
// Direct form:  fn(db, txn, query, ...)
// Plan form:    fn(plan, txn, ...)

static napi_value
js_execute (napi_env env, napi_callback_info info)
{
  napi_value argv[3];
  if (!get_args (env, info, 3, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_NUMSTORE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  char *q = get_string (env, argv[2], "query");
  if (!q)
    return NULL;

  int rc = ns_execute (d->ns, t->txn, "%s", q);
  free (q);
  if (rc < 0)
    {
      throw_db (env, d->ns, "ns_execute failed");
      return NULL;
    }
  return undefined (env);
}

static napi_value
js_plan_execute (napi_env env, napi_callback_info info)
{
  napi_value argv[2];
  if (!get_args (env, info, 2, argv))
    return NULL;
  plan_w *p = get_plan (env, argv[0]);
  if (!p)
    return NULL;
  txn_w *t = get_txn (env, argv[1], p->db);
  if (!t)
    return NULL;

  if (ns_plan_execute (p->plan, t->txn) < 0)
    {
      throw_plan (env, p->plan, "ns_plan_execute failed");
      return NULL;
    }
  return undefined (env);
}

static napi_value
js_get_var (napi_env env, napi_callback_info info)
{
  napi_value argv[3];
  if (!get_args (env, info, 3, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_NUMSTORE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  char *q = get_string (env, argv[2], "query");
  if (!q)
    return NULL;

  nsdb_var_t *var = ns_get_var (d->ns, t->txn, "%s", q);
  free (q);
  if (!var)
    {
      throw_db (env, d->ns, "ns_get_var failed");
      return NULL;
    }
  return wrap_var (env, var);
}

static napi_value
js_plan_get_var (napi_env env, napi_callback_info info)
{
  napi_value argv[2];
  if (!get_args (env, info, 2, argv))
    return NULL;
  plan_w *p = get_plan (env, argv[0]);
  if (!p)
    return NULL;
  txn_w *t = get_txn (env, argv[1], p->db);
  if (!t)
    return NULL;

  nsdb_var_t *var = ns_plan_get_var (p->plan, t->txn);
  if (!var)
    {
      throw_plan (env, p->plan, "ns_plan_get_var failed");
      return NULL;
    }
  return wrap_var (env, var);
}

static napi_value
js_read (napi_env env, napi_callback_info info)
{
  napi_value argv[4];
  if (!get_args (env, info, 4, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_NUMSTORE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  void  *dest;
  size_t dlen;
  if (!get_bytes (env, argv[3], false, &dest, &dlen))
    return NULL;
  char *q = get_string (env, argv[2], "query");
  if (!q)
    return NULL;

  sb_size n = ns_read (d->ns, t->txn, dest, (b_size)dlen, "%s", q);
  free (q);
  if (n < 0)
    {
      throw_db (env, d->ns, "ns_read failed");
      return NULL;
    }
  return make_i64 (env, n);
}

static napi_value
js_plan_read (napi_env env, napi_callback_info info)
{
  napi_value argv[3];
  if (!get_args (env, info, 3, argv))
    return NULL;
  plan_w *p = get_plan (env, argv[0]);
  if (!p)
    return NULL;
  txn_w *t = get_txn (env, argv[1], p->db);
  if (!t)
    return NULL;
  void  *dest;
  size_t dlen;
  if (!get_bytes (env, argv[2], false, &dest, &dlen))
    return NULL;

  sb_size n = ns_plan_read (p->plan, t->txn, dest, (b_size)dlen);
  if (n < 0)
    {
      throw_plan (env, p->plan, "ns_plan_read failed");
      return NULL;
    }
  return make_i64 (env, n);
}

static napi_value
js_write (napi_env env, napi_callback_info info)
{
  napi_value argv[4];
  if (!get_args (env, info, 4, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_NUMSTORE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  void  *src;
  size_t slen;
  if (!get_bytes (env, argv[3], false, &src, &slen))
    return NULL;
  char *q = get_string (env, argv[2], "query");
  if (!q)
    return NULL;

  sb_size n = ns_write (d->ns, t->txn, src, (b_size)slen, "%s", q);
  free (q);
  if (n < 0)
    {
      throw_db (env, d->ns, "ns_write failed");
      return NULL;
    }
  return make_i64 (env, n);
}

static napi_value
js_plan_write (napi_env env, napi_callback_info info)
{
  napi_value argv[3];
  if (!get_args (env, info, 3, argv))
    return NULL;
  plan_w *p = get_plan (env, argv[0]);
  if (!p)
    return NULL;
  txn_w *t = get_txn (env, argv[1], p->db);
  if (!t)
    return NULL;
  void  *src;
  size_t slen;
  if (!get_bytes (env, argv[2], false, &src, &slen))
    return NULL;

  sb_size n = ns_plan_write (p->plan, t->txn, src, (b_size)slen);
  if (n < 0)
    {
      throw_plan (env, p->plan, "ns_plan_write failed");
      return NULL;
    }
  return make_i64 (env, n);
}

// Copies a malloc'd result into a JS Buffer and frees it. Copying (instead of
// an external buffer) keeps this working in runtimes that forbid external
// buffers, e.g. Electron with the V8 sandbox.
static napi_value
adopt_malloc (napi_env env, void *buf, b_size len)
{
  napi_value out;
  void      *copy;
  if (len > (b_size)SIZE_MAX)
    {
      free (buf);
      napi_throw_range_error (env, NULL, "result too large for this platform");
      return NULL;
    }
  napi_status s = napi_create_buffer_copy (env, (size_t)len, len ? buf : "", &copy, &out);
  free (buf);
  if (s != napi_ok)
    {
      throw_napi (env);
      return NULL;
    }
  return out;
}

static napi_value
js_read_all (napi_env env, napi_callback_info info)
{
  napi_value argv[3];
  if (!get_args (env, info, 3, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_NUMSTORE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  char *q = get_string (env, argv[2], "query");
  if (!q)
    return NULL;

  // A NULL result with dlen set to 0 is a successful empty read; anything
  // else NULL is an error. The sentinel tells the two apart.
  b_size dlen = UINT64_MAX;
  void  *buf  = ns_malloc (d->ns, t->txn, &dlen, "%s", q);
  free (q);
  if (!buf && dlen != 0)
    {
      throw_db (env, d->ns, "ns_malloc failed");
      return NULL;
    }
  return adopt_malloc (env, buf, buf ? dlen : 0);
}

static napi_value
js_plan_read_all (napi_env env, napi_callback_info info)
{
  napi_value argv[2];
  if (!get_args (env, info, 2, argv))
    return NULL;
  plan_w *p = get_plan (env, argv[0]);
  if (!p)
    return NULL;
  txn_w *t = get_txn (env, argv[1], p->db);
  if (!t)
    return NULL;

  b_size dlen = UINT64_MAX;
  void  *buf  = ns_plan_malloc (p->plan, t->txn, &dlen);
  if (!buf && dlen != 0)
    {
      throw_plan (env, p->plan, "ns_plan_malloc failed");
      return NULL;
    }
  return adopt_malloc (env, buf, buf ? dlen : 0);
}

////////////////////////////////////////////////////////////// Smart files

static napi_value
js_smfile_size (napi_env env, napi_callback_info info)
{
  napi_value argv[2];
  if (!get_args (env, info, 2, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_SMARTFILE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;

  sb_size n = ns_smfile_size (d->ns, t->txn);
  if (n < 0)
    {
      throw_db (env, d->ns, "ns_smfile_size failed");
      return NULL;
    }
  return make_i64 (env, n);
}

// smfileInsert(db, txn, src, bofst)
static napi_value
js_smfile_insert (napi_env env, napi_callback_info info)
{
  napi_value argv[4];
  if (!get_args (env, info, 4, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_SMARTFILE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  void  *src;
  size_t slen;
  if (!get_bytes (env, argv[2], false, &src, &slen))
    return NULL;
  int64_t bofst;
  if (!get_i64 (env, argv[3], "offset", &bofst))
    return NULL;

  sb_size n = ns_smfile_insert (d->ns, t->txn, src, bofst, (b_size)slen);
  if (n < 0)
    {
      throw_db (env, d->ns, "ns_smfile_insert failed");
      return NULL;
    }
  return make_i64 (env, n);
}


// fn(db, txn, buf, size, bofst, stride, nelem)
static napi_value
strided (napi_env env, napi_callback_info info, const char *name, bool allow_null, bool is_write)
{
  napi_value argv[7];
  if (!get_args (env, info, 7, argv))
    return NULL;
  db_w *d = get_db (env, argv[0], KIND_SMARTFILE);
  if (!d)
    return NULL;
  txn_w *t = get_txn (env, argv[1], d);
  if (!t)
    return NULL;
  void  *buf;
  size_t blen;
  if (!get_bytes (env, argv[2], allow_null, &buf, &blen))
    return NULL;

  int64_t size, bofst, stride, nelem;
  if (!get_i64 (env, argv[3], "elementSize", &size) || !get_i64 (env, argv[4], "offset", &bofst)
      || !get_i64 (env, argv[5], "stride", &stride) || !get_i64 (env, argv[6], "count", &nelem))
    return NULL;

  if (size < 1 || (uint64_t)size > UINT32_MAX)
    {
      napi_throw_range_error (env, NULL, "elementSize must be between 1 and 2^32-1");
      return NULL;
    }
  if (nelem < 0)
    {
      napi_throw_range_error (env, NULL, "count must be >= 0");
      return NULL;
    }
  // Never let the library touch memory outside the JS buffer
  if (buf && (uint64_t)nelem > (uint64_t)blen / (uint64_t)size)
    {
      char msg[160];
      snprintf (msg, sizeof msg, "buffer too small: %" PRId64 " elements of %" PRId64 " bytes need more than %zu bytes",
                nelem, size, blen);
      napi_throw_range_error (env, NULL, msg);
      return NULL;
    }

  sb_size n;
  if (is_write)
    n = ns_smfile_write (d->ns, t->txn, buf, (t_size)size, bofst, stride, (b_size)nelem);
  else if (allow_null)
    n = ns_smfile_remove (d->ns, t->txn, buf, (t_size)size, bofst, stride, (b_size)nelem);
  else
    n = ns_smfile_read (d->ns, t->txn, buf, (t_size)size, bofst, stride, (b_size)nelem);

  if (n < 0)
    {
      throw_db (env, d->ns, name);
      return NULL;
    }
  return make_i64 (env, n);
}

static napi_value
js_smfile_write (napi_env env, napi_callback_info info)
{
  return strided (env, info, "ns_smfile_write failed", false, true);
}

static napi_value
js_smfile_read (napi_env env, napi_callback_info info)
{
  return strided (env, info, "ns_smfile_read failed", false, false);
}

static napi_value
js_smfile_remove (napi_env env, napi_callback_info info)
{
  return strided (env, info, "ns_smfile_remove failed", true, false);
}

////////////////////////////////////////////////////////////// Module

#define FN(name, fn) { name, NULL, fn, NULL, NULL, NULL, napi_enumerable, NULL }

NAPI_MODULE_INIT ()
{
  napi_property_descriptor fns[] = {
    FN ("open", js_open),
    FN ("smfileOpen", js_smfile_open),
    FN ("cleanup", js_cleanup),
    FN ("close", js_close),
    FN ("crash", js_crash),
    FN ("dbIsOpen", js_db_is_open),

    FN ("begin", js_begin),
    FN ("commit", js_commit),
    FN ("rollback", js_rollback),
    FN ("txnIsActive", js_txn_is_active),

    FN ("planCreate", js_plan_create),
    FN ("planFree", js_plan_free),
    FN ("planIsLive", js_plan_is_live),

    FN ("varLen", js_var_len),
    FN ("varFree", js_var_free),
    FN ("varIsLive", js_var_is_live),

    FN ("execute", js_execute),
    FN ("planExecute", js_plan_execute),
    FN ("getVar", js_get_var),
    FN ("planGetVar", js_plan_get_var),
    FN ("read", js_read),
    FN ("planRead", js_plan_read),
    FN ("write", js_write),
    FN ("planWrite", js_plan_write),
    FN ("readAll", js_read_all),
    FN ("planReadAll", js_plan_read_all),

    FN ("smfileSize", js_smfile_size),
    FN ("smfileInsert", js_smfile_insert),
    FN ("smfileWrite", js_smfile_write),
    FN ("smfileRead", js_smfile_read),
    FN ("smfileRemove", js_smfile_remove),
  };
  NAPI_CALL (env, napi_define_properties (env, exports, sizeof fns / sizeof fns[0], fns));

  napi_value constants, v;
  NAPI_CALL (env, napi_create_object (env, &constants));
  NAPI_CALL (env, napi_create_uint32 (env, NS_PAGE_SIZE, &v));
  NAPI_CALL (env, napi_set_named_property (env, constants, "PAGE_SIZE", v));
  NAPI_CALL (env, napi_create_bigint_int64 (env, NS_END, &v));
  NAPI_CALL (env, napi_set_named_property (env, constants, "END", v));
  NAPI_CALL (env, napi_create_bigint_int64 (env, SMF_END, &v));
  NAPI_CALL (env, napi_set_named_property (env, constants, "SMF_END", v));
  NAPI_CALL (env, napi_set_named_property (env, exports, "constants", constants));

  return exports;
}
