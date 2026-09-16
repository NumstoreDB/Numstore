#ifndef NS_ACTUAL_DB_STEPPER
#define NS_ACTUAL_DB_STEPPER

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_time.h"
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"

/**
 * A database stepper is a little state machine that
 * holds onto a variable and operates a bunch of operations
 * on that variable
 */
struct ns_db
{
  struct nsdb         *db;
  struct ns_txn       *tx;

  char                *var_committed; // Current variable we act on
  char                *var_working;   // For rollback

  struct i_mem         reliable_mem;

  // Metrics
  struct i_timer       timer;
  u64                  total_working_ns;
  u64                  prev_op_duration_ns;
  u64                  db_size_bytes;

  // Open parameters
  const char          *dbname;
  struct i_mem         test_mem;
  struct i_file_system test_fs;
};

struct ns_db *ns_db_new (
    struct i_mem reliable_mem, // Memory used for allocating internal data structures etc
    struct i_mem
        test_mem, // Memory used for allocating data within test (Can be intentionally faulty)
    struct i_file_system fs, // File system used for database
    const char          *dbname,
    error               *e
);
err_t ns_db_close (struct ns_db *db, error *e);

err_t ns_db_begin_txn (struct ns_db *db, error *e);
err_t ns_db_rollback_txn (struct ns_db *db, error *e);
err_t ns_db_commit_txn (struct ns_db *db, error *e);
err_t ns_db_crash_and_reopen (struct ns_db *db, error *e);
err_t ns_db_close_and_reopen (struct ns_db *db, error *e);
err_t ns_db_create_and_maybe_switch (
    struct ns_db *db,
    const char   *vname,
    struct type   dtype,
    error        *e
);
err_t ns_db_switch (struct ns_db *db, const char *next, error *e);
err_t ns_db_delete_cur_and_switch (struct ns_db *db, const char *next, error *e);
err_t ns_db_insert (struct ns_db *db, void *data, b_size ofst, b_size len, error *e);
err_t ns_db_remove (struct ns_db *db, void *dest, struct stride str, error *e);
err_t ns_db_read (struct ns_db *db, void *dest, struct stride str, error *e);
err_t ns_db_write (struct ns_db *db, void *data, struct stride str, error *e);

#endif
