#ifndef FAKE_DATABASE_H
#define FAKE_DATABASE_H

#include "block_array_forward.h"

/*
 * fake_database: an in-memory reference model of an nsdb database.
 *
 * It is a flat list of variables, each holding its own block_array of data.
 * Lookup is by linear scan over the names (string compare). The structure
 * is intentionally simple: no hashing, no indexes, no transactions.
 *
 * Transactions are NOT a concern of this layer. To get transactional
 * behaviour, keep two of these around (committed and working) and snapshot
 * via fake_db_clone() on BEGIN_TXN.
 */

#define FAKE_DB_MAX_VARS 64

/**
 * A single tracked variable: name, type-as-string, element size in bytes,
 * and the reference data in a block_array.
 */
struct fake_var {
  char               *name;
  char               *typestr;
  u32                 elem_size;
  struct block_array *data;
};

/**
 * Flat container of fake_vars. Exposed by value because the swarm test
 * is the only consumer and inlining keeps things simple.
 */
struct fake_database {
  struct fake_var vars[FAKE_DB_MAX_VARS];
  int             n;
};

/* ---------------------------------------------------------------------- */
/*  Lifecycle                                                             */
/* ---------------------------------------------------------------------- */

/** Allocate an empty fake_database. */
struct fake_database *fake_db_create (void);

/**
 * Free a fake_database and everything it owns (names, typestrs, block
 * arrays). Safe to call with NULL.
 */
void fake_db_free (struct fake_database *db);

/**
 * Deep-copy a fake_database. Used to snapshot the committed state into
 * a working copy at BEGIN_TXN. Allocates new names/typestrs/block_arrays
 * for every variable.
 */
struct fake_database *fake_db_clone (const struct fake_database *src);

/* ---------------------------------------------------------------------- */
/*  Variable management                                                   */
/* ---------------------------------------------------------------------- */

/** Number of variables currently tracked. */
int fake_db_var_count (const struct fake_database *db);

/** Get the variable at @p idx, or NULL if out of range. */
struct fake_var *fake_db_var_at (struct fake_database *db, int idx);

/** Find a variable by name. Returns NULL if not found. */
struct fake_var *fake_db_find (struct fake_database *db, const char *name);

/**
 * Add a new variable. Takes ownership of @p name and @p typestr (they are
 * stored verbatim; do not free them after this call). An empty block_array
 * is allocated automatically.
 *
 * @return 0 on success, -1 if the database is full or the name collides.
 */
int fake_db_add_var (struct fake_database *db, char *name, char *typestr, u32 elem_size);

/**
 * Remove a variable by name, freeing all its owned resources and shifting
 * later entries down.
 *
 * @return 0 if removed, -1 if not found.
 */
int fake_db_remove_var (struct fake_database *db, const char *name);

#endif /* FAKE_DATABASE_H */
