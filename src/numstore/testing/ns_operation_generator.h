#ifndef NS_OPERATION_GENERATOR
#define NS_OPERATION_GENERATOR

#include "nscore/types/ns_types.h"
#include "numstore/testing/ns_reference_db_stepper.h"

enum ns_action_type
{
  NSS_BEGIN_TXN,
  NSS_COMMIT_TXN,
  NSS_ROLLBACK_TXN,

  NSS_CRASH_AND_REOPEN,
  NSS_CLOSE_AND_REOPEN,

  NSS_CREATE, // Create a new variable (don't swap unless it's the first one)
  NSS_SWITCH, // Swap to an existing variable
  NSS_DELETE, // Delete current variable and swap to a different one

  NSS_INSERT, // Insert data into this one
  NSS_REMOVE, // remove data from this one
  NSS_READ,   // Read data from this one
  NSS_WRITE,  // Write data to this one

  NSS_AT_LEN,

  NSS_NONE_AVAILABLE,
};

struct operation
{
  enum ns_action_type type;

  union {
    struct
    {
      char        *vname;
      struct type *t;
      char        *typestr;
    } op_create;

    struct
    {
      char *vname;
    } op_switch;

    struct
    {
      char *next;
    } op_delete;

    struct
    {
      b_size ofst;
      b_size nelems;
      u8    *data;
    } op_insert;

    struct
    {
      b_size start;
      b_size stride;
      b_size nelems;
      u8    *db_dest;
      u8    *ref_dest;
    } op_remove;

    struct
    {
      b_size start;
      b_size stride;
      b_size nelems;
      u8    *db_dest;
      u8    *ref_dest;
    } op_read;

    struct
    {
      b_size start;
      b_size stride;
      b_size nelems;
      u8    *data;
    } op_write;
  };

  struct allocator alloc;
  struct i_mem     mem;
};

struct rand_op_params
{
  // Used for selecting random variables from the database
  struct ns_ref *ref;

  // Operations that are enabled
  u8             enabled[NSS_AT_LEN];

  // Maximum insert length
  b_size         max_nelems;
  struct i_mem   mem;
};

// Spin a random
void opg_spin_enabled (u8 enabled[NSS_AT_LEN]);
struct operation *opg_random (struct rand_op_params params, error *e);
void opg_free (struct operation *op);

#endif
