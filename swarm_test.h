#ifndef SWARM_TEST_H
#define SWARM_TEST_H

#include "block_array_forward.h"
#include "nsdb_forward.h"

/*
 * Swarm test for the nsdb database.
 *
 * A swarm test repeatedly selects a random action from a "swarm" (a uniformly
 * chosen subset of the full action set) and applies it to the database. The
 * test maintains a parallel reference model -- a `fake_database` (in-memory
 * list of named block_arrays) -- so that every operation can be cross-checked
 * against a known-good implementation.
 *
 * Transaction model
 * -----------------
 * The fake_database itself knows nothing about transactions. The swarm test
 * wraps it in a small transactional layer:
 *
 *   - Outside a txn: exactly ONE fake_database exists (`committed`). All
 *     operations modify it in place (auto-commit).
 *   - BEGIN_TXN:    Deep-copy committed -> working. Now two databases exist.
 *   - During txn:   Operations modify `working` only.
 *   - COMMIT:       Free `committed`, move `working` into its place.
 *   - ROLLBACK:     Free `working` (current work discarded).
 *   - CRASH+REOPEN: Free `working` (current work discarded), reopen nsdb.
 *
 * This naturally extends to transactional CREATE/DELETE: a rollback throws
 * away the entire working snapshot, which contains any vars created or
 * deleted during the txn.
 */

/* ---------------------------------------------------------------------- */
/*  Action enumeration                                                    */
/* ---------------------------------------------------------------------- */

/**
 * Every action the swarm test knows how to perform.
 *
 * AT_LEN is the count of real actions (used to size arrays and to bound
 * the "power set" sampling). Do not add anything after AT_LEN.
 */
enum action_type {
  BEGIN_TXN,        /* Begin a new transaction                            */
  COMMIT_TXN,       /* Commit the open transaction                        */
  ROLLBACK_TXN,     /* Rollback the open transaction                      */
  CRASH_AND_REOPEN, /* Crash the database, then reopen it                 */
  CLOSE_AND_REOPEN, /* Cleanly close the database, then reopen it         */
  INSERT,           /* Insert a random amount of data                     */
  REMOVE,           /* Remove a random amount of data                     */
  READ,             /* Read a random amount of data                       */
  WRITE,            /* Write a random amount of data                      */
  CREATE,           /* Create a new variable                              */
  SWITCH,           /* Switch to a different existing variable            */
  DELETE,           /* Delete an existing variable                        */

  AT_LEN, /* Sentinel: number of action types                   */
};

/**
 * Weighted action descriptor. Reserved for future biased selection; the
 * current step loop treats all allowed actions uniformly.
 */
struct action {
  enum action_type type;
  int              weight;
};

/* ---------------------------------------------------------------------- */
/*  Swarm test handle (opaque)                                            */
/* ---------------------------------------------------------------------- */

struct swarm_test;

/* ---------------------------------------------------------------------- */
/*  Public API                                                            */
/* ---------------------------------------------------------------------- */

/**
 * Allocate and initialise a new swarm test.
 *
 * The database at @p dbname is cleaned up and re-opened fresh; an empty
 * committed fake_database is created. The caller must eventually pass the
 * returned handle to swmt_close().
 *
 * @param probability_of_swarm_change   Probability in [0,1] that after a step
 *                                      the enabled action subset is re-rolled.
 *                                      Pass 0 for a fixed-swarm test.
 * @param probability_of_full_read      Probability in [0,1] that after a step
 *                                      we run a full validation pass.
 * @param start_enabled                 Array of length AT_LEN giving the
 *                                      initial enabled mask (1 = enabled).
 * @param dbname                        Path of the database file to use.
 * @param max_insert_len                Upper bound on insertion length per
 *                                      step.
 *
 * @return A new swarm_test handle, or aborts on failure.
 */
struct swarm_test *swmt_open (
    float       probability_of_swarm_change,
    float       probability_of_full_read,
    int         start_enabled[AT_LEN],
    const char *dbname,
    int         max_insert_len);

/**
 * Close a swarm test, releasing the database, both fake_databases, and any
 * tracked metadata. Any open transaction is committed first. After this
 * call, @p meta is no longer valid.
 */
void swmt_close (struct swarm_test *meta);

/**
 * Run one swarm test step: pick a random allowed action and apply it to
 * both the nsdb under test and the fake_database reference model. After
 * the action, possibly re-roll the swarm and/or run a full validation pass.
 */
void swmt_step (struct swarm_test *meta);

/**
 * Convenience driver: open a test with all actions enabled and step
 * forever. Intended to be paired with an alarm signal that terminates
 * the loop; see the TODO in the implementation.
 */
void run_swarm_test (void);

#endif /* SWARM_TEST_H */
