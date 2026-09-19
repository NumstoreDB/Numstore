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
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "numstore/ns_numstore_internal.h"
#include "numstore/numstore.h"

static inline sb_size
numstore_vexecute_query (
    numstore_t           *ns,
    ns_txn_t             *tx,
    struct numstore_plan *plan,
    struct query         *q,
    struct arena_alloc   *valloc,
    struct variable      *vdest
)
{
  ASSERT (tx);

  if (plan->options & NSDB_PLAN_OPT_ALLOCATE_DATA) {
    ASSERT (plan->data == NULL); // Is a valid plan
    return nsdb_execute_malloc (ns->db, tx, q, vdest, valloc, &plan->data, &ns->e);
  } else {
    return nsdb_execute_on_buffer (ns->db, tx, q, vdest, plan->data, plan->dlen, valloc, &ns->e);
  }
}

sb_size
numstore_vexecute (
    numstore_t           *ns,
    ns_txn_t             *txn,
    struct numstore_plan *plan,
    const char           *query_fmt,
    va_list               args
)
{
  // Validate numstore plan
  if (numstore_plan_validate (plan, &ns->e) < 0) {
    return error_trace (&ns->e);
  }

  ALLOC_INIT (temp);

  // Compile the query
  struct query q;
  sb_size      ret = ns_query_fcompile (&temp, query_fmt, args, &q, &ns->e);
  if (ret < 0) {
    goto theend;
  }

  // Allocator to use for the variable (depends on if we're keeping it)
  struct arena_alloc *valloc = &temp;
  struct variable    *vdest  = NULL;

  if (plan->options & NSDB_PLAN_OPT_CAPTURE_VAR) {
    // Create a variable reference to save
    plan->var = nsdb_var_create (ns->db->mem, &ns->e);
    if (plan->var == NULL) {
      ret = error_trace (&ns->e);
      goto theend;
    }

    // Set the allocator to the persistent allocator
    // used in the variable reference
    valloc = &plan->var->alloc;
    vdest  = &plan->var->var;
  }

  // Do the execution step in an auto transaction
  WITH_AUTO_TXN (
      ret,
      ns->db,
      txn,
      numstore_vexecute_query (ns, txn, plan, &q, valloc, vdest),
      &ns->e
  );

theend:
  ALLOC_CLOSE (temp);
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
  TEST_CASE ("Get a non existent variable, create then get")
  {
    ALLOC_INIT (alloc);
    error e = error_create ();

    numstore_cleanup ("test");
    numstore_t          *db       = numstore_open ("test");

    // Get a variable that doesn't exist
    struct numstore_plan get_plan = {0};
    numstore_plan_setopt (&get_plan, NSDB_PLAN_OPT_CAPTURE_VAR);
    sb_size res = numstore_fexecute (db, NULL, &get_plan, "get %s", "a");
    test_assert_int_equal (res, ERR_VARIABLE_NE);
    numstore_perror (db, "get");
    test_assert_equal (get_plan.var->var.dtype, NULL);

    // Create the variable now
    struct numstore_plan create_plan = {0};
    res = numstore_fexecute (db, NULL, &create_plan, "create %s %s", "a", "u32");
    test_assert_int_equal (res, SUCCESS);

    // Get it - should return this time
    memset (&get_plan, 0, sizeof (get_plan));
    numstore_plan_setopt (&get_plan, NSDB_PLAN_OPT_CAPTURE_VAR);
    res = numstore_fexecute (db, NULL, &get_plan, "get %s", "a");
    test_assert_int_equal (res, SUCCESS);
    test_assert (get_plan.var->var.dtype != NULL);

    // Variable name the same
    test_assert (string_equal (get_plan.var->var.vname, strfcstr ("a")));

    // Variable type is the same
    struct type *expected = compile_type_alloc ("u32", &alloc, &e);
    test_assert (type_equal (get_plan.var->var.dtype, expected));

    // Variable length is the same
    test_assert_equal (numstore_var_len (get_plan.var), 0);

    // Clean up
    numstore_var_free (get_plan.var);
    numstore_close (db);

    ALLOC_CLOSE (alloc);
  }
}

TEST (numstore_fexecute_allocate)
{
  // Clean up and open
  numstore_cleanup ("test");
  numstore_t          *db          = numstore_open ("test");

  // Create a variable
  struct numstore_plan create_plan = {0};
  sb_size              ret         = numstore_fexecute (db, NULL, &create_plan, "create foo u32");
  test_assert_int_equal (ret, 0);

  // Insert some data
  u32                  src[5]      = {10, 11, 12, 13, 14};
  struct numstore_plan insert_plan = {.data = src, .dlen = sizeof (src)};
  ret                              = numstore_fexecute (db, NULL, &insert_plan, "insert foo 0 5");
  test_assert_int_equal (ret, 5);

  // Read with allocate
  struct numstore_plan read_plan = {0};
  numstore_plan_setopt (&read_plan, NSDB_PLAN_OPT_ALLOCATE_DATA);
  ret = numstore_fexecute (db, NULL, &read_plan, "read foo[0:]");
  test_assert_int_equal (ret, 5);
  test_assert (read_plan.data != NULL);
  test_assert (memcmp (read_plan.data, src, sizeof (src)) == 0);
  i_free (db->db->mem, read_plan.data);

  // Remove with allocate
  struct numstore_plan remove_plan = {0};
  numstore_plan_setopt (&remove_plan, NSDB_PLAN_OPT_ALLOCATE_DATA);
  ret = numstore_fexecute (db, NULL, &remove_plan, "remove foo[0:2]");
  test_assert_int_equal (ret, 2);
  test_assert (remove_plan.data != NULL);
  u32 *removed = remove_plan.data;
  test_assert_equal (removed[0], 10u);
  test_assert_equal (removed[1], 11u);
  i_free (db->db->mem, remove_plan.data);

  // Read with allocate
  struct numstore_plan remaining_plan = {0};
  numstore_plan_setopt (&remaining_plan, NSDB_PLAN_OPT_ALLOCATE_DATA);
  ret = numstore_fexecute (db, NULL, &remaining_plan, "read foo[0:]");
  test_assert_int_equal (ret, 3);
  test_assert (remaining_plan.data != NULL);
  u32 *remaining = remaining_plan.data;
  test_assert_equal (remaining[0], 12u);
  test_assert_equal (remaining[1], 13u);
  test_assert_equal (remaining[2], 14u);
  i_free (db->db->mem, remaining_plan.data);

  // Insert some more
  u32                  more[1]   = {99};
  struct numstore_plan more_plan = {.data = more, .dlen = sizeof (more)};
  test_assert (numstore_fexecute (db, NULL, &more_plan, "insert foo 0 1") >= 0);

  numstore_close (db);
}
#endif
