#include "nscore/testing/simulation/ns_db_state_machine.h"

#include "core/ns_error.h"
#include "core/ns_stride.h"
#include "core/ns_testing.h"
#include "nscore/disk_pager/ns_file_pager.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "os/ns_memory.h"
#include "os/ns_time.h"

#include <string.h>

err_t
ns_db_set_file_size (struct ns_db *db, error *e)
{
  (void)e;

  // p->fp is a `struct file_pager *`, not an `i_file *` - going through
  // impl_file_size() reinterpreted the wrong type via void* and always
  // reported 0. fpgr_get_npages() is the public accessor for this.
  db->db_size_bytes = (u64)fpgr_get_npages (db->db->p->fp) * NS_PAGE_SIZE;

  return SUCCESS;
}

static err_t
ns_db_reopen_handle (struct ns_db *db, error *e)
{
  struct nsdb *ns = nsdb_open (db->dbname, db->test_mem, db->test_fs, e);
  if (ns == NULL) {
    return error_trace (e);
  }

  if (nsdb_writeit_numstore (ns, e)) {
    nsdb_close (ns, e);
    return error_trace (e);
  }

  // Operations outside a transaction (db->tx == NULL) run in an auto txn
  nsdb_allow_auto_txn (ns);

  db->db = ns;
  return SUCCESS;
}

struct ns_db *
ns_db_new (
    struct i_mem         reliable_mem,
    struct i_mem         test_mem,
    struct i_file_system test_fs,
    const char          *dbname,
    error               *e
)
{
  struct ns_db *ret = i_malloc (reliable_mem, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  *ret = (struct ns_db){
      .db                  = NULL,
      .tx                  = NULL,
      .var_committed       = NULL,
      .var_working         = NULL,
      .reliable_mem        = reliable_mem,
      .total_working_ns    = 0,
      .prev_op_duration_ns = 0,
      .db_size_bytes       = 0,

      .test_mem            = test_mem,
      .test_fs             = test_fs,
      .dbname              = dbname,
  };

  // After the struct assignment above, otherwise it wipes the timer
  if (i_timer_create (&ret->timer, e) < 0) {
    i_free (reliable_mem, ret);
    return NULL;
  }

  if (ns_db_reopen_handle (ret, e)) {
    i_timer_free (&ret->timer);
    i_free (reliable_mem, ret);
    return NULL;
  }

  if (ns_db_set_file_size (ret, e)) {
    nsdb_close (ret->db, e);
    i_timer_free (&ret->timer);
    i_free (reliable_mem, ret);
    return NULL;
  }

  return ret;
}

err_t
ns_db_close (struct ns_db *db, error *e)
{
  ASSERT (db->tx == NULL);
  ASSERT (db->var_working == NULL);

  i_cfree (db->reliable_mem, db->var_committed);

  err_t ret = nsdb_close (db->db, e);
  i_timer_free (&db->timer);
  i_free (db->reliable_mem, db);

  return ret;
}

#define pre_op(db) u64 now = i_timer_now_ns (&db->timer)

#define post_op(db)                                              \
  do {                                                           \
    db->prev_op_duration_ns = i_timer_now_ns (&db->timer) - now; \
    db->total_working_ns += db->prev_op_duration_ns;             \
  }                                                              \
  while (0)

// Returns a reliable_mem copy of src, or NULL if src is NULL.
// Check (ret == NULL && src != NULL) for allocation failure.
static inline char *
ns_db_copy_name (struct ns_db *db, const char *src, error *e)
{
  if (src == NULL) {
    return NULL;
  }

  // +1 so the copy stays NUL terminated - it's handed to strfcstr(), which
  // calls strlen() on it.
  size_t len  = strlen (src) + 1;
  char  *copy = i_malloc (db->reliable_mem, len, 1, e);
  if (copy == NULL) {
    return NULL;
  }
  memcpy (copy, src, len);

  return copy;
}

err_t
ns_db_begin_txn (struct ns_db *db, error *e)
{
  ASSERT (db);
  ASSERT (db->tx == NULL);
  ASSERT (db->var_working == NULL);

  char *var_working = ns_db_copy_name (db, db->var_committed, e);
  if (var_working == NULL && db->var_committed != NULL) {
    return error_trace (e);
  }

  // Do the operation
  pre_op (db);
  struct txn *tx = nsdb_begin (db->db, e);
  post_op (db);

  if (tx == NULL) {
    i_cfree (db->reliable_mem, var_working);
    return error_trace (e);
  }

  db->tx          = tx;
  db->var_working = var_working;
  return ns_db_set_file_size (db, e);
}

err_t
ns_db_rollback_txn (struct ns_db *db, error *e)
{
  ASSERT (db);
  ASSERT (db->tx);

  // Do the operation
  pre_op (db);
  err_t ret = nsdb_rollback (db->db, db->tx, e);
  post_op (db);

  if (ret < 0) {
    return error_trace (e);
  }

  i_cfree (db->reliable_mem, db->var_working);
  db->tx          = NULL;
  db->var_working = NULL;
  return ns_db_set_file_size (db, e);
}

err_t
ns_db_commit_txn (struct ns_db *db, error *e)
{
  ASSERT (db);
  ASSERT (db->tx);

  char *new_committed = ns_db_copy_name (db, db->var_working, e);
  if (new_committed == NULL && db->var_working != NULL) {
    return error_trace (e);
  }

  // Do the operation
  pre_op (db);
  err_t ret = nsdb_commit (db->db, db->tx, e);
  post_op (db);

  if (ret < 0) {
    i_cfree (db->reliable_mem, new_committed);
    return error_trace (e);
  }

  // Transfer state
  i_cfree (db->reliable_mem, db->var_working);
  i_cfree (db->reliable_mem, db->var_committed);
  db->var_committed = new_committed;
  db->tx            = NULL;
  db->var_working   = NULL;
  return ns_db_set_file_size (db, e);
}

err_t
ns_db_crash_and_reopen (struct ns_db *db, error *e)
{
  pre_op (db);

  // Crash the database
  err_t ret = nsdb_crash (db->db, e);
  db->db    = NULL;

  // Re open
  if (ret == SUCCESS) {
    ret = ns_db_reopen_handle (db, e);
  }

  post_op (db);

  if (ret < 0) {
    return error_trace (e);
  }

  i_cfree (db->reliable_mem, db->var_working);
  db->tx          = NULL;
  db->var_working = NULL;
  return ns_db_set_file_size (db, e);
}

err_t
ns_db_close_and_reopen (struct ns_db *db, error *e)
{
  ASSERT (db->tx == NULL);
  ASSERT (db->var_working == NULL);

  pre_op (db);

  err_t ret = nsdb_close (db->db, e);
  db->db    = NULL;

  // Re open
  if (ret == SUCCESS) {
    ret = ns_db_reopen_handle (db, e);
  }

  post_op (db);

  if (ret < 0) {
    return error_trace (e);
  }

  return ns_db_set_file_size (db, e);
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

// Takes ownership of vname (may be NULL)
static inline void
ns_db_set_cur (struct ns_db *db, char *vname)
{
  if (db->tx) {
    i_cfree (db->reliable_mem, db->var_working);
    db->var_working = vname;
  } else {
    i_cfree (db->reliable_mem, db->var_committed);
    db->var_committed = vname;
  }
}

static inline err_t
ns_db_copy_cur (struct ns_db *db, const char *vname, error *e)
{
  ASSERT (vname);

  char *copy = ns_db_copy_name (db, vname, e);
  if (copy == NULL) {
    return error_trace (e);
  }

  ns_db_set_cur (db, copy);

  return SUCCESS;
}

err_t
ns_db_create (struct ns_db *db, const char *vname, const char *typestr, error *e)
{
  ASSERT (vname);
  ASSERT (typestr);

  // Construct the query
  struct nsdb_plan *plan = nsdb_plan_fcreate (db->db, "create %s %s", e, vname, typestr);
  if (plan == NULL) {
    return error_trace (e);
  }

  // Execute under a timer
  pre_op (db);
  err_t ret = nsdb_plan_execute (plan, db->tx, e);
  post_op (db);

  nsdb_plan_free (plan);

  if (ret < 0) {
    return error_trace (e);
  }

  // First variable becomes the current one
  if (ns_db_cur (db) == NULL) {
    WRAP (ns_db_copy_cur (db, vname, e));
  }

  return ns_db_set_file_size (db, e);
}

err_t
ns_db_switch (struct ns_db *db, const char *next, error *e)
{
  pre_op (db);
  // Do nothing
  post_op (db);

  WRAP (ns_db_copy_cur (db, next, e));

  return ns_db_set_file_size (db, e);
}

err_t
ns_db_delete_and_switch (struct ns_db *db, const char *next, error *e)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Construct the query
  struct nsdb_plan *plan = nsdb_plan_fcreate (db->db, "delete %s", e, cur);
  if (plan == NULL) {
    return error_trace (e);
  }

  // Copy next (NULL when deleting the last remaining variable - there's
  // nothing to switch to)
  char *copy = ns_db_copy_name (db, next, e);
  if (copy == NULL && next != NULL) {
    nsdb_plan_free (plan);
    return error_trace (e);
  }

  // Do the operation under a timer
  pre_op (db);
  err_t ret = nsdb_plan_execute (plan, db->tx, e);
  post_op (db);

  nsdb_plan_free (plan);

  if (ret < 0) {
    i_cfree (db->reliable_mem, copy);
    return error_trace (e);
  }

  ns_db_set_cur (db, copy);
  return ns_db_set_file_size (db, e);
}

sb_size
ns_db_insert (struct ns_db *db, const void *data, b_size dlen, b_size ofst, b_size nelems, error *e)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Construct the query
  struct nsdb_plan *plan = nsdb_plan_fcreate (
      db->db,
      "insert %s %llu %llu",
      e,
      cur,
      (unsigned long long)ofst,
      (unsigned long long)nelems
  );
  if (plan == NULL) {
    return error_trace (e);
  }

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_plan_write (plan, db->tx, data, dlen, e);
  post_op (db);

  nsdb_plan_free (plan);

  if (ret < 0) {
    return error_trace (e);
  }

  if (ns_db_set_file_size (db, e) < 0) {
    return error_trace (e);
  }

  return ret;
}

sb_size
ns_db_remove (struct ns_db *db, void *dest, b_size dlen, struct stride str, error *e)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Construct the query
  struct nsdb_plan
      *plan = nsdb_plan_fcreate (db->db, "remove %s[%d:%d:%d]", e, cur, stride_ustr_args (str));
  if (plan == NULL) {
    return error_trace (e);
  }

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_plan_read (plan, db->tx, dest, dlen, e);
  post_op (db);

  nsdb_plan_free (plan);

  if (ret < 0) {
    return error_trace (e);
  }

  if (ns_db_set_file_size (db, e) < 0) {
    return error_trace (e);
  }

  return ret;
}

sb_size
ns_db_read (struct ns_db *db, void *dest, b_size dlen, struct stride str, error *e)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Construct the query
  struct nsdb_plan
      *plan = nsdb_plan_fcreate (db->db, "read %s[%d:%d:%d]", e, cur, stride_ustr_args (str));
  if (plan == NULL) {
    return error_trace (e);
  }

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_plan_read (plan, db->tx, dest, dlen, e);
  post_op (db);

  nsdb_plan_free (plan);

  if (ret < 0) {
    return error_trace (e);
  }

  if (ns_db_set_file_size (db, e) < 0) {
    return error_trace (e);
  }

  return ret;
}

sb_size
ns_db_write (struct ns_db *db, const void *data, b_size dlen, struct stride str, error *e)
{
  char *cur = ns_db_cur (db);
  ASSERT (cur);

  // Construct the query
  struct nsdb_plan
      *plan = nsdb_plan_fcreate (db->db, "write %s[%d:%d:%d]", e, cur, stride_ustr_args (str));
  if (plan == NULL) {
    return error_trace (e);
  }

  // Do operation
  pre_op (db);
  sb_size ret = nsdb_plan_write (plan, db->tx, data, dlen, e);
  post_op (db);

  nsdb_plan_free (plan);

  if (ret < 0) {
    return error_trace (e);
  }

  if (ns_db_set_file_size (db, e) < 0) {
    return error_trace (e);
  }

  return ret;
}

#ifndef NDEBUG

TEST (ns_db)
{
  error e = error_create ();
  nsdb_cleanup ("./test_db.db", &e);
  struct ns_db *db = ns_db_new (mem, mem, fs, "./test_db.db", &e);
  test_assert (db != NULL);

  u32 dest[20];

#  define STR(_start, _stride, _nelems) \
    ((struct stride){.start = (_start), .stride = (_stride), .nelems = (_nelems)})

  // Reads the whole current variable and checks both the length and the
  // contents
#  define validate(expected)                                                 \
    do {                                                                     \
      sb_size _n = ns_db_read (db, dest, sizeof (dest), STR (0, 1, 20), &e); \
      test_assert_int_equal (_n, sizeof (expected) / sizeof (u32));          \
      test_assert_memequal (expected, dest, sizeof (expected));              \
    }                                                                        \
    while (0)

  // Checks which variable is current (works in and out of a transaction)
#  define check_cur(_name)                                       \
    do {                                                         \
      test_assert (ns_db_cur (db) != NULL);                      \
      test_assert_int_equal (strcmp (ns_db_cur (db), _name), 0); \
    }                                                            \
    while (0)

  TEST_CASE ("create, switch, write, read, insert, remove, delete")
  {
    test_assert (ns_db_cur (db) == NULL);

    // First create auto-switches (cur was NULL), the rest don't
    test_assert_int_equal (ns_db_create (db, "var1", "u32", &e), SUCCESS);
    check_cur ("var1");
    test_assert_int_equal (ns_db_create (db, "var2", "u32", &e), SUCCESS);
    check_cur ("var1");
    test_assert_int_equal (ns_db_create (db, "var3", "u32", &e), SUCCESS);
    check_cur ("var1");

    ns_db_begin_txn (db, &e);
    {
      u32 a[] = {10, 20, 30, 40};
      test_assert_int_equal (ns_db_insert (db, a, sizeof (a), 0, 4, &e), 4);
      validate (((u32[]){10, 20, 30, 40}));

      u32 b[] = {50, 60};
      test_assert_int_equal (ns_db_insert (db, b, sizeof (b), 4, 2, &e), 2);
      validate (((u32[]){10, 20, 30, 40, 50, 60}));

      u32 c[] = {70};
      test_assert_int_equal (ns_db_insert (db, c, sizeof (c), 6, 1, &e), 1);
      validate (((u32[]){10, 20, 30, 40, 50, 60, 70}));

      // 100 20 200 40 300 60 400
      u32 w[] = {100, 200, 300, 400};
      test_assert_int_equal (ns_db_write (db, w, sizeof (w), STR (0, 2, 4), &e), 4);
      validate (((u32[]){100, 20, 200, 40, 300, 60, 400}));

      // 20 40 60
      u32 removed[4] = {0};
      test_assert_int_equal (ns_db_remove (db, removed, sizeof (removed), STR (0, 2, 4), &e), 4);
      test_assert_memequal (((u32[]){100, 200, 300, 400}), removed, sizeof (removed));
      validate (((u32[]){20, 40, 60}));
    }
    test_assert_int_equal (ns_db_commit_txn (db, &e), SUCCESS);

    check_cur ("var1");
    validate (((u32[]){20, 40, 60}));
  }

  TEST_CASE ("rollback restores prior state, including current variable")
  {
    // Outside a transaction - these go through nsdb's auto transactions
    ns_db_switch (db, "var2", &e);
    u32 a[] = {10, 20, 30, 40};
    test_assert_int_equal (ns_db_insert (db, a, sizeof (a), 0, 4, &e), 4);

    ns_db_switch (db, "var3", &e);
    u32 b[] = {100, 200, 300, 400};
    test_assert_int_equal (ns_db_insert (db, b, sizeof (b), 0, 4, &e), 4);

    ns_db_switch (db, "var1", &e);

    // deletes var1 and switches to var2
    test_assert_int_equal (ns_db_delete_and_switch (db, "var2", &e), SUCCESS);
    check_cur ("var2");
    validate (((u32[]){10, 20, 30, 40}));

    ns_db_begin_txn (db, &e);
    {
      ns_db_switch (db, "var3", &e);
      check_cur ("var3");

      u32 c[] = {111, 222, 333, 444};
      test_assert_int_equal (ns_db_insert (db, c, sizeof (c), 0, 4, &e), 4);
      validate (((u32[]){111, 222, 333, 444, 100, 200, 300, 400}));
    }
    test_assert_int_equal (ns_db_rollback_txn (db, &e), SUCCESS);

    // Still var2
    check_cur ("var2");
    validate (((u32[]){10, 20, 30, 40}));

    // var3 didn't get changes
    ns_db_switch (db, "var3", &e);
    validate (((u32[]){100, 200, 300, 400}));
  }

  TEST_CASE ("commit persists the switch and the write")
  {
    ns_db_begin_txn (db, &e);
    {
      ns_db_switch (db, "var2", &e);
      u32 w[] = {111, 222, 333, 444};
      test_assert_int_equal (ns_db_write (db, w, sizeof (w), STR (0, 1, 4), &e), 4);
      validate (((u32[]){111, 222, 333, 444}));
    }
    test_assert_int_equal (ns_db_commit_txn (db, &e), SUCCESS);

    // Still on var2
    check_cur ("var2");
    validate (((u32[]){111, 222, 333, 444}));

    // var3 didn't get changes
    ns_db_switch (db, "var3", &e);
    validate (((u32[]){100, 200, 300, 400}));
  }

  TEST_CASE ("close and reopen keeps committed data and current variable")
  {
    test_assert_int_equal (ns_db_close_and_reopen (db, &e), SUCCESS);
    check_cur ("var3");
    validate (((u32[]){100, 200, 300, 400}));

    ns_db_switch (db, "var2", &e);
    validate (((u32[]){111, 222, 333, 444}));
  }

  TEST_CASE ("crash drops the open transaction")
  {
    ns_db_begin_txn (db, &e);
    {
      u32 w[] = {9, 9, 9, 9};
      ns_db_write (db, w, sizeof (w), STR (0, 1, 4), &e);
      validate (((u32[]){9, 9, 9, 9}));
    }
    test_assert_int_equal (ns_db_crash_and_reopen (db, &e), SUCCESS);

    check_cur ("var2");
    validate (((u32[]){111, 222, 333, 444}));
  }

  ns_db_close (db, &e);
  nsdb_cleanup ("./test_db.db", &e);

#  undef STR
#  undef validate
#  undef check_cur
}

#endif
