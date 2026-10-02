#ifndef NS_OPERATION_GENERATOR
#define NS_OPERATION_GENERATOR

#include "core/ns_stride.h"
#include "nscore/types/ns_types.h"

enum ns_action_type
{
  NSS_BEGIN_TXN,
  NSS_COMMIT_TXN,
  NSS_ROLLBACK_TXN,

  NSS_CRASH_AND_REOPEN,
  NSS_CLOSE_AND_REOPEN,

  NSS_CREATE_AND_SWAP_IF_EMPTY,           // Create a new variable (don't swap unless it's
                                          // the first one)
  NSS_SWITCH,                             // Swap to an existing variable
  NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH, // Delete current variable and swap to
                                          // a different one

  NSS_INSERT, // Insert data into this one
  NSS_REMOVE, // remove data from this one
  NSS_READ,   // Read data from this one
  NSS_WRITE,  // Write data to this one

  NSS_AT_LEN,

  NSS_NONE_AVAILABLE,
};

/// Indexed by enum value, so the order can't drift from the enum.
/// Keep in sync when adding actions.
static const char *const action_names[NSS_AT_LEN] = {
    [NSS_BEGIN_TXN]                          = "BEGIN_TXN",
    [NSS_COMMIT_TXN]                         = "COMMIT_TXN",
    [NSS_ROLLBACK_TXN]                       = "ROLLBACK_TXN",
    [NSS_CRASH_AND_REOPEN]                   = "CRASH_AND_REOPEN",
    [NSS_CLOSE_AND_REOPEN]                   = "CLOSE_AND_REOPEN",
    [NSS_CREATE_AND_SWAP_IF_EMPTY]           = "CREATE_AND_SWAP_IF_EMPTY",
    [NSS_SWITCH]                             = "SWITCH",
    [NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH] = "DELETE_CURRENT_VARIABLE_AND_SWITCH",
    [NSS_INSERT]                             = "INSERT",
    [NSS_REMOVE]                             = "REMOVE",
    [NSS_READ]                               = "READ",
    [NSS_WRITE]                              = "WRITE",
};

struct operation
{
  enum ns_action_type type;

  // Per action parameters
  union {
    struct
    {
      char        *vname;
      struct type *t;
      char        *typestr;
    } op_create;

    struct
    {
      char *next;
    } op_switch;

    struct
    {
      char *next;
    } op_delete;

    struct
    {
      b_size ofst;
      b_size nelems;
    } op_insert;

    struct
    {
      struct stride str;
    } op_remove;

    struct
    {
      struct stride str;
    } op_read;

    struct
    {
      struct stride str;
    } op_write;
  };

  // Buffers shared by every action type. NULL / 0 when the action doesn't
  // need them.
  //
  //   data     - source bytes for INSERT / WRITE
  //   db_buf   - destination for reading back from the database
  //   ref_buf  - destination for reading back from the reference model
  //
  // db_buf and ref_buf are always the same size (buf_size).
  u8                *data;
  b_size             data_size;

  u8                *db_buf;
  u8                *ref_buf;
  b_size             buf_size;

  struct arena_alloc alloc;
  struct i_mem       mem;
};

struct rand_op_params
{
  // Used for selecting random variables from the database
  struct ns_ref *ref;

  // Operations that are enabled
  u8            *enabled;

  // Maximum insert length
  b_size         max_nelems;
  t_size         max_tsize;
  struct i_mem   mem;
};

// Spin a random
void opg_spin_enabled (u8 enabled[NSS_AT_LEN]);
struct operation *opg_random (struct rand_op_params params, error *e);
void opg_free (struct operation *op);

#define OP_CREATE(_name, _type)                  \
  ((struct operation){                           \
      .type      = NSS_CREATE_AND_SWAP_IF_EMPTY, \
      .op_create = {                             \
          .vname = (char *)(_name),              \
          .t     = (_type),                      \
      },                                         \
  })

#define OP_SWITCH(_name)           \
  ((struct operation){             \
      .type      = NSS_SWITCH,     \
      .op_switch = {               \
          .next = (char *)(_name), \
      },                           \
  })

#define OP_DELETE(_next)                                   \
  ((struct operation){                                     \
      .type      = NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH, \
      .op_delete = {                                       \
          .next = (char *)(_next),                         \
      },                                                   \
  })

#define OP_INSERT(_data, _dlen, _ofst, _n) \
  ((struct operation){                     \
      .type = NSS_INSERT,                  \
      .op_insert =                         \
          {                                \
              .ofst   = (_ofst),           \
              .nelems = (_n),              \
          },                               \
      .data      = (_data),                \
      .data_size = (_dlen),                \
  })

#define OP_WRITE(_data, _dlen, _start, _stride, _nelems) \
  ((struct operation){                                   \
      .type = NSS_WRITE,                                 \
      .op_write =                                        \
          {                                              \
              .str =                                     \
                  (struct stride){                       \
                      .start  = (_start),                \
                      .stride = (_stride),               \
                      .nelems = (_nelems),               \
                  },                                     \
          },                                             \
      .data      = (_data),                              \
      .data_size = (_dlen),                              \
  })

#define OP_READ(_db_buf, _ref_buf, _dlen, _start, _stride, _nelems) \
  ((struct operation){                                              \
      .type = NSS_READ,                                             \
      .op_read =                                                    \
          {                                                         \
              .str =                                                \
                  (struct stride){                                  \
                      .start  = (_start),                           \
                      .stride = (_stride),                          \
                      .nelems = (_nelems),                          \
                  },                                                \
          },                                                        \
      .ref_buf  = (_ref_buf),                                       \
      .db_buf   = (_db_buf),                                        \
      .buf_size = (_dlen),                                          \
  })

#define OP_REMOVE(_db_buf, _ref_buf, _dlen, _start, _stride, _nelems) \
  ((struct operation){                                                \
      .type = NSS_REMOVE,                                             \
      .op_remove =                                                    \
          {                                                           \
              .str =                                                  \
                  (struct stride){                                    \
                      .start  = (_start),                             \
                      .stride = (_stride),                            \
                      .nelems = (_nelems),                            \
                  },                                                  \
          },                                                          \
      .ref_buf  = (_ref_buf),                                         \
      .db_buf   = (_db_buf),                                          \
      .buf_size = (_dlen),                                            \
  })

#endif
