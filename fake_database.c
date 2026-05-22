#define _POSIX_C_SOURCE 200809L /* for strdup */

#include "fake_database.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Block-array node capacity used when allocating new arrays. The exact
 * value doesn't matter for correctness; smaller values exercise more
 * nodes in the underlying tree. */
#define BLOCK_CAP_PER_NODE 256u

struct fake_database *fake_db_create (void) {
  struct fake_database *db = calloc (1, sizeof *db);
  return db;
}

void fake_db_free (struct fake_database *db) {
  if (!db) { return; }
  for (int i = 0; i < db->n; ++i) {
    free (db->vars[i].name);
    free (db->vars[i].typestr);
    block_array_free (db->vars[i].data);
  }
  free (db);
}

int fake_db_var_count (const struct fake_database *db) { return db->n; }

struct fake_var *fake_db_var_at (struct fake_database *db, int idx) {
  if (idx < 0 || idx >= db->n) { return NULL; }
  return &db->vars[idx];
}

struct fake_var *fake_db_find (struct fake_database *db, const char *name) {
  for (int i = 0; i < db->n; ++i) {
    if (strcmp (db->vars[i].name, name) == 0) { return &db->vars[i]; }
  }
  return NULL;
}

int fake_db_add_var (struct fake_database *db, char *name, char *typestr, u32 elem_size) {
  if (db->n >= FAKE_DB_MAX_VARS) { return -1; }
  if (fake_db_find (db, name) != NULL) { return -1; }

  struct block_array *data = block_array_create (BLOCK_CAP_PER_NODE, NULL);
  if (!data) { return -1; }

  db->vars[db->n] = (struct fake_var){
      .name      = name,
      .typestr   = typestr,
      .elem_size = elem_size,
      .data      = data,
  };
  db->n++;
  return 0;
}

int fake_db_remove_var (struct fake_database *db, const char *name) {
  for (int i = 0; i < db->n; ++i) {
    if (strcmp (db->vars[i].name, name) == 0) {
      free (db->vars[i].name);
      free (db->vars[i].typestr);
      block_array_free (db->vars[i].data);

      if (i < db->n - 1) {
        memmove (
            &db->vars[i],
            &db->vars[i + 1],
            (size_t)(db->n - i - 1) * sizeof (struct fake_var));
      }
      db->n--;
      return 0;
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------- */
/*  Clone                                                                 */
/* ---------------------------------------------------------------------- */

/**
 * Deep-copy one variable's block_array. Reads the entire source as a
 * contiguous byte stream and bulk-inserts into a fresh array.
 */
static struct block_array *clone_block_array (const struct block_array *src, u32 elem_size) {
  struct block_array *dst = block_array_create (BLOCK_CAP_PER_NODE, NULL);
  if (!dst) { return NULL; }

  u64 total_bytes = block_array_getlen (src);
  if (total_bytes == 0) { return dst; }

  void *buf = malloc ((size_t)total_bytes);
  if (!buf) {
    block_array_free (dst);
    return NULL;
  }

  struct stride str = {
      .start  = 0,
      .stride = elem_size,
      .nelems = total_bytes / elem_size,
  };
  block_array_read (src, str, elem_size, buf);

  err_t err = block_array_insert (dst, 0, buf, (u32)total_bytes, NULL);
  free (buf);

  if (err != 0) {
    block_array_free (dst);
    return NULL;
  }
  return dst;
}

struct fake_database *fake_db_clone (const struct fake_database *src) {
  struct fake_database *dst = fake_db_create ();
  if (!dst) { return NULL; }

  for (int i = 0; i < src->n; ++i) {
    const struct fake_var *sv = &src->vars[i];

    char               *name    = strdup (sv->name);
    char               *typestr = strdup (sv->typestr);
    struct block_array *data    = clone_block_array (sv->data, sv->elem_size);

    if (!name || !typestr || !data) {
      free (name);
      free (typestr);
      block_array_free (data);
      fake_db_free (dst);
      return NULL;
    }

    dst->vars[i] = (struct fake_var){
        .name      = name,
        .typestr   = typestr,
        .elem_size = sv->elem_size,
        .data      = data,
    };
  }
  dst->n = src->n;
  return dst;
}
