#include <assert.h>
#include <math.h>
#include <rpcndr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct nsdb  nsdb_t;
typedef struct error error;
typedef struct type  type;
typedef int          sb_size;
typedef unsigned int b_size;

int          type_byte_size (const struct type *t);
struct type *random_type ();
char        *type_str (struct type *t);
char        *random_name (int min, int max);
void         type_free (struct type *t);
int          nsdb_validate (nsdb_t *db);

// Lifecycle
nsdb_t *nsdb_open (const char *path);
int     nsdb_cleanup (const char *path);
nsdb_t *nsdb_new_context (nsdb_t *ns);
int     nsdb_close (nsdb_t *ns);
int     nsdb_crash (nsdb_t *ns);

// Simple api
const char *nsdb_strerror (nsdb_t *ns);
int         nsdb_perror (nsdb_t *ns, const char *prefix);
int         nsdb_delete (nsdb_t *ns, const char *vname);

// Transaction Support
int nsdb_begin (nsdb_t *ns);
int nsdb_commit (nsdb_t *ns);
int nsdb_rollback (nsdb_t *ns);

// Variable API
int nsdb_create (nsdb_t *ns, const char *name, const char *type);
int nsdb_delete (nsdb_t *ns, const char *name);

// Primary API
sb_size nsdb_len (nsdb_t *ns, const char *vname);

// In array notation [a:b:c]
#define STOP_PRESENT  (1 << 0) // [:c]
#define STEP_PRESENT  (1 << 1) // [:b:]
#define START_PRESENT (1 << 2) // [a:]
#define COLON_PRESENT (1 << 3) // [:]

sb_size nsdb_insert (nsdb_t *ns, const char *name, const void *src, sb_size ofst, b_size slen);

sb_size nsdb_write (
    nsdb_t     *ns,
    const char *name,
    const void *src,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags);

sb_size nsdb_read (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags);

sb_size nsdb_remove (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags);

struct action {
  // Number of unique groups = 2^n - 1

  enum action_type {
    BEGIN_TXN,        // Begin a new transaction
    COMMIT_TXN,       // Commit this open transaction
    ROLLBACK_TXN,     // Rollback this open transaction
    CRASH_AND_REOPEN, // Crash the database
    CLOSE_AND_REOPEN, // Close the database cleanly
    INSERT,           // Insert a random amount of data
    REMOVE,           // Remove a random amount of data
    READ,             // Read a random amount of data
    WRITE,            // Write a random amount of data
    CREATE,           // Create a new variable
    SWITCH,           // Switch to a different existing variable
    DELETE,           // Delete an existing variable

    AT_LEN,
  } type;

  int weight;
};

struct var_meta {
  char        *name;
  char        *typestr;
  struct type *type;
  int          len;
};

struct var_meta random_var_meta (int minname, int maxname) {
  char        *name    = random_name (minname, maxname);
  struct type *type    = random_type ();
  char        *typestr = type_str (type);

  return (struct var_meta){
      .name    = name,
      .typestr = typestr,
      .type    = type,
      .len     = 0,
  };
}

void free_var_meta (struct var_meta *meta) {
  free (meta->name);
  free (meta->typestr);
  type_free (meta->type);
}

struct swarm_test {
  // Existing variables to pull from during a switch
  struct var_meta vars[40];
  int             vlen; // Length

  // Currently selected variable
  struct var_meta *cur;

  int enabled[AT_LEN]; // What actions are curly available
  int allowed[AT_LEN]; // What actions we can actually take (e.g. remove is enabled but len = 0)

  // Main db instance
  nsdb_t *db;

  int         in_txn;
  const char *dbname;

  int max_insert_len;

  // Probability that after a test step that our list of enabled actions changes
  // When it does change - a random set of actions are selected from enabled
  // 0 means true swarm test
  float probability_of_swarm_change;

  // Probability that after a test step that we do one big read of all the data
  // in reference and system under test and compare them
  float probability_of_full_read;
};

/**
 * Let's say we have N actions, then the "swarm action space" is all
 * just the power sets of each item (minus the empty set):
 *
 * { INSERT }, { INSERT, READ }, { READ }, ....
 *
 * There are 2^N - 1 collections
 *
 * We wish to sample from this set of collections uniformly.
 *
 * Therefore, take a random number from [1, 2^n) and each bit
 * represents if the item is included or not
 */
void swmt_set_random_enabled (struct swarm_test *test) {
  int mask = rand () % ((1 << AT_LEN) - 1) + 1;
  for (int i = 0; i < AT_LEN; ++i) { test->enabled[i] = mask & (1 << i); }
}
void swmt_full_validation (struct swarm_test *test) { nsdb_validate (test->db); }

void swmt_set_allowed (struct swarm_test *test) {
  assert (test);
  assert (test->db);
  assert (test->dbname);

  memset (test->allowed, 0, sizeof (test->allowed));

  // Can always crash
  test->allowed[CRASH_AND_REOPEN] = 1;

  // Can create if we have room
  if (test->vlen < 40) { test->allowed[CREATE] = 1; }

  if (!test->in_txn) {
    // Can begin txn if not in txn
    test->allowed[BEGIN_TXN] = 1;

    // Can close if we're done with all txns
    test->allowed[CLOSE_AND_REOPEN] = 1;
  }

  if (test->in_txn) {
    // Can commit if we are in a txn
    test->allowed[COMMIT_TXN] = 1;

    // Can rollback if we are in a txn
    test->allowed[ROLLBACK_TXN] = 1;
  }

  if (test->cur) {
    assert (test->vlen > 0);

    // Can insert if we have any variable
    test->allowed[INSERT] = 1;

    if (test->cur->len > 0) {
      test->allowed[REMOVE] = 1;
      test->allowed[READ]   = 1;
      test->allowed[WRITE]  = 1;
    }
  } else {
    assert (test->vlen == 0);
  }

  if (test->vlen > 1) {
    test->allowed[DELETE] = 1;
    test->allowed[SWITCH] = 1;
  }
}

void swmt_assert (int result) {
  if (!result) {
    printf ("Failed swarm test\n");
    abort ();
  }
}

struct swarm_test *swmt_open (
    float       probability_of_swarm_change,
    float       probability_of_full_read,
    int         start_enabled[AT_LEN],
    const char *dbname,
    int         max_insert_len) {
  struct swarm_test *ret = malloc (sizeof *ret);

  swmt_assert (nsdb_cleanup ("dbname") == 0);

  *ret = (struct swarm_test){
      .vlen                        = 0,
      .cur                         = NULL,
      .db                          = nsdb_open (dbname),
      .in_txn                      = 0,
      .dbname                      = dbname,
      .max_insert_len              = max_insert_len,
      .probability_of_swarm_change = probability_of_swarm_change,
      .probability_of_full_read    = probability_of_full_read,
  };

  swmt_assert (ret->db != NULL);

  memcpy (ret->enabled, start_enabled, AT_LEN * sizeof (int));
}

void swmt_close (struct swarm_test *meta) {
  if (meta->in_txn) { swmt_commit_txn (meta); }
  swmt_assert (nsdb_close (meta->db));
  free (meta);
}

//////////////////////////////////////////////
///// Action Framework

void swmt_begin_txn (struct swarm_test *meta) {
  assert (!meta->in_txn);
  swmt_assert (nsdb_begin (meta->db) == 0);
  meta->in_txn = 1;
}

void swmt_commit_txn (struct swarm_test *meta) {
  assert (meta->in_txn);
  swmt_assert (nsdb_commit (meta->db) == 0);
  meta->in_txn = 0;
}

void swmt_rollback_txn (struct swarm_test *meta) {
  assert (meta->in_txn);
  swmt_assert (nsdb_rollback (meta->db) == 0);
  meta->in_txn = 0;
}

void swmt_crash_and_reopen (struct swarm_test *meta) {
  // Crash
  swmt_assert (nsdb_crash (meta->db) == 0);

  // Reopen
  meta->db = nsdb_open (meta->dbname);
  swmt_assert (meta->db != NULL);

  meta->in_txn = 0;
}

void swmt_close_and_reopen (struct swarm_test *meta) {
  // Close
  swmt_assert (nsdb_close (meta->db) == 0);

  // Reopen
  meta->db = nsdb_open (meta->dbname);
  swmt_assert (meta->db != NULL);

  meta->in_txn = 0;
}

void swmt_insert (struct swarm_test *meta) {
  // Construct Parameters
  int len  = (rand () % (meta->max_insert_len)) + 1;
  int ofst = rand () % (meta->cur->len + 1);

  // Initialize the data
  int      blen = len * type_byte_size (meta->cur->type);
  uint8_t *data = malloc (blen);
  for (int i = 0; i < blen; ++i) { data[i] = rand (); }

  // Insert data
  swmt_assert (nsdb_insert (meta->db, meta->cur->name, data, ofst, len) == 0);

  meta->cur->len += len;

  free (data);
}

void swmt_remove (struct swarm_test *meta) {
  // Construct Parameters
  int ofst   = rand () % (meta->cur->len + 1);               // [ 0, len ]
  int stride = rand () % meta->cur->len - ofst;              // [ 0, len - ofst ]
  int len    = rand () % ((meta->cur->len - ofst) / stride); // [ 0, (len - ofst) / stride ]

  // The removed data
  uint8_t *data = calloc (len, type_byte_size (meta->cur->type));

  // Do Remove
  swmt_assert (nsdb_remove (meta->db, meta->cur->name, data, ofst, stride, ofst + len, 0xFF) == 0);

  assert (meta->cur->len > len);
  meta->cur->len -= len;

  free (data);
}

void swmt_read (struct swarm_test *meta) {
  // Construct Parameters
  int ofst   = rand () % (meta->cur->len + 1);               // [ 0, len ]
  int stride = rand () % meta->cur->len - ofst;              // [ 0, len - ofst ]
  int len    = rand () % ((meta->cur->len - ofst) / stride); // [ 0, (len - ofst) / stride ]

  // The read data
  uint8_t *data = calloc (len, type_byte_size (meta->cur->type));

  // Do Remove
  swmt_assert (nsdb_read (meta->db, meta->cur->name, data, ofst, stride, ofst + len, 0xFF) == 0);

  free (data);
}

void swmt_write (struct swarm_test *meta) {
  // Construct Parameters
  int ofst   = rand () % (meta->cur->len + 1);               // [ 0, len ]
  int stride = rand () % meta->cur->len - ofst;              // [ 0, len - ofst ]
  int len    = rand () % ((meta->cur->len - ofst) / stride); // [ 0, (len - ofst) / stride ]

  // The data to write
  int      blen = len * type_byte_size (meta->cur->type);
  uint8_t *data = malloc (blen);
  for (int i = 0; i < blen; ++i) { data[i] = rand (); }

  // Do Remove
  swmt_assert (nsdb_write (meta->db, meta->cur->name, data, ofst, stride, ofst + len, 0xFF) == 0);

  free (data);
}

void swmt_create (struct swarm_test *meta) {
tryagain:
  struct var_meta v = random_var_meta (1, 100);

  // Avoid duplicates
  for (int i = 0; i < meta->vlen; ++i) {
    if (strcmp (v.name, meta->vars[i].name) == 0) {
      free_var_meta (&v);
      goto tryagain;
    }
  }

  swmt_assert (nsdb_create (meta->db, v.name, v.typestr) == 0);

  assert (meta->vlen < 40);
  meta->vars[meta->vlen++] = v;
}

void swmt_switch (struct swarm_test *meta) {
  assert (meta->vlen > 1);

tryagain:
  int              choice = rand () % meta->vlen;
  struct var_meta *next   = &meta->vars[choice];
  if (next == meta->cur) { goto tryagain; }
  meta->cur = next;
}

void swmt_delete (struct swarm_test *meta) {
  assert (meta->vlen > 0);

  struct var_meta *cur = meta->cur;
  swmt_assert (nsdb_delete (meta->db, cur->name) == 0);
  memmove (
      meta->cur,
      meta->cur + sizeof (struct var_meta),
      (40 * sizeof (struct var_meta)) - (size_t)meta->cur);
}

void swmt_step (struct swarm_test *meta) {
  int len = 0;
  for (int i = 0; i < AT_LEN; ++i) { len += meta->allowed[i]; }

  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < AT_LEN; ++index) {
    if (meta->allowed[index]) {
      if (choice == next) {
        break;
      } else {
        choice++;
      }
    }
  }

  enum action_type action = (enum action_type)index;

  switch (action) {
    case BEGIN_TXN: {
      swmt_begin_txn (meta);
      break;
    }
    case COMMIT_TXN: {
      swmt_commit_txn (meta);
      break;
    }
    case ROLLBACK_TXN: {
      swmt_rollback_txn (meta);
      break;
    }
    case CRASH_AND_REOPEN: {
      swmt_crash_and_reopen (meta);
      break;
    }
    case CLOSE_AND_REOPEN: {
      swmt_close_and_reopen (meta);
      break;
    }
    case INSERT: {
      swmt_insert (meta);
      break;
    }
    case REMOVE: {
      swmt_remove (meta);
      break;
    }
    case READ: {
      swmt_read (meta);
      break;
    }
    case WRITE: {
      swmt_write (meta);
      break;
    }
    case CREATE: {
      swmt_create (meta);
      break;
    }
    case SWITCH: {
      swmt_switch (meta);
      break;
    }
    case DELETE: {
      swmt_delete (meta);
      break;
    }
    default: {
      assert (0);
    }
  }

  swmt_set_allowed (meta);

  // Change the list of enabled ops
  if (rand () / RAND_MAX < meta->probability_of_swarm_change) { swmt_set_random_enabled (meta); }

  // Do a full validation step
  if (rand () / RAND_MAX < meta->probability_of_full_read) { swmt_full_validation (meta); }
}

void run_swarm_test () {
  int start_enabled[AT_LEN];
  for (int i = 0; i < AT_LEN; ++i) { start_enabled[i] = 1; }

  struct swarm_test *meta = swmt_open (0.1, 0.1, start_enabled, "test", 10000);

  // TODO - set up signal for timer

  while (1) { swmt_step (meta); }

  swmt_close (meta);
}
