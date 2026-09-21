#ifndef NS_ACTUAL_DB_STEPPER
#define NS_ACTUAL_DB_STEPPER

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_time.h"
#include "nscore/types/ns_types.h"

/**
 * A database stepper is a little state machine that
 * holds onto a variable and operates a bunch of operations
 * on that variable
 */
struct ns_db
{
  struct nsdb         *db; // The system under test
  struct ns_txn       *tx; // The active transaction

  char                *var_committed; // Current variable we act on
  char                *var_working;   // For rollback

  struct i_mem         reliable_mem; // Memory used that isn't supposed to fail

  // Metrics
  struct i_timer       timer;               // timer used to record metrics
  u64                  total_working_ns;    // Total time spent working
  u64                  prev_op_duration_ns; // Time it took to execute the previous operation
  u64                  db_size_bytes;       // Total size of the database in bytes

  // Open parameters
  const char          *dbname;   // Name of the database
  struct i_mem         test_mem; // Memory used in test - can fail
  struct i_file_system test_fs;  // File system used in test - can fail
};

// Create a new simulation
struct ns_db *ns_db_new (
    struct i_mem         reliable_mem,
    struct i_mem         test_mem,
    struct i_file_system fs,
    const char          *dbname,
    error               *e
);
err_t ns_db_close (struct ns_db *db, error *e);

// State Machine Actions
err_t ns_db_begin_txn (struct ns_db *db, error *e);
err_t ns_db_rollback_txn (struct ns_db *db, error *e);
err_t ns_db_commit_txn (struct ns_db *db, error *e);
err_t ns_db_crash_and_reopen (struct ns_db *db, error *e);
err_t ns_db_close_and_reopen (struct ns_db *db, error *e);
err_t ns_db_create (struct ns_db *db, const char *vname, struct type dtype, error *e);
err_t ns_db_switch (struct ns_db *db, const char *next, error *e);
err_t ns_db_delete_and_switch (struct ns_db *db, const char *next, error *e);
sb_size ns_db_insert (struct ns_db *db, void *data, b_size ofst, b_size len, error *e);
sb_size ns_db_remove (struct ns_db *db, void *dest, struct stride str, error *e);
sb_size ns_db_read (struct ns_db *db, void *dest, struct stride str, error *e);
sb_size ns_db_write (struct ns_db *db, void *data, struct stride str, error *e);

#endif
