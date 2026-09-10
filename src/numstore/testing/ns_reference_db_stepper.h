#ifndef NS_REFERENCE_DB_STEPPER
#define NS_REFERENCE_DB_STEPPER

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/types/ns_types.h"

struct db_state
{
  struct mem_vhmap     *db_data;
  struct var_with_data *cur;
  u32                   nvars;
  u32                   tracked_bytes;
  struct i_mem          mem;
};

struct ns_ref
{
  struct db_state *committed;
  struct db_state *working;
  struct i_mem     mem;
  bool             in_txn;
};

struct ns_ref *ns_ref_new (struct i_mem, error *e);
void ns_ref_free (struct ns_ref *ref);

// Utils
u32 ns_ref_nvars (struct ns_ref *ref);
const char *ns_ref_cur_name (struct ns_ref *ref);
b_size ns_ref_cur_len (struct ns_ref *ref);
t_size ns_ref_cur_tsize (struct ns_ref *ref);
bool ns_ref_var_exists (struct ns_ref *ref, const char *name);
const char *ns_ref_random_var (struct ns_ref *ref);

// Main functions
err_t ns_ref_begin_txn (struct ns_ref *ref, error *e);
void ns_ref_rollback_txn (struct ns_ref *ref);
err_t ns_ref_commit_txn (struct ns_ref *ref, error *e);
void ns_ref_crash_and_reopen (struct ns_ref *ref);
void ns_ref_close_and_reopen (struct ns_ref *ref);
err_t ns_ref_create (struct ns_ref *ref, const char *vname, struct type *type, error *e);
void ns_ref_switch (struct ns_ref *ref, const char *next);
void ns_ref_delete (struct ns_ref *ref, const char *next);
err_t ns_ref_insert (struct ns_ref *ref, void *data, b_size ofst, b_size len, error *e);
void ns_ref_remove (struct ns_ref *ref, void *dest, struct stride str);
void ns_ref_read (struct ns_ref *ref, void *dest, struct stride str);
void ns_ref_write (struct ns_ref *ref, void *data, struct stride str);

#endif
