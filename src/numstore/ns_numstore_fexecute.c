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
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "numstore/numstore.h"

// Executes a data operation
sb_size
numstore_vexecute (
    numstore_t           *ns,
    ns_txn_t             *txn,
    struct numstore_plan *plan,
    const char           *query_fmt,
    va_list               args
)
{
  // TODO - validate plan
  ALLOC_INIT (alloc);

  sb_size      ret;
  struct query q;

  ns->e.cause_code = 0;
  ns->e.cmlen      = 0;

  // Compile the query
  ret              = ns_query_fcompile (&alloc, query_fmt, args, &q, &ns->e);
  if (ret < 0) {
    goto theend;
  }

  struct arena_alloc *valloc = &alloc;
  struct variable    *vdest  = NULL;

  // Create a destination variable for capture
  if (plan->options & NSDB_PLAN_OPT_CAPTURE_VAR) {
    plan->var = nsdb_var_create (ns->db->mem, &ns->e);
    vdest     = &plan->var->var;
    if (plan->var == NULL) {
      ret = error_trace (&ns->e);
      goto theend;
    }
    valloc = &plan->var->alloc;
  }

  struct auto_txn auto_tx;
  if (nsdb_auto_begin (ns->db, txn, &auto_tx, &ns->e)) {
    goto theend;
  }

  if (plan->options & NSDB_PLAN_OPT_ALLOCATE_DATA) {
    switch (q.type) {
      case QT_READ: {
        panic ("TODO");
        return 0;
      }
      case QT_REMOVE: {
        panic ("TODO");
        return 0;
      }
      case QT_CREATE: {
        if (numstore_create (
                ns->db->p,
                auto_tx.tx,
                q.create.name,
                q.create.type,
                valloc,
                vdest,
                &ns->e
            )) {
          nsdb_auto_rollback (ns->db, &auto_tx, &ns->e);
          goto theend;
        }
        break;
      }
      case QT_DELETE: {
        if (numstore_delete (ns->db->p, auto_tx.tx, q.delete.name, q.delete.if_exists, &ns->e)) {
          nsdb_auto_rollback (ns->db, &auto_tx, &ns->e);
          goto theend;
        }
        break;
      }
      case QT_GET: {
        if (numstore_get (
                ns->db->p,
                auto_tx.tx,
                q.get.if_exists,
                q.get.name,
                valloc,
                vdest,
                &ns->e
            )) {
          nsdb_auto_rollback (ns->db, &auto_tx, &ns->e);
          goto theend;
        }
        break;
      }
      case QT_EXIT:
      case QT_HELP: {
        // Nothing to do - maybe throw
        goto theend;
      }
      case QT_INSERT:
      case QT_WRITE: {
        error_causef (&ns->e, ERR_INVALID_ARGUMENT, "Must provide data for insert/write");
        goto theend;
      }
    }
  } else {
    // Execute query
    ret = nsdb_execute_on_buffer (
        ns->db,
        auto_tx.tx,
        &q,
        vdest,
        plan->data,
        plan->dlen,
        valloc,
        &ns->e
    );
    if (ret < 0) {
      nsdb_auto_rollback (ns->db, &auto_tx, &ns->e);
      goto theend;
    }
  }

  if (nsdb_auto_commit (ns->db, &auto_tx, &ns->e)) {
    ret = error_trace (&ns->e);
    goto theend;
  }

theend:
  ALLOC_CLOSE (alloc);
  return ret;
}

// Executes a data operation
sb_size
numstore_fexecute (
    numstore_t           *ns,
    ns_txn_t             *txn,
    struct numstore_plan *plan,
    const char           *query_fmt,
    ...
)
{
  va_list ap;
  va_start (ap, query_fmt);
  sb_size ret = numstore_vexecute (ns, txn, plan, query_fmt, ap);
  va_end (ap);
  return ret;
}

#ifdef TESTING
TEST (numstore_fexecute)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

  numstore_cleanup ("test");
  numstore_t          *db       = numstore_open ("test");

  // "get" without "if exists" on a missing var fails, leaving plan.var
  // untouched (still NULL from the {0} init) - matches the original test's
  // implicit expectation that the call doesn't populate var here.
  struct numstore_plan get_plan = {0};
  numstore_plan_setopt (&get_plan, NSDB_PLAN_OPT_CAPTURE_VAR);
  numstore_fexecute (db, NULL, &get_plan, "get %s", "a");
  test_assert_equal (get_plan.var->var.dtype, NULL);

  struct numstore_plan create_plan = {0};
  numstore_fexecute (db, NULL, &create_plan, "create %s %s", "a", "u32");

  numstore_fexecute (db, NULL, &get_plan, "get %s", "a");
  test_assert (get_plan.var->var.dtype != NULL);

  test_assert (string_equal (get_plan.var->var.vname, strfcstr ("a")));
  test_assert (type_equal (get_plan.var->var.dtype, compile_type_alloc ("u32", &alloc, &e)));
  test_assert_equal (numstore_var_len (get_plan.var), 0);
  numstore_var_free (get_plan.var);

  numstore_close (db);
  ALLOC_CLOSE (alloc);
}

/**
TEST_DISABLED (numstore_fexecute_allocate)
{
  numstore_cleanup ("test");
  numstore_t          *db          = numstore_open ("test");

  struct numstore_plan create_plan = {0};
  test_assert_int_equal (numstore_fexecute (db, NULL, &create_plan, "create foo u32"), 0);

  u32                  src[5]      = {10, 11, 12, 13, 14};
  struct numstore_plan insert_plan = {.data = src, .dlen = sizeof (src)};
  test_assert_int_equal (numstore_fexecute (db, NULL, &insert_plan, "insert foo 0 5"), 5);

  // read with data/dlen unset + ALLOCATE_DATA: library allocates the
  // destination buffer itself and hands it back via plan.data
  struct numstore_plan read_plan = {0};
  numstore_plan_setopt (&read_plan, NSDB_PLAN_OPT_ALLOCATE_DATA);
  test_assert (numstore_fexecute (db, NULL, &read_plan, "read foo[0:]") >= 0);
  test_assert (read_plan.data != NULL);
  test_assert (memcmp (read_plan.data, src, sizeof (src)) == 0);
  i_free (mem, read_plan.data);

  // remove with ALLOCATE_DATA returns the removed data the same way
  struct numstore_plan remove_plan = {0};
  numstore_plan_setopt (&remove_plan, NSDB_PLAN_OPT_ALLOCATE_DATA);
  test_assert (numstore_fexecute (db, NULL, &remove_plan, "remove foo[0:2]") >= 0);
  test_assert (remove_plan.data != NULL);
  u32 *removed = remove_plan.data;
  test_assert_equal (removed[0], 10u);
  test_assert_equal (removed[1], 11u);
  i_free (mem, remove_plan.data);

  struct numstore_plan remaining_plan = {0};
  numstore_plan_setopt (&remaining_plan, NSDB_PLAN_OPT_ALLOCATE_DATA);
  test_assert (numstore_fexecute (db, NULL, &remaining_plan, "read foo[0:]") >= 0);
  test_assert (remaining_plan.data != NULL);
  u32 *remaining = remaining_plan.data;
  test_assert_equal (remaining[0], 12u);
  test_assert_equal (remaining[1], 13u);
  test_assert_equal (remaining[2], 14u);
  i_free (mem, remaining_plan.data);

  // insert/write still require the caller's own source data - no
  // ALLOCATE_DATA here, just a plain caller-supplied buffer
  u32                  more[1]   = {99};
  struct numstore_plan more_plan = {.data = more, .dlen = sizeof (more)};
  test_assert (numstore_fexecute (db, NULL, &more_plan, "insert foo 0 1") >= 0);

  numstore_close (db);
}
*/
#endif
