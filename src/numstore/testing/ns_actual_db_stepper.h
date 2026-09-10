#ifndef NS_ACTUAL_DB_STEPPER
#define NS_ACTUAL_DB_STEPPER

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_time.h"

struct ns_db
{
  struct nsdb   *db;
  struct ns_txn *tx;

  char          *var_committed; // Current variable we act on
  char          *var_working;   // For rollback

  struct i_mem   reliable_mem;

  // Metrics
  struct i_timer timer;
  u64            total_working_ns;
  u64            prev_op_duration_ns;
  u64            db_size_bytes;

  enum prev_operation
  {
    PO_NONE,
    PO_SUCCESS,
    PO_FAILED_OTHER_REASONS,
    PO_DB_FAILED,
  } op_status;
};

struct ns_db *ns_db_new (
    struct i_mem         reliable_mem,
    struct i_mem         test_mem,
    struct i_file_system fs,
    const char          *dbname,
    error               *e
);
err_t ns_db_close (struct ns_db *db);

void ns_db_begin_txn (struct ns_db *db);
void ns_db_rollback_txn (struct ns_db *db);
void ns_db_commit_txn (struct ns_db *db);
void ns_db_crash_and_reopen (struct ns_db *db);
void ns_db_close_and_reopen (struct ns_db *db);
void ns_db_create (struct ns_db *db, const char *vname, const char *type_str);
void ns_db_switch (struct ns_db *db, const char *next);
void ns_db_delete (struct ns_db *db, const char *next);
void ns_db_insert (struct ns_db *db, void *data, b_size ofst, b_size len);
void ns_db_remove (struct ns_db *db, void *dest, struct stride str);
void ns_db_read (struct ns_db *db, void *dest, struct stride str);
void ns_db_write (struct ns_db *db, void *data, struct stride str);

#endif
