#include "numstore/testing/ns_reference_db_stepper.h"

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_ext_array.h"
#include "core/ns_string.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_types.h"
#include "numstore/testing/ns_mem_vhmap.h"

#include <string.h>

static inline struct db_state *
ns_ref_cur (struct ns_ref *ref)
{
  if (ref->in_txn) {
    return ref->working;
  } else {
    return ref->committed;
  }
}

// DB STATE

static struct db_state *
db_state_create (struct i_mem mem, error *e)
{
  struct db_state *ret = i_malloc (mem, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  ret->db_data = mem_vhmap_create (mem, e);
  if (ret->db_data == NULL) {
    i_free (mem, ret);
    return NULL;
  }

  ret->cur           = NULL;
  ret->nvars         = 0;
  ret->tracked_bytes = 0;
  ret->mem           = mem;

  return ret;
}

static void
db_state_free (struct db_state *db)
{
  mem_vhmap_free (db->db_data);
  struct i_mem mem = db->mem;
  i_free (mem, db);
}

struct db_state *
db_state_clone (struct i_mem mem, struct db_state *state, error *e)
{
  struct db_state *ret = db_state_create (mem, e);
  if (ret == NULL) {
    return NULL;
  }

  // Clone db_data
  struct mem_vhmap *cloned_db_data = mem_vhmap_clone (mem, state->db_data, e);
  if (cloned_db_data == NULL) {
    db_state_free (ret);
    return NULL;
  }

  // Clone variable name
  struct var_with_data *new_cur = NULL;
  if (state->cur) {
    new_cur = mem_vhmap_get (cloned_db_data, state->cur->var.vname);
    if (new_cur == NULL) {
      mem_vhmap_free (cloned_db_data);
      db_state_free (ret);
      return NULL;
    }
  }

  // Free the one that was given to us by create
  mem_vhmap_free (ret->db_data);

  ret->db_data       = cloned_db_data;
  ret->cur           = new_cur;
  ret->nvars         = state->nvars;
  ret->tracked_bytes = state->tracked_bytes;
  ret->mem           = mem;

  return ret;
}

// NS_REF

struct ns_ref *
ns_ref_new (struct i_mem mem, error *e)
{
  struct ns_ref *ref = i_malloc (mem, 1, sizeof *ref, e);
  if (ref == NULL) {
    return NULL;
  }

  struct db_state *committed = db_state_create (mem, e);
  if (committed == NULL) {
    i_free (mem, ref);
    return NULL;
  }

  *ref = (struct ns_ref){
      .committed = committed,
      .working   = NULL,
      .mem       = mem,
      .in_txn    = false,
  };

  return ref;
}

void
ns_ref_free (struct ns_ref *ref)
{
  if (ref->working) {
    db_state_free (ref->working);
  }
  if (ref->committed) {
    db_state_free (ref->committed);
  }
  i_free (ref->mem, ref);
}

// Return nu
u32
ns_ref_nvars (struct ns_ref *ref)
{
  return ns_ref_cur (ref)->nvars;
}

const char *
ns_ref_cur_name (struct ns_ref *ref)
{
  struct db_state *state = ns_ref_cur (ref);
  ASSERT (state->cur);
  return state->cur->var.vname.data;
}

b_size
ns_ref_cur_len (struct ns_ref *ref)
{
  struct db_state *state = ns_ref_cur (ref);
  ASSERT (state->cur);
  return ext_array_get_len (&state->cur->data);
}

t_size
ns_ref_cur_tsize (struct ns_ref *ref)
{
  struct db_state *state = ns_ref_cur (ref);
  ASSERT (state->cur);
  return type_byte_size (state->cur->var.dtype);
}

bool
ns_ref_var_exists (struct ns_ref *ref, const char *name)
{
  struct db_state *state = ns_ref_cur (ref);
  return mem_vhmap_get (state->db_data, strfcstr (name)) != NULL;
}

const char *
ns_ref_random_var (struct ns_ref *ref)
{
  struct db_state *state = ns_ref_cur (ref);
  return mem_vhmap_random (state->db_data)->var.vname.data;
}

err_t
ns_ref_begin_txn (struct ns_ref *ref, error *e)
{
  ASSERT (ref);
  ASSERT (!ref->in_txn);
  ASSERT (ref->working == NULL);
  ASSERT (ref->committed);

  ref->working = db_state_clone (ref->mem, ref->committed, e);

  if (ref->working == NULL) {
    return error_trace (e);
  }

  ref->in_txn = true;

  return SUCCESS;
}

void
ns_ref_rollback_txn (struct ns_ref *ref)
{
  ASSERT (ref);
  ASSERT (ref->in_txn);
  ASSERT (ref->working);
  ASSERT (ref->committed);

  db_state_free (ref->working);
  ref->working = NULL;
  ref->in_txn  = false;
}

err_t
ns_ref_commit_txn (struct ns_ref *ref, error *e)
{
  ASSERT (ref);
  ASSERT (ref->in_txn);
  ASSERT (ref->working);
  ASSERT (ref->committed);

  struct db_state *new_committed = db_state_clone (ref->mem, ref->working, e);
  if (new_committed == NULL) {
    return error_trace (e);
  }

  db_state_free (ref->committed);
  db_state_free (ref->working);

  ref->committed = new_committed;
  ref->working   = NULL;
  ref->in_txn    = false;

  return SUCCESS;
}

void
ns_ref_crash_and_reopen (struct ns_ref *ref)
{
  if (ref->in_txn) {
    ns_ref_rollback_txn (ref);
  }
}

void
ns_ref_close_and_reopen (struct ns_ref *ref)
{
  ASSERT (!ref->in_txn);
  (void)ref;
}

err_t
ns_ref_create (struct ns_ref *ref, const char *vname, struct type *type, error *e)
{
  struct db_state *state = ns_ref_cur (ref);

  struct variable  var   = {
      .vname    = strfcstr (vname),
      .dtype    = type,
      .nbytes   = 0,
      .rpt_root = 0,
      .var_root = 0,
  };
  struct var_with_data *data = mem_vhmap_add (state->db_data, &var, e);

  if (data == NULL) {
    return error_trace (e);
  }

  state->nvars += 1;

  if (state->cur == NULL) {
    state->cur = data;
  }

  return SUCCESS;
}

void
ns_ref_switch (struct ns_ref *ref, const char *next)
{
  struct var_with_data *data = mem_vhmap_get (ns_ref_cur (ref)->db_data, strfcstr (next));

  // Shouldn't fail - provide a valid variable name
  ASSERT (data);

  ns_ref_cur (ref)->cur = data;
}

void
ns_ref_delete (struct ns_ref *ref, const char *next)
{
  struct db_state      *state = ns_ref_cur (ref);
  struct var_with_data *data  = NULL;
  if (next != NULL) {
    mem_vhmap_get (state->db_data, strfcstr (next));
  }

  // Shouldn't pass the same variable
  ASSERT (state->cur);
  ASSERT (data != state->cur);
  ASSERT (state->nvars > 0);

  mem_vhmap_remove (state->db_data, state->cur->var.vname);
  state->cur = data;
  state->nvars -= 1;
}

err_t
ns_ref_insert (struct ns_ref *ref, void *data, b_size ofst, b_size len, error *e)
{
  struct db_state *state = ns_ref_cur (ref);
  ASSERT (state->cur);

  t_size size = type_byte_size (state->cur->var.dtype);
  if (ext_array_insert (&state->cur->data, ofst, data, len * size, e) < 0) {
    return error_trace (e);
  }

  state->tracked_bytes += len * size;

  return SUCCESS;
}

void
ns_ref_remove (struct ns_ref *ref, void *dest, struct stride str)
{
  struct db_state      *state = ns_ref_cur (ref);
  struct var_with_data *cur   = state->cur;
  ASSERT (cur);

  t_size size = type_byte_size (cur->var.dtype);
  ext_array_remove (&cur->data, str, size, dest);
  state->tracked_bytes -= size;
}

void
ns_ref_read (struct ns_ref *ref, void *dest, struct stride str)
{
  struct var_with_data *cur = ns_ref_cur (ref)->cur;
  ASSERT (cur);
  ext_array_read (&cur->data, str, type_byte_size (cur->var.dtype), dest);
}

void
ns_ref_write (struct ns_ref *ref, void *data, struct stride str)
{
  struct var_with_data *cur = ns_ref_cur (ref)->cur;
  ASSERT (cur);
  ext_array_write (&cur->data, str, type_byte_size (cur->var.dtype), data);
}

/**
#ifdef TESTING

TEST_DISABLED (ns_ref)
{
  error          e   = error_create ();
  struct ns_ref *ref = ns_ref_new (mem, &e);
  ALLOC_INIT (alloc);

  TEST_CASE ("create, switch, write, read, insert, remove, delete")
  {
    struct type type;
    compile_type (&type, "u32", &alloc, &e);

    ns_ref_create (ref, "test_var", &type, &e);
    ns_ref_create (ref, "var2", &type, &e);
    ns_ref_create (ref, "var3", &type, &e);

    ns_ref_begin_txn (ref, &e);
    {
      u32           write_buf[4] = {10, 20, 30, 40};
      struct stride str          = {.start = 0, .stride = 1, .nelems = 4};
      ns_ref_write (ref, write_buf, str);

      u32 read_buf[4] = {0};
      ns_ref_read (ref, read_buf, str);
      for (int i = 0; i < 4; i++) {
        test_assert_int_equal (write_buf[i], read_buf[i]);
      }

      // insert 2 more elements right after (offset = 4 elements in, len = 2 elements)
      u32   insert_buf[2] = {50, 60};
      err_t rc            = ns_ref_insert (ref, insert_buf, 4, 2, &e);
      (void)rc; // ignoring error return values

      struct stride insert_str         = {.start = 4, .stride = 1, .nelems = 2};
      u32           insert_read_buf[2] = {0};
      ns_ref_read (ref, insert_read_buf, insert_str);
      for (int i = 0; i < 2; i++) {
        test_assert_int_equal (insert_buf[i], insert_read_buf[i]);
      }

      // strided write/read over the first 8 elements, touching every other one
      u32           stride_write_buf[4] = {100, 200, 300, 400};
      struct stride stride_str          = {.start = 0, .stride = 2, .nelems = 4};
      ns_ref_write (ref, stride_write_buf, stride_str);

      u32 stride_read_buf[4] = {0};
      ns_ref_read (ref, stride_read_buf, stride_str);
      for (int i = 0; i < 4; i++) {
        test_assert_int_equal (stride_write_buf[i], stride_read_buf[i]);
      }

      // remove the originally inserted elements
      u32 remove_dest[2] = {0};
      ns_ref_remove (ref, remove_dest, insert_str);
      for (int i = 0; i < 2; i++) {
        test_assert_int_equal (insert_buf[i], remove_dest[i]);
      }
    }
    ns_ref_commit_txn (ref, &e);

    ns_ref_delete (ref, "var2");
  }

  TEST_CASE ("rollback restores prior state, including current variable")
  {
    struct stride str              = {.start = 0, .stride = 1, .nelems = 4};

    // still on test_var with the committed baseline data
    u32           baseline_read[4] = {0};
    ns_ref_read (ref, baseline_read, str);
    u32 expected_baseline[4] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (expected_baseline[i], baseline_read[i]);
    }

    ns_ref_begin_txn (ref, &e);
    {
      ns_ref_switch (ref, "var3");
      u32 var3_write[4] = {111, 222, 333, 444};
      ns_ref_write (ref, var3_write, str);

      u32 var3_read[4] = {0};
      ns_ref_read (ref, var3_read, str);
      for (int i = 0; i < 4; i++) {
        test_assert_int_equal (var3_write[i], var3_read[i]);
      }
    }
    ns_ref_rollback_txn (ref);

    // Still test_var - not var3
    u32 post_rollback_read[4] = {0};
    ns_ref_read (ref, post_rollback_read, str);
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (expected_baseline[i], post_rollback_read[i]);
    }

    // var3 itself should not have the rolled-back write either
    ns_ref_switch (ref, "var3");
    u32 var3_post_rollback[4] = {0};
    ns_ref_read (ref, var3_post_rollback, str);
    u32 expected_var3_untouched[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (expected_var3_untouched[i], var3_post_rollback[i]);
    }
  }

  TEST_CASE ("commit persists the switch and the write")
  {
    struct stride str           = {.start = 0, .stride = 1, .nelems = 4};
    u32           var3_write[4] = {111, 222, 333, 444};

    ns_ref_begin_txn (ref, &e);
    {
      ns_ref_switch (ref, "var3");
      ns_ref_write (ref, var3_write, str);
    }
    ns_ref_commit_txn (ref, &e);

    // still on var3 after commit, with the committed data
    u32 post_commit_read[4] = {0};
    ns_ref_read (ref, post_commit_read, str);
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (var3_write[i], post_commit_read[i]);
    }
  }
}

#endif
*/
