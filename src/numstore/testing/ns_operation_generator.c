#include "numstore/testing/ns_operation_generator.h"

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/testing/ns_reference_db_stepper.h"

/******************************************************************************
 *                                  Getters                                   *
 ******************************************************************************/

/**
 * Even though we have enabled actions, some actions should be unavailable.
 * For example, if the variable we're working with is empty, we cannot
 * remove data from it.
 *
 * allowed is the list of allowed actions - derived from the database state
 * and the enabled actions list.
 */
static void
get_allowed (struct ns_ref *ref, u8 allowed[NSS_AT_LEN], const u8 enabled[NSS_AT_LEN])
{
  memset (allowed, 0, NSS_AT_LEN);

  // Can always crash
  allowed[NSS_CRASH_AND_REOPEN] = enabled[NSS_CRASH_AND_REOPEN];

  if (!ref->in_txn) {
    // If you're not in a transaction - you can begin and close
    allowed[NSS_BEGIN_TXN]        = enabled[NSS_BEGIN_TXN];
    allowed[NSS_CLOSE_AND_REOPEN] = enabled[NSS_CLOSE_AND_REOPEN];
  } else {
    // If you're in a transaction - you can commit and rollback
    allowed[NSS_COMMIT_TXN]   = enabled[NSS_COMMIT_TXN];
    allowed[NSS_ROLLBACK_TXN] = enabled[NSS_ROLLBACK_TXN];
  }

  // You can always create a new variable
  allowed[NSS_CREATE] = enabled[NSS_CREATE];

  u32 nvars           = ns_ref_nvars (ref);

  if (nvars > 1) {
    // You can switch if there's another variable
    allowed[NSS_SWITCH] = enabled[NSS_SWITCH];
  }

  if (nvars > 0) {
    allowed[NSS_DELETE] = enabled[NSS_DELETE];

    // You're always guaranteed to have an active
    // variable if there's more than 0 variables
    allowed[NSS_INSERT] = enabled[NSS_INSERT];

    // Can remove, read and write if length > 0
    if (ns_ref_cur_len (ref) > 0) {
      allowed[NSS_REMOVE] = enabled[NSS_REMOVE];
      allowed[NSS_READ]   = enabled[NSS_READ];
      allowed[NSS_WRITE]  = enabled[NSS_WRITE];
    }
  }
}

static enum ns_action_type
get_random_action_type (u8 allowed[NSS_AT_LEN])
{
  // Count the number of allowed actions
  int len = 0;
  for (int i = 0; i < NSS_AT_LEN; ++i) {
    len += allowed[i];
  }

  if (len == 0) {
    return NSS_NONE_AVAILABLE;
  }

  // Pick the n-th allowed action
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < NSS_AT_LEN; ++index) {
    if (allowed[index]) {
      if (choice == next) {
        break;
      }
      choice++;
    }
  }

  return (enum ns_action_type)index;
}

static u32
get_random_name_len (void)
{
  u32 roll = randu32r (1, 100);

  // 90% small
  if (roll <= 90) {
    return randu32r (2, 10);
  }

  // 5% medium
  if (roll <= 95) {
    return randu32r (11, NS_PAGE_SIZE);
  }

  // 5% large
  return randu32r (NS_PAGE_SIZE + 1, 10 * NS_PAGE_SIZE);
}

static char *
get_random_name (struct allocator *alloc, error *e)
{
  u32   length = get_random_name_len ();
  char *buffer = allocate (alloc, length + 1, 1, e);
  if (buffer == NULL) {
    return NULL;
  }

  var_random_name (buffer, length);
  buffer[length] = '\0';

  return buffer;
}

static char *
get_random_unique_name (struct ns_ref *ref, struct allocator *alloc, error *e)
{
  ALLOC_INIT (temp);
  char *ret = NULL;

  while (true) {
    // Generate a random name
    char *candidate = get_random_name (&temp, e);
    if (candidate == NULL) {
      goto theend;
    }

    // Avoid duplicates
    if (ns_ref_var_exists (ref, candidate)) {
      continue;
    }

    // Copy over to ret (including the null terminator)
    u32 len = strlen (candidate);

    ret     = allocate (alloc, len + 1, 1, e);
    if (ret == NULL) {
      goto theend;
    }

    memcpy (ret, candidate, len + 1);
    break;
  }

theend:
  ALLOC_CLOSE (temp);
  return ret;
}

static char *
get_random_other_existing_name (struct ns_ref *ref, struct allocator *alloc, error *e)
{
  ASSERT (ns_ref_nvars (ref) > 1);

  const char *cur = ns_ref_cur_name (ref);

  while (true) {
    const char *cand = ns_ref_random_var (ref);

    // Keep rolling until we land on something that isn't the current variable
    if (strcmp (cur, cand) == 0) {
      continue;
    }

    u32   len = strlen (cand);
    char *ret = allocate (alloc, len + 1, 1, e);
    if (ret == NULL) {
      return NULL;
    }

    memcpy (ret, cand, len + 1);
    return ret;
  }
}

static char *
get_random_other_existing_name_or_null (struct ns_ref *ref, struct allocator *alloc, error *e)
{
  ASSERT (ns_ref_nvars (ref) > 0);

  if (ns_ref_nvars (ref) == 1) {
    return NULL;
  }

  char *ret = get_random_other_existing_name (ref, alloc, e);

  return ret;
}

static struct type *
get_random_type (struct allocator *alloc, error *e)
{
  u32 depth = randu32r (1, 3);
  return type_random (alloc, depth, e);
}

static void
get_random_slice (struct ns_ref *ref, b_size *ofst, b_size *stride, b_size *nelems)
{
  b_size var_len = ns_ref_cur_len (ref);

  ASSERT (var_len > 0);

  // Offset is any value between [0, len - 1]
  *ofst            = randu64r (0, var_len - 1);

  // Stride is anything between [1, len - offset]
  b_size remaining = var_len - *ofst;
  *stride          = randu64r (1, remaining);

  // Elements is between [1, (len - offset + stride - 1) / stride]
  b_size max_len   = (remaining + *stride - 1) / *stride;
  *nelems          = randu64r (1, max_len);
}

static u8 *
get_random_empty_data (struct ns_ref *ref, struct allocator *alloc, b_size nelems, error *e)
{
  t_size size = ns_ref_cur_tsize (ref);

  u8    *data = allocate (alloc, nelems, size, e);
  if (data == NULL) {
    return NULL;
  }

  memset (data, 0, nelems * size);

  return data;
}

static u8 *
get_random_data (struct ns_ref *ref, struct allocator *alloc, b_size nelems, error *e)
{
  t_size size = ns_ref_cur_tsize (ref);

  u8    *data = allocate (alloc, nelems, size, e);
  if (data == NULL) {
    return NULL;
  }

  rand_bytes (data, nelems * size);

  return data;
}

/******************************************************************************
 *                              Action Builders                               *
 ******************************************************************************/

static inline err_t
build_create (struct operation *dest, struct rand_op_params params, error *e)
{
  char        *vname   = get_random_unique_name (params.ref, &dest->alloc, e);
  struct type *t       = get_random_type (&dest->alloc, e);
  char        *typestr = type_tostr (&dest->alloc, t, e);

  if (vname == NULL || t == NULL || typestr == NULL) {
    return error_trace (e);
  }

  dest->op_create.vname   = vname;
  dest->op_create.t       = t;
  dest->op_create.typestr = typestr;

  return SUCCESS;
}

static inline err_t
build_switch (struct operation *dest, struct rand_op_params params, error *e)
{
  char *vname = get_random_other_existing_name (params.ref, &dest->alloc, e);

  if (vname == NULL) {
    return error_trace (e);
  }

  dest->op_switch.vname = vname;

  return SUCCESS;
}

static inline err_t
build_delete (struct operation *dest, struct rand_op_params params, error *e)
{
  char *next = get_random_other_existing_name_or_null (params.ref, &dest->alloc, e);

  if (e->cause_code) {
    return error_trace (e);
  }

  dest->op_delete.next = next;

  return SUCCESS;
}

static inline err_t
build_insert (struct operation *dest, struct rand_op_params params, error *e)
{
  b_size ofst   = randu64r (0, ns_ref_cur_len (params.ref));
  b_size nelems = randu64r (1, params.max_nelems);
  u8    *data   = get_random_data (params.ref, &dest->alloc, nelems, e);

  if (data == NULL) {
    return error_trace (e);
  }

  dest->op_insert.ofst   = ofst;
  dest->op_insert.nelems = nelems;
  dest->op_insert.data   = data;

  return SUCCESS;
}

static inline err_t
build_remove (struct operation *dest, struct rand_op_params params, error *e)
{
  b_size ofst;
  b_size stride;
  b_size nelems;
  get_random_slice (params.ref, &ofst, &stride, &nelems);

  u8 *db_dest  = get_random_empty_data (params.ref, &dest->alloc, nelems, e);
  u8 *ref_dest = get_random_empty_data (params.ref, &dest->alloc, nelems, e);

  if (db_dest == NULL || ref_dest == NULL) {
    return error_trace (e);
  }

  dest->op_remove.start    = ofst;
  dest->op_remove.stride   = stride;
  dest->op_remove.nelems   = nelems;
  dest->op_remove.db_dest  = db_dest;
  dest->op_remove.ref_dest = ref_dest;

  return SUCCESS;
}

static inline err_t
build_read (struct operation *dest, struct rand_op_params params, error *e)
{
  b_size ofst;
  b_size stride;
  b_size nelems;
  get_random_slice (params.ref, &ofst, &stride, &nelems);

  u8 *db_dest  = get_random_empty_data (params.ref, &dest->alloc, nelems, e);
  u8 *ref_dest = get_random_empty_data (params.ref, &dest->alloc, nelems, e);

  if (db_dest == NULL || ref_dest == NULL) {
    return error_trace (e);
  }

  dest->op_read.start    = ofst;
  dest->op_read.stride   = stride;
  dest->op_read.nelems   = nelems;
  dest->op_read.db_dest  = db_dest;
  dest->op_read.ref_dest = ref_dest;

  return SUCCESS;
}

static inline err_t
build_write (struct operation *dest, struct rand_op_params params, error *e)
{
  b_size ofst;
  b_size stride;
  b_size nelems;
  get_random_slice (params.ref, &ofst, &stride, &nelems);

  u8 *data = get_random_data (params.ref, &dest->alloc, nelems, e);
  if (data == NULL) {
    return error_trace (e);
  }

  dest->op_write.start  = ofst;
  dest->op_write.stride = stride;
  dest->op_write.nelems = nelems;
  dest->op_write.data   = data;

  return SUCCESS;
}

/******************************************************************************
 *                                Public API                                  *
 ******************************************************************************/

void
opg_spin_enabled (u8 enabled[NSS_AT_LEN])
{
  int mask = rand () % ((1 << NSS_AT_LEN) - 1) + 1;
  for (int i = 0; i < NSS_AT_LEN; ++i) {
    enabled[i] = (mask >> i) & 1;
  }
}

struct operation *
opg_random (struct rand_op_params params, error *e)
{
  struct operation *ret = i_malloc (params.mem, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  u8 allowed[NSS_AT_LEN];
  get_allowed (params.ref, allowed, params.enabled);

  // Generate a random action (might be none)
  enum ns_action_type type = get_random_action_type (allowed);

  // Build common parameters
  ret->type                = type;
  ret->mem                 = params.mem;
  create_default_allocator (&ret->alloc);

  switch (type) {
      // Nothing to build
    case NSS_BEGIN_TXN:
    case NSS_COMMIT_TXN:
    case NSS_ROLLBACK_TXN:
    case NSS_CRASH_AND_REOPEN:
    case NSS_CLOSE_AND_REOPEN:
    case NSS_NONE_AVAILABLE: {
      return ret;
    }

    case NSS_CREATE: {
      if (build_create (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }
    case NSS_SWITCH: {
      if (build_switch (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }
    case NSS_DELETE: {
      if (build_delete (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }
    case NSS_INSERT: {
      if (build_insert (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }
    case NSS_REMOVE: {
      if (build_remove (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }
    case NSS_READ: {
      if (build_read (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }
    case NSS_WRITE: {
      if (build_write (ret, params, e) < 0) {
        goto failed;
      }
      return ret;
    }

    case NSS_AT_LEN: {
      UNREACHABLE ();
    }
  }

failed:
  allocator_free (&ret->alloc);
  i_free (params.mem, ret);
  return NULL;
}

void
opg_free (struct operation *op)
{
  allocator_free (&op->alloc);
  i_free (op->mem, op);
}

/**
#ifdef TESTING

TEST_DISABLED (opg)
{
  error                 e      = error_create ();
  struct ns_ref        *ref    = ns_ref_new (mem, &e);
  struct rand_op_params params = {
      .ref        = ref,
      .max_nelems = 64,
  };
  memset (params.enabled, 1, NSS_AT_LEN);

  TEST_CASE ("1000 random operations against the reference model")
  {
    for (int i = 0; i < 1000; ++i) {
      struct operation *op = opg_random (params, &e);

      test_assert_int_equal (1, op != NULL);
      test_assert_int_equal (1, params.enabled[op->type] != 0);

      opg_free (op);
    }
  }
}

#endif
*/
