#ifndef BLOCK_ARRAY_FORWARD_H
#define BLOCK_ARRAY_FORWARD_H

#include <stdint.h>

/*
 * Forward declarations for the block_array reference model used by the
 * swarm test. The actual implementation lives in the project under test;
 * this header just lets the swarm test compile against it.
 *
 * All sizes/offsets in the block_array API are in BYTES, not elements.
 * The `size` parameter on read/write/remove is the element size in bytes.
 */

typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef int      err_t;

/* `error` is also forward-declared in nsdb_forward.h; both decls are
 * compatible because they describe the same incomplete struct. */
struct error;
struct block_array;
struct data_writer;

/**
 * A resolved, internal stride descriptor for tree operations.
 *
 *   start  -- byte offset at which to begin
 *   stride -- bytes to advance between successive elements
 *   nelems -- number of elements to access
 */
struct stride {
  u64 start;
  u64 stride;
  u64 nelems;
};

struct block_array *block_array_create (u32 cap_per_node, struct error *e);
void                block_array_free (struct block_array *r);

err_t block_array_insert (
    struct block_array *r,
    u32                 ofst,
    const void         *src,
    u32                 slen,
    struct error       *e);

u64 block_array_read (const struct block_array *r, struct stride str, u32 size, void *dest);

u64 block_array_write (const struct block_array *r, struct stride str, u32 size, const void *src);

i64 block_array_remove (
    struct block_array *r,
    struct stride       str,
    u32                 size,
    void               *dest,
    struct error       *e);

u64 block_array_getlen (const struct block_array *r);

/* Array accessor pattern */
void *block_array_get (struct block_array *r, u64 idx);
void  block_array_set (struct block_array *r, u64 idx, const void *data, u32 dlen);

void block_array_data_writer (struct data_writer *dest, struct block_array *arr);

#endif /* BLOCK_ARRAY_FORWARD_H */
