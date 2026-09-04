#include "numstore/testing/ns_actual_db_stepper.h"

#include "core/ns_error.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_os_vtable.h"
#include "core/os/ns_time.h"
#include "core/testing/ns_testing.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "numstore/numstore.h"

#include <string.h>

err_t
ns_db_set_file_size (struct ns_db *db, error *e)
{
  // TODO - change this
  i64 ret = impl_file_size (db->db->p->fp, e);
  if (ret < 0) {
    return error_trace (e);
  }

  db->db_size_bytes = ret;

  return SUCCESS;
}

struct ns_db *
ns_db_new (
    struct i_mem         reliable_mem,
    struct i_mem         test_mem,
    struct i_file_system fs,
    const char          *dbname,
    error               *e
)
{
  struct ns_db *ret = i_malloc (reliable_mem, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  if (i_timer_create (&ret->timer, e) < 0) {
    i_free (reliable_mem, ret);
    return NULL;
  }

  struct nsdb *db = nsdb_open_with_resources (dbname, test_mem, fs);
  if (db == NULL) {
    i_timer_free (&ret->timer);
    i_free (reliable_mem, ret);
    return NULL;
  }

  *ret = (struct ns_db){
      .db                  = db,
      .tx                  = NULL,
      .var_committed       = NULL,
      .var_working         = NULL,
      .reliable_mem        = reliable_mem,
      // .timer ,
      .total_working_ns    = 0,
      .prev_op_duration_ns = 0,
      .db_size_bytes       = 0,
      .op_status           = PO_NONE,
  };

  if (ns_db_set_file_size (ret, e)) {
    nsdb_close (db);
    i_timer_free (&ret->timer);
    i_free (reliable_mem, ret);
    return NULL;
  }

  return ret;
}

err_t
ns_db_close (struct ns_db *db)
{
  ASSERT (db->tx == NULL);
  ASSERT (db->var_working == NULL);

  i_cfree (db->reliable_mem, db->var_committed);

  err_t ret = nsdb_close (db->db);
  i_free (db->reliable_mem, db);

  return ret;
}

#define pre_op(db) u64 now = i_timer_now_ns (&db->timer)

#define post_op(db)                                              \
  do {                                                           \
    db->prev_op_duration_ns = i_timer_now_ns (&db->timer) - now; \
    db->total_working_ns += db->prev_op_duration_ns;             \
    db->op_status = PO_SUCCESS;                                  \
  }                                                              \
  while (0)

static inline void
end_method (struct ns_db *db)
{
  // Set the database file size after the operation
  if (ns_db_set_file_size (db, &db->db->e) < 0) {
    // DB_FAILED takes precedence over other operation failing
    if (db->op_status != PO_DB_FAILED) {
      db->op_status = PO_FAILED_OTHER_REASONS;
    }
  }
}

void
ns_db_begin_txn (struct ns_db *db)
{
  ASSERT (db);
  ASSERT (db->tx == NULL);
  ASSERT (db->var_working == NULL);

  // Copy over a committed variable to working variable
  char *var_working = NULL;
  if (db->var_committed) {
    var_working = i_malloc (db->reliable_mem, strlen (db->var_committed), 1, &db->db->e);
    if (var_working == NULL) {
      db->op_status = PO_FAILED_OTHER_REASONS;
      goto theend;
    }
    memcpy (var_working, db->var_committed, strlen (db->var_committed));
  }

  // Do the operation
  pre_op (db);
  struct ns_txn *tx = nsdb_begin (db->db);
  post_op (db);

  if (tx == NULL) {
    i_cfree (db->reliable_mem, var_working);
    db->op_status = PO_DB_FAILED;

  } else {
    db->tx          = tx;
    db->var_working = var_working;
  }

theend:
  end_method (db);
}

void
ns_db_rollback_txn (struct ns_db *db)
{
  ASSERT (db);
  ASSERT (db->tx);

  // Do the operation
  pre_op (db);
  err_t ret = nsdb_rollback (db->db, db->tx);
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;

  } else {
    i_cfree (db->reliable_mem, db->var_working);
    db->tx          = NULL;
    db->var_working = NULL;
  }

  end_method (db);
}

void
ns_db_commit_txn (struct ns_db *db)
{
  ASSERT (db);
  ASSERT (db->tx);

  // Copy over the working variable name
  char *new_committed = NULL;
  if (db->var_working) {
    new_committed = i_malloc (db->reliable_mem, strlen (db->var_working), 1, &db->db->e);
    if (new_committed == NULL) {
      db->op_status = PO_FAILED_OTHER_REASONS;
      goto theend;
    }
    memcpy (new_committed, db->var_working, strlen (db->var_working));
  }

  // Do the operation
  pre_op (db);
  err_t ret = nsdb_commit (db->db, db->tx);
  post_op (db);

  if (ret < 0) {
    i_cfree (db->reliable_mem, new_committed);
    db->op_status = PO_DB_FAILED;

  } else {
    // Transfer state
    i_cfree (db->reliable_mem, db->var_working);
    i_cfree (db->reliable_mem, db->var_committed);
    db->var_committed = new_committed;
    db->tx            = NULL;
    db->var_working   = NULL;
  }

theend:
  end_method (db);
}

void
ns_db_crash_and_reopen (struct ns_db *db)
{
  pre_op (db);
  err_t ret = nsdb_crash (db->db);
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;

  } else {
    i_cfree (db->reliable_mem, db->var_working);
    db->tx          = NULL;
    db->var_working = NULL;
  }

  end_method (db);
}

void
ns_db_close_and_reopen (struct ns_db *db)
{
  ASSERT (db->tx == NULL);
  ASSERT (db->var_working == NULL);

  pre_op (db);
  err_t ret = nsdb_close (db->db);
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;
  }

  end_method (db);
}

static inline char *
ns_db_cur (struct ns_db *db)
{
  if (db->tx) {
    return db->var_working;
  } else {
    return db->var_committed;
  }
}

static inline void
ns_db_set_cur (struct ns_db *db, char *vname)
{
  // Copy it to either destination
  if (db->tx) {
    i_cfree (db->reliable_mem, db->var_working);
    db->var_working = vname;
  } else {
    i_cfree (db->reliable_mem, db->var_committed);
    db->var_committed = vname;
  }
}

static inline err_t
ns_db_copy_cur (struct ns_db *db, const char *vname)
{
  // Copy the variable to a new location
  char *copy = i_malloc (db->reliable_mem, strlen (vname), 1, &db->db->e);
  if (copy == NULL) {
    return error_trace (&db->db->e);
  }
  memcpy (copy, vname, strlen (vname));

  ns_db_set_cur (db, copy);

  return SUCCESS;
}

void
ns_db_create (struct ns_db *db, const char *vname, const char *type_str)
{
  pre_op (db);
  sb_size ret = nsdb_fexecute (db->db, db->tx, "create %s %s", NULL, vname, type_str);
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;

  } else {
    if (ns_db_cur (db) == NULL) {
      if (ns_db_copy_cur (db, vname) < 0) {
        db->op_status = PO_FAILED_OTHER_REASONS;
      }
    }
  }

  end_method (db);
}

void
ns_db_switch (struct ns_db *db, const char *next)
{
  pre_op (db);
  // Do nothing
  post_op (db);

  if (ns_db_copy_cur (db, next) < 0) {
    db->op_status = PO_FAILED_OTHER_REASONS;
  }

  end_method (db);
}

void
ns_db_delete (struct ns_db *db, const char *next)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Copy next to a new destination
  char *copy = i_malloc (db->reliable_mem, strlen (next), 1, &db->db->e);
  if (copy == NULL) {
    db->op_status = PO_FAILED_OTHER_REASONS;
    goto theend;
  }
  memcpy (copy, next, strlen (next));

  // Do the operation
  pre_op (db);
  sb_size ret = nsdb_fexecute (db->db, db->tx, "delete %s", NULL, cur);
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;
    i_free (db->reliable_mem, copy);

  } else {
    ns_db_set_cur (db, copy);
  }

theend:
  end_method (db);
}

void
ns_db_insert (struct ns_db *db, void *data, b_size ofst, b_size len)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_fexecute (
      db->db,
      db->tx,
      "insert %s %" PRb_size " %" PRb_size,
      data,
      cur,
      ofst,
      len
  );
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;
  }

  end_method (db);
}

void
ns_db_remove (struct ns_db *db, void *dest, struct stride str)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_fexecute (
      db->db,
      db->tx,
      "remove %s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
      dest,
      cur,
      str.start,
      str.start + str.nelems * str.stride,
      str.stride
  );
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;
  }

  end_method (db);
}

void
ns_db_read (struct ns_db *db, void *dest, struct stride str)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_fexecute (
      db->db,
      db->tx,
      "read %s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
      dest,
      cur,
      str.start,
      str.start + str.nelems * str.stride,
      str.stride
  );
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;
  }

  end_method (db);
}

void
ns_db_write (struct ns_db *db, void *data, struct stride str)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_fexecute (
      db->db,
      db->tx,
      "write %s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
      data,
      cur,
      str.start,
      str.start + str.nelems * str.stride,
      str.stride
  );
  post_op (db);

  if (ret < 0) {
    db->op_status = PO_DB_FAILED;
  }

  end_method (db);
}

/**
#ifdef TESTING

TEST_DISABLED (ns_db)
{
  error         e  = error_create ();
  struct ns_db *db = ns_db_new (mem, mem, fs, "test_db", &e);

  TEST_CASE ("create, switch, write, read, insert, remove, delete")
  {
    // create at least 3 vars
    // (create only auto-switches if cur is NULL, i.e. on the very first create)
    ns_db_create (db, "test_var", "u32"); // auto-switches here, cur was NULL
    ns_db_create (db, "var2", "u32");     // cur is now test_var, no auto-switch
    ns_db_create (db, "var3", "u32");     // cur still test_var, no auto-switch

    // already on test_var due to the first create's auto-switch
    ns_db_begin_txn (db);

    // write 4 contiguous u32 elements
    u32           write_buf[4] = {10, 20, 30, 40};
    struct stride str          = {.start = 0, .stride = 1, .nelems = 4};
    ns_db_write (db, write_buf, str);

    u32 read_buf[4] = {0};
    ns_db_read (db, read_buf, str);
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (write_buf[i], read_buf[i]);
    }

    // insert 2 more elements right after (offset = 4 elements in, len = 2 elements)
    u32 insert_buf[2] = {50, 60};
    ns_db_insert (db, insert_buf, 4, 2);

    struct stride insert_str         = {.start = 4, .stride = 1, .nelems = 2};
    u32           insert_read_buf[2] = {0};
    ns_db_read (db, insert_read_buf, insert_str);
    for (int i = 0; i < 2; i++) {
      test_assert_int_equal (insert_buf[i], insert_read_buf[i]);
    }

    // strided write/read over the first 8 elements, touching every other one
    u32           stride_write_buf[4] = {100, 200, 300, 400};
    struct stride stride_str          = {.start = 0, .stride = 2, .nelems = 4};
    ns_db_write (db, stride_write_buf, stride_str);

    u32 stride_read_buf[4] = {0};
    ns_db_read (db, stride_read_buf, stride_str);
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (stride_write_buf[i], stride_read_buf[i]);
    }

    // remove the originally inserted elements
    u32 remove_dest[2] = {0};
    ns_db_remove (db, remove_dest, insert_str);
    for (int i = 0; i < 2; i++) {
      test_assert_int_equal (insert_buf[i], remove_dest[i]);
    }

    // commit this baseline txn so it's the durable state to roll back to
    ns_db_commit_txn (db);

    // delete one of the three vars
    ns_db_delete (db, "var2");
  }

  TEST_CASE ("rollback restores prior state, including current variable")
  {
    struct stride str              = {.start = 0, .stride = 1, .nelems = 4};

    // still on test_var with the committed baseline data
    u32           baseline_read[4] = {0};
    ns_db_read (db, baseline_read, str);
    u32 expected_baseline[4] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (expected_baseline[i], baseline_read[i]);
    }

    ns_db_begin_txn (db);
    {
      ns_db_switch (db, "var3");
      u32 var3_write[4] = {111, 222, 333, 444};
      ns_db_write (db, var3_write, str);

      u32 var3_read[4] = {0};
      ns_db_read (db, var3_read, str);
      for (int i = 0; i < 4; i++) {
        test_assert_int_equal (var3_write[i], var3_read[i]);
      }
    }
    ns_db_rollback_txn (db);

    // Still test_var - not var3
    u32 post_rollback_read[4] = {0};
    ns_db_read (db, post_rollback_read, str);
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (expected_baseline[i], post_rollback_read[i]);
    }

    // var3 itself should not have the rolled-back write either
    ns_db_switch (db, "var3");
    u32 var3_post_rollback[4] = {0};
    ns_db_read (db, var3_post_rollback, str);
    u32 expected_var3_untouched[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (expected_var3_untouched[i], var3_post_rollback[i]);
    }
  }

  TEST_CASE ("commit persists the switch and the write")
  {
    struct stride str = {.start = 0, .stride = 1, .nelems = 4};

    ns_db_begin_txn (db);

    // switch to var3 and write different data
    ns_db_switch (db, "var3");
    u32 var3_write[4] = {111, 222, 333, 444};
    ns_db_write (db, var3_write, str);

    ns_db_commit_txn (db);

    // still on var3 after commit, with the committed data
    u32 post_commit_read[4] = {0};
    ns_db_read (db, post_commit_read, str);
    for (int i = 0; i < 4; i++) {
      test_assert_int_equal (var3_write[i], post_commit_read[i]);
    }
  }

  ns_db_close (db);
}

#endif
*/
