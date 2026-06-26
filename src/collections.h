/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "alloc.h"       // slab alloc
#include "concurrency.h" // latch
#include "error.h"       // err_t
#include "os.h"          // i_file
#include "platform.h"    // HEADER_FUNC
#include "stdtypes.h"    // u32 ...etc

/******************************************************************************
 * SECTION: Linked List
 * ----------------------------------------------------------------------------
 *
 * @brief A header-only linked list implemented as an intrusive data structure.
 *
 * @par Usage:
 * Embed `struct llnode` directly inside your user-defined structure. Use
 * typecasting or container offset macros to translate back from node to object.
 *
 * @code
 *
 * struct foo {
 *    int data;
 *    struct llnode node; // Embed a linked list node in your struct
 * };
 *
 * struct llnode *head = NULL;
 *
 * struct foo item1 = { .data = 42 };
 * llnode_init(&item1.node);
 *
 * list_push(&head, &item1.node);
 *
 * LLIST_FOR_EACH(head, iter) {
 *    struct foo *f = (struct foo *)((char *)iter - offsetof(struct foo, node));
 *    printf("Data: %d\n", f->data);
 * }
 *
 * @endcode
 ******************************************************************************/

struct llnode
{
  struct llnode *next;
};

HEADER_FUNC void
llnode_init (struct llnode *n)
{
  n->next = NULL;
}

HEADER_FUNC u32
list_length (const struct llnode *head)
{
  u32 len = 0;
  for (const struct llnode *cur = head; cur; cur = cur->next)
  {
    len++;
  }
  return len;
}

HEADER_FUNC void
list_push (struct llnode **head, struct llnode *n)
{
  n->next = *head;
  *head   = n;
}

HEADER_FUNC void
list_append (struct llnode **head, struct llnode *n)
{
  n->next = NULL;
  if (!*head)
  {
    *head = n;
  }
  else
  {
    struct llnode *cur = *head;
    while (cur->next)
    {
      cur = cur->next;
    }
    cur->next = n;
  }
}

HEADER_FUNC struct llnode *
list_pop (struct llnode **head)
{
  if (!*head)
  {
    return NULL;
  }

  struct llnode *n = *head;
  *head            = n->next;
  n->next          = NULL;

  return n;
}

HEADER_FUNC struct llnode *
list_find (
    u32                 *didx,
    struct llnode       *head,
    const struct llnode *node,
    bool (*eq) (const struct llnode *left, const struct llnode *right)
)
{
  *didx = 0;
  for (struct llnode *iter = (head); iter; iter = iter->next, *didx = *didx + 1)
  {
    if (eq (iter, node))
    {
      return iter;
    }
  }
  return NULL;
}

HEADER_FUNC void
list_remove (struct llnode **head, struct llnode *n)
{
  struct llnode **cur = head;
  while (*cur && *cur != n)
  {
    cur = &(*cur)->next;
  }
  if (*cur)
  {
    *cur    = n->next;
    n->next = NULL;
  }
}

HEADER_FUNC struct llnode *
llnode_get_n (struct llnode *head, const u32 index)
{
  struct llnode *cur = head;
  for (u32 i = 0; cur && i < index; ++i)
  {
    cur = cur->next;
  }
  return cur;
}

// Iterate over list
#define LLIST_FOR_EACH(head, iter) \
  for (llnode *iter = (head); iter; iter = iter->next)

/******************************************************************************
 * SECTION: Stride
 * ----------------------------------------------------------------------------
 *
 * @brief Stride is a pattern for iterating through an array
 ******************************************************************************/

/**
 * @struct stride
 * @brief A more tight stride than user_stride that restricts to the domain of
 * an array
 *
 * Stride is the main point of entry for any strided operation. User stride is
 * just the user facing version - user strides are "resolved" into strides
 *
 * @var stride::start
 * @brief The start index
 *
 * @var stride::stride
 * @brief The step between each element
 *
 * @var stride::nelems
 * @brief The number of elements to touch
 */
struct stride
{
  u64 start;
  u64 stride;
  u64 nelems;
};

enum
{
  START_PRESENT = (1 << 0),
  STEP_PRESENT  = (1 << 1),
  STOP_PRESENT  = (1 << 2),
  COLON_PRESENT = (1 << 3),
};

/**
 * @struct user_stride
 * @brief The user stride is an easy way for the user to define a stride based
 * on the variable
 *
 * In normal stride operations, queries like [0:-1] or [0::] or [:-1] etc are
 * all valid queries. User stride encodes this property and there's a single
 * function to convert a user stride into a stride for more rigorous stride
 * operations
 *
 * @var user_stride::start
 * @brief The start element - meaningless if present & START_PRESENT is 0
 * Negative means from the end
 *
 * @var user_stride::step
 * @brief The step - equivalent to stride::stride
 * meaningless if present & STEP_PRESENT is 0
 *
 * @var user_stride::end
 * @brief The end element - meaningless if present & STOP_PRESENT is 0
 *
 * @var user_stride::present
 * @brief A set of flags on which value is present.
 */

struct user_stride
{
  i64 start;
  i64 step;
  i64 stop;
  u32 present;
};

struct multi_user_stride
{
  struct user_stride *strides;
  u32                 len;
};

/**
 * @brief all elements from 0 to end
 */
#define USER_STRIDE_ALL                        \
  ((struct user_stride){                       \
      .start   = 0,                            \
      .step    = 1,                            \
      .stop    = 0,                            \
      .present = STEP_PRESENT | START_PRESENT, \
  })

bool ustride_equal (struct user_stride left, struct user_stride right);
bool user_stride_equal (
    const struct user_stride *left,
    const struct user_stride *right
);
void
stride_resolve_expect (struct stride *dest, struct user_stride src, u64 arrlen);
err_t stride_resolve (
    struct stride     *dest,
    struct user_stride src,
    u64                arrlen,
    error             *e
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Small Constructors
 * @brief Short Constructors for building user strides
 *----------------------------------------------------------------------------*/

#define make_ustride(start_, stop_, step_, present_) \
  ((struct user_stride){                             \
      .start   = (start_),                           \
      .stop    = (stop_),                            \
      .step    = (step_),                            \
      .present = (present_),                         \
  })

// [:stop]
HEADER_FUNC struct user_stride
ustride1 (i64 stop)
{
  return make_ustride (0, stop, 0, STOP_PRESENT | COLON_PRESENT);
}

// [::step]
HEADER_FUNC struct user_stride
ustride2 (i64 step)
{
  return make_ustride (0, 0, step, STEP_PRESENT | COLON_PRESENT);
}

// [:stop:step]
HEADER_FUNC struct user_stride
ustride12 (i64 stop, i64 step)
{
  return make_ustride (
      0,
      stop,
      step,
      STOP_PRESENT | STEP_PRESENT | COLON_PRESENT
  );
}

// [start:]
HEADER_FUNC struct user_stride
ustride0 (i64 start)
{
  return make_ustride (start, 0, 0, START_PRESENT | COLON_PRESENT);
}

// [start:stop]
HEADER_FUNC struct user_stride
ustride01 (i64 start, i64 stop)
{
  return make_ustride (
      start,
      stop,
      0,
      STOP_PRESENT | START_PRESENT | COLON_PRESENT
  );
}

// [start::step]
HEADER_FUNC struct user_stride
ustride02 (i64 start, i64 step)
{
  return make_ustride (
      start,
      0,
      step,
      STEP_PRESENT | START_PRESENT | COLON_PRESENT
  );
}

// [start:stop:step]
HEADER_FUNC struct user_stride
ustride012 (i64 start, i64 stop, i64 step)
{
  return make_ustride (
      start,
      stop,
      step,
      STOP_PRESENT | STEP_PRESENT | START_PRESENT | COLON_PRESENT
  );
}

// [start]  — bare index, no colon
HEADER_FUNC struct user_stride
ustride_single (i64 start)
{
  return make_ustride (start, 0, 0, START_PRESENT);
}

// [:]  — colon only
HEADER_FUNC struct user_stride
ustride (void)
{
  return make_ustride (0, 0, 0, COLON_PRESENT);
}

HEADER_FUNC struct user_stride
usfrms (const struct stride str)
{
  return ustride012 (
      str.start,
      str.start + str.stride * str.nelems,
      str.stride
  );
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Multi User Stride Builder
 *----------------------------------------------------------------------------*/

struct mus_llnode
{
  struct user_stride stride;
  struct llnode      link;
};

struct mus_builder
{
  struct llnode      *head;
  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void musb_create (
    struct mus_builder *dest,
    struct chunk_alloc *temp,
    struct chunk_alloc *persistent
);

err_t
musb_accept_key (struct mus_builder *eb, struct user_stride stride, error *e);

err_t musb_build (
    struct multi_user_stride *persistent,
    const struct mus_builder *eb,
    error                    *e
);

/******************************************************************************
 * SECTION: Data Writer
 * ----------------------------------------------------------------------------
 *
 * @brief A polymorphic data writer
 *
 * This is a polymorphic set of functions that encodes
 * insert, read write and remove. Useful in testing numstore
 * array patterns by defining them in memory and comparing
 ******************************************************************************/

typedef err_t (*insert_func) (
    void       *ctx,
    u32         ofst,
    const void *src,
    u32         slen,
    error      *e
);
typedef i64 (*read_func) (
    void         *ctx,
    struct stride str,
    u32           size,
    void         *dest,
    error        *e
);
typedef i64 (*write_func) (
    void         *ctx,
    struct stride str,
    u32           size,
    const void   *src,
    error        *e
);
typedef i64 (*remove_func) (
    void         *ctx,
    struct stride str,
    u32           size,
    void         *dest,
    error        *e
);
typedef i64 (*get_len_func) (void *ctx, error *e);

struct data_writer_functions
{
  insert_func  insert;
  read_func    read;
  write_func   write;
  remove_func  remove;
  get_len_func getlen;
};

struct data_writer
{
  struct data_writer_functions functions;
  void                        *ctx;
};

/******************************************************************************
 * SECTION: Double Buffer
 * ----------------------------------------------------------------------------
 *
 * @brief A standard doubling buffer - kind of like C++ vector allocation
 * semantics
 *
 * @par Usage:
 * @code
 *
 * TODO
 *
 * @endcode
 ******************************************************************************/

struct dbl_buffer
{
  latch latch;
  void *data;
  u32   size;
  u32   nelem_cap;
  u32   nelem;
};

err_t
dblb_create (struct dbl_buffer *dest, u32 size, u32 initial_cap, error *e);
err_t dblb_append (struct dbl_buffer *d, const void *data, u32 nelem, error *e);
err_t dblb_ensure_space (struct dbl_buffer *d, u32 nelem, error *e);
void *dblb_append_alloc (struct dbl_buffer *d, u32 nelem, error *e);
void  dblb_free (struct dbl_buffer *d); // Target buffer

/******************************************************************************
 * SECTION: Extending array
 * ----------------------------------------------------------------------------
 *
 * @brief A [data_writer] that doubles in length when it reaches a limit
 *
 * @par Usage:
 * @code
 *
 * TODO
 *
 * @endcode
 ******************************************************************************/

struct ext_array
{
  u8 *data;
  u32 len;
  u32 cap;
};

struct ext_array ext_array_create (void);
void             ext_array_free (struct ext_array *r);

i64 ext_array_insert (
    struct ext_array *r,
    u32               ofst,
    const void       *src,
    u32               slen,
    error            *e
);
i64 ext_array_read (
    const struct ext_array *r,
    struct stride           str,
    u32                     size,
    void                   *dest,
    error                  *e
);
i64 ext_array_write (
    const struct ext_array *r,
    struct stride           str,
    u32                     size,
    const void             *src,
    error                  *e
);
i64 ext_array_remove (
    struct ext_array *r,
    struct stride     str,
    u32               size,
    void             *dest,
    error            *e
);
u64 ext_array_get_len (const struct ext_array *r);

void ext_array_data_writer (struct data_writer *dest, struct ext_array *arr);

/******************************************************************************
 * SECTION: Block Array
 * ----------------------------------------------------------------------------
 *
 * @brief A [data_writer] that extends by allocating chunks and linking nodes
 * together
 *
 * TODO - a diagram here
 *
 * @par Usage:
 * @code
 *
 * TODO
 *
 * @endcode
 ******************************************************************************/

/**
 * @class block
 * @brief A block is an individual node in a block array that holds
 * data array that is sized [cap_per_node] of it's parent and it has a
 * length that is how full it is
 */
struct block
{
  struct block *next;
  struct block *prev;
  u32           len;
  u8            data[];
};

/**
 * @class block_array
 * @brief A block array is a linked listed of data blocks that
 * represent byte data. Each block has capacity [cap_per_node] and
 * links to their neighbors
 */
struct block_array
{
  struct slab_alloc block_alloc; // Allocates blocks
  u32               cap_per_node;
  struct block     *head;

  u32 tlen;  // length of tail
  u8 tail[]; // A temporary buffer used for storing the right half of a block on
             // insert
};

struct block_array *block_array_create (u32 cap_per_node, error *e);
struct block_array *block_array_clone (const struct block_array *r, error *e);
void                block_array_free (struct block_array *r);

err_t block_array_insert (
    struct block_array *r,
    u32                 ofst,
    const void         *src,
    u32                 slen,
    error              *e
);

u64 block_array_read (
    const struct block_array *r,
    struct stride             str,
    u32                       size,
    void                     *dest
);

u64 block_array_write (
    const struct block_array *r,
    struct stride             str,
    u32                       size,
    const void               *src
);

i64 block_array_remove (
    struct block_array *r,
    struct stride       str,
    u32                 size,
    void               *dest,
    error              *e
);

u64 block_array_getlen (const struct block_array *r);

// Array accessor pattern
void *block_array_get (struct block_array *r, u64 idx);

void
block_array_set (struct block_array *r, u64 idx, const void *data, u32 dlen);

void
block_array_data_writer (struct data_writer *dest, struct block_array *arr);

/******************************************************************************
 * SECTION: Circular Buffer
 * ----------------------------------------------------------------------------
 *
 * @brief A ring buffer
 *
 *
 * A cbuffer wraps a fixed-size byte array and maintains head/tail pointers
 * to implement a FIFO queue. No heap allocation is performed - the caller
 * owns the backing memory and decides its lifetime.
 *
 * Layout:
 * @code
 * [-------------++++++++++++++++------------]
 * ^tail          ^head
 * @endcode
 *
 * Data occupies [tail, head). When the buffer is full, head == tail and
 * isfull is true.
 *
 * @par Usage:
 * @code
 *
 * TODO
 *
 * @endcode
 ******************************************************************************/

/**
 * @struct cbuffer
 * @brief Circular ring buffer tracking byte array sequences.
 *
 * @var cbuffer::data
 * @brief Pointer to the caller-supplied backing array.
 *
 * @var cbuffer::cap
 * @brief Total capacity of the backing array in bytes.
 *
 * @var cbuffer::head
 * @brief Write cursor - next byte is written here.
 *
 * @var cbuffer::tail
 * @brief Read cursor - next byte is read from here.
 *
 * @var cbuffer::isfull
 * @brief True when head == tail and the buffer is full (not empty).
 */
struct cbuffer
{
  u8  *data;
  u32  cap;
  u32  head;
  u32  tail;
  bool isfull;
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: cbuffer Creation
 * @brief Creating cbuffers
 *----------------------------------------------------------------------------*/

/**
 * @def cbuffer_create_from
 * @brief Creates a cbuffer over an existing array with zero initial length.
 * @param data Pointer to the backing array.
 */
#define cbuffer_create_from(data) cbuffer_create (data, sizeof data)

/**
 * @def cbuffer_create_full_from
 * @brief Creates a cbuffer over an existing array, treating it as full.
 * @param data Pointer to the backing array (already filled).
 */
#define cbuffer_create_full_from(data) \
  cbuffer_create_with (data, sizeof data, sizeof data)

/**
 * @def cbuffer_create_from_cstr
 * @brief Creates a cbuffer from a C string, treating the string bytes as data.
 * @param cstr Null-terminated string to wrap (length is strlen(cstr)).
 */
#define cbuffer_create_from_cstr(cstr) \
  cbuffer_create_with (cstr, strlen (cstr), strlen (cstr))

/**
 * @fn struct cbuffer cbuffer_create(void *data, u32 cap)
 * @brief Creates an empty cbuffer over a caller-supplied array.
 *
 * @param data Pointer to the backing array.
 * @param cap Size of the backing array in bytes.
 * @return Initialized cbuffer with no data.
 */
struct cbuffer cbuffer_create (void *data, u32 cap);

/**
 * @fn struct cbuffer cbuffer_create_with(void *data, u32 cap, u32 len)
 * @brief Creates a cbuffer with an initial data length already present.
 *
 * @param data Pointer to the backing array (first len bytes are considered
 * data).
 * @param cap Total size of the backing array in bytes.
 * @param len Number of bytes already present in the buffer.
 * @return Initialized cbuffer with head advanced by len.
 */
struct cbuffer cbuffer_create_with (void *data, u32 cap, u32 len);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Properties
 * @brief Properties on cbuffers
 *----------------------------------------------------------------------------*/

/**
 * @fn u32 cbuffer_len(const struct cbuffer *b)
 * @brief Returns the number of bytes currently in the buffer.
 *
 * @param b The cbuffer (must not be NULL).
 * @return Number of bytes available to read.
 */
HEADER_FUNC u32
cbuffer_len (const struct cbuffer *b)
{
  u32 len;
  if (b->isfull)
  {
    len = b->cap;
  }
  else if (b->head >= b->tail)
  {
    len = b->head - b->tail;
  }
  else
  {
    len = b->cap - (b->tail - b->head);
  }
  return len;
}

DEFINE_DBG_ASSERT (struct cbuffer, cbuffer, b, {
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (b->data);
  if (b->isfull)
  {
    ASSERT (b->tail == b->head);
  }
  ASSERT (cbuffer_len (b) <= b->cap);
})

/**
 * @fn bool cbuffer_isempty(const struct cbuffer *b)
 * @brief Returns true if the buffer contains no data.
 *
 * @param b The cbuffer (must not be NULL).
 * @return True if empty, false otherwise.
 */
HEADER_FUNC bool
cbuffer_isempty (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  return (!b->isfull && b->head == b->tail);
}

/**
 * @fn u32 cbuffer_slen(const struct cbuffer *b, const u32 size)
 * @brief Returns the number of elements of size bytes currently in the buffer.
 *
 * @param b The cbuffer.
 * @param size Element size in bytes - must evenly divide the current length.
 * @return Number of whole elements present.
 */
HEADER_FUNC u32
cbuffer_slen (const struct cbuffer *b, const u32 size)
{
  const u32 len = cbuffer_len (b);
  ASSERT (len % size == 0);
  return len / size;
}

/**
 * @fn u32 cbuffer_avail(const struct cbuffer *b)
 * @brief Returns the number of bytes available for writing.
 *
 * @param b The cbuffer (must not be NULL).
 * @return Bytes of free space remaining.
 */
HEADER_FUNC u32
cbuffer_avail (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  return b->cap - len;
}

/**
 * @fn u32 cbuffer_savail(const struct cbuffer *b, const u32 size)
 * @brief Returns the number of elements of size bytes that can still be
 * written.
 *
 * @param b The cbuffer (must not be NULL).
 * @param size Element size in bytes - must evenly divide the current length.
 * @return Number of whole elements that fit in the remaining space.
 */
HEADER_FUNC u32
cbuffer_savail (const struct cbuffer *b, const u32 size)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  ASSERT (len % size == 0);
  return (b->cap - len) / size;
}

/**
 * @fn void cbuffer_discard_all(struct cbuffer *b)
 * @brief Resets the buffer to empty, discarding all data.
 *
 * @param b The cbuffer to reset.
 */
void cbuffer_discard_all (struct cbuffer *b);

/**
 * @fn struct bytes cbuffer_get_next_avail_bytes(const struct cbuffer *b)
 * @brief Returns a bytes view of the next contiguous free region in the backing
 * array.
 *
 * @param b The cbuffer context.
 * @return Fragmented view of free continuous memory.
 */
struct bytes cbuffer_get_next_avail_bytes (const struct cbuffer *b);

/**
 * @fn struct bytes cbuffer_get_next_data_bytes(const struct cbuffer *b)
 * @brief Returns a bytes view of the next contiguous data region in the backing
 * array.
 *
 * @param b The cbuffer context.
 * @return Fragmented view of active data contiguous memory.
 */
struct bytes cbuffer_get_next_data_bytes (const struct cbuffer *b);

/**
 * @fn void cbuffer_fakeread(struct cbuffer *b, u32 bytes)
 * @brief Advances the tail pointer by bytes, as if that many bytes were read.
 *
 * @param b The target cbuffer.
 * @param bytes Number of byte indices to advance.
 */
void cbuffer_fakeread (struct cbuffer *b, u32 bytes);

/**
 * @fn void cbuffer_fakewrite(struct cbuffer *b, u32 bytes)
 * @brief Advances the head pointer by bytes, as if that many bytes were
 * written.
 *
 * @param b The target cbuffer.
 * @param bytes Number of byte indices to advance.
 */
void cbuffer_fakewrite (struct cbuffer *b, u32 bytes);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Raw Read / Write from bytes
 * @brief Reading and writing to cbuffers from / to bytes
 *----------------------------------------------------------------------------*/

/**
 * @fn u32 cbuffer_read(void *dest, u32 size, u32 n, struct cbuffer *b)
 * @brief Consumes elements out of the ring buffer into a destination buffer.
 *
 * @param dest Memory target destination.
 * @param size Unit size of elements.
 * @param n Explicit quantity of elements to transfer.
 * @param b The source ring buffer.
 * @return Elements successfully processed.
 */
u32 cbuffer_read (void *dest, u32 size, u32 n, struct cbuffer *b);

/**
 * @fn u32 cbuffer_copy(void *dest, u32 size, u32 n, const struct cbuffer *b)
 * @brief Inspects elements out of the ring buffer without mutating tracking
 * pointers.
 *
 * @param dest Memory target destination.
 * @param size Unit size of elements.
 * @param n Explicit quantity of elements to view.
 * @param b The source ring buffer.
 * @return Elements successfully copied.
 */
u32 cbuffer_copy (void *dest, u32 size, u32 n, const struct cbuffer *b);

/**
 * @fn u32 cbuffer_write(const void *src, u32 size, u32 n, struct cbuffer *b)
 * @brief Appends elements from an external slice onto the ring buffer payload.
 *
 * @param src Memory source data pointer.
 * @param size Unit size of elements.
 * @param n Explicit quantity of elements to add.
 * @param b The target ring buffer.
 * @return Elements successfully written.
 */
u32 cbuffer_write (const void *src, u32 size, u32 n, struct cbuffer *b);

/**
 * @def cbuffer_read_expect
 * @brief Reads exactly n elements - ASSERTs if the buffer does not have enough
 * data.
 */
#define cbuffer_read_expect(dest, size, n, b)     \
  do                                              \
  {                                               \
    u32 __read = cbuffer_read (dest, size, n, b); \
    ASSERT (__read == n);                         \
  }                                               \
  while (0)

/**
 * @def cbuffer_write_expect
 * @brief Writes exactly n elements - ASSERTs if the buffer does not have enough
 * space.
 */
#define cbuffer_write_expect(src, size, n, b)        \
  do                                                 \
  {                                                  \
    u32 __written = cbuffer_write (src, size, n, b); \
    ASSERT (__written == n);                         \
  }                                                  \
  while (0)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Raw Read / Write from other cbuffers
 * @brief Reading and writing to cbuffers from / to other cbuffers
 *----------------------------------------------------------------------------*/

/**
 * @fn u32 cbuffer_cbuffer_move(struct cbuffer *dest, u32 size, u32 n, struct
 * cbuffer *src)
 * @brief Dequeues items out of a source buffer and pushes them directly onto a
 * destination buffer.
 *
 * @param dest Target ring buffer destination.
 * @param size Sizing dimensions of items.
 * @param n Explicit quantity to transfer.
 * @param src Source ring buffer generator.
 * @return Total elements moved.
 */
u32 cbuffer_cbuffer_move (
    struct cbuffer *dest,
    u32             size,
    u32             n,
    struct cbuffer *src
);

/**
 * @fn u32 cbuffer_cbuffer_copy(struct cbuffer *dest, u32 size, u32 n, const
 * struct cbuffer *src)
 * @brief Copies items out of a source buffer and pushes them directly onto a
 * destination buffer without eviction.
 *
 * @param dest Target ring buffer destination.
 * @param size Sizing dimensions of items.
 * @param n Explicit quantity to clone.
 * @param src Source ring buffer reference container.
 * @return Total elements copied.
 */
u32 cbuffer_cbuffer_copy (
    struct cbuffer       *dest,
    u32                   size,
    u32                   n,
    const struct cbuffer *src
);

/**
 * @def cbuffer_cbuffer_move_max
 * @brief Evicts and moves all tracked active data elements safely between
 * contexts.
 */
#define cbuffer_cbuffer_move_max(dest, src) \
  cbuffer_cbuffer_move (dest, 1, cbuffer_len (src), src)

/**
 * @def cbuffer_cbuffer_copy_max
 * @brief Copies all tracked active data elements safely between contexts.
 */
#define cbuffer_cbuffer_copy_max(dest, src) \
  cbuffer_cbuffer_copy (dest, 1, cbuffer_len (src), src)

/*-----------------------------------------------------------------------------
 * SUBSECTION: IO Read / Writing
 * @brief Reading and writing to cbuffers from / to files
 *----------------------------------------------------------------------------*/

/**
 * @fn i32 cbuffer_write_to_file_1(i_file *dest, const struct cbuffer *b, u32
 * len, error *e)
 * @brief Stage-one pipeline flush writing content blocks directly to descriptor
 * files.
 *
 * @param dest Destination file handle.
 * @param b Source ring buffer containing elements.
 * @param len Exact metrics representing byte transfer lengths.
 * @param e Error reporting instance container.
 * @return Tracked metric status representing processed fields.
 */
i32 cbuffer_write_to_file_1 (
    i_file               *dest,
    const struct cbuffer *b,
    u32                   len,
    error                *e
);

/**
 * @fn err_t cbuffer_write_to_file_1_expect(i_file *dest, const struct cbuffer
 * *b, u32 len, error *e)
 * @brief Stage-one pipeline file flush asserting that errors do not populate.
 *
 * @param dest Destination file handle.
 * @param b Source ring buffer containing elements.
 * @param len Exact metrics representing byte transfer lengths.
 * @param e Error reporting instance container.
 * @return Code validation metrics verifying the operation.
 */
err_t cbuffer_write_to_file_1_expect (
    i_file               *dest,
    const struct cbuffer *b,
    u32                   len,
    error                *e
);

/**
 * @fn void cbuffer_write_to_file_2(struct cbuffer *b, u32 nwritten)
 * @brief Stage-two pipeline handler tracking written blocks and updating read
 * offsets.
 *
 * @param b Target operational buffer container.
 * @param nwritten Completed total byte outputs processed.
 */
void cbuffer_write_to_file_2 (struct cbuffer *b, u32 nwritten);

/**
 * @fn i32 cbuffer_write_to_file(i_file *dest, struct cbuffer *b, u32 len, error
 * *e)
 * @brief Consolidated write utility piping data blocks directly onto disk
 * structures.
 *
 * @param dest Target storage stream container file.
 * @param b Target operational buffer container.
 * @param len Desired scale length requested for conversion.
 * @param e Tracker catching operational framework faults.
 * @return Output indicator metrics.
 */
i32 cbuffer_write_to_file (i_file *dest, struct cbuffer *b, u32 len, error *e);

/**
 * @fn i32 cbuffer_read_from_file_1(i_file *src, const struct cbuffer *b, u32
 * len, error *e)
 * @brief Stage-one storage system call tracking input read sizes from
 * descriptor objects.
 *
 * @param src Source tracking file descriptor container.
 * @param b Target ring buffer receiving fields.
 * @param len Target processing byte boundary constraints.
 * @param e Error tracking storage.
 * @return Read verification data loops.
 */
i32 cbuffer_read_from_file_1 (
    i_file               *src,
    const struct cbuffer *b,
    u32                   len,
    error                *e
);

/**
 * @fn err_t cbuffer_read_from_file_1_expect(i_file *src, const struct cbuffer
 * *b, u32 len, error *e)
 * @brief Stage-one pipeline read checking that descriptor fetches pass
 * constraints.
 *
 * @param src Source tracking file descriptor container.
 * @param b Target ring buffer receiving fields.
 * @param len Target processing byte boundary constraints.
 * @param e Error tracking storage.
 * @return Validation context verification fields.
 */
err_t cbuffer_read_from_file_1_expect (
    i_file               *src,
    const struct cbuffer *b,
    u32                   len,
    error                *e
);

/**
 * @fn void cbuffer_read_from_file_2(struct cbuffer *b, u32 nread)
 * @brief Stage-two tracking step modifying internal write offsets after reading
 * from disk.
 *
 * @param b Target buffer managing tracking indices.
 * @param nread Concrete verified elements count extracted.
 */
void cbuffer_read_from_file_2 (struct cbuffer *b, u32 nread);

/**
 * @fn i32 cbuffer_read_from_file(i_file *src, struct cbuffer *b, u32 len, error
 * *e)
 * @brief Consolidated pipeline action piping files explicitly back into
 * operational storage pools.
 *
 * @param src Source input track file.
 * @param b Target system receiver ring buffer context.
 * @param len Limit constraints evaluating operational boundaries.
 * @param e Tracker logging environment runtime validation exceptions.
 * @return State verification markers.
 */
i32 cbuffer_read_from_file (i_file *src, struct cbuffer *b, u32 len, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Single Element Read / Write
 * @brief Writing single elements
 *----------------------------------------------------------------------------*/

/**
 * @fn bool cbuffer_get(void *dest, u32 size, u32 idx, const struct cbuffer *b)
 * @brief Indexing accessor pulling elements safely from specific index markers.
 *
 * @param dest Storage location reference target.
 * @param size Uniform dimension scaling metrics.
 * @param idx Position tracking variable target.
 * @param b Targeted buffer data repository context.
 * @return True if valid match found, false otherwise.
 */
bool cbuffer_get (void *dest, u32 size, u32 idx, const struct cbuffer *b);

/**
 * @fn bool cbuffer_push_back(const void *src, u32 size, struct cbuffer *b)
 * @brief Pushes a single item onto the trailing frame edge of the ring array.
 *
 * @param src Element item source locator.
 * @param size Byte spacing metrics.
 * @param b Target tracking context receiver.
 * @return Operational verification flag.
 */
bool cbuffer_push_back (const void *src, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_push_front(const void *src, u32 size, struct cbuffer *b)
 * @brief Pushes a single item onto the leading boundary edge of the ring array.
 *
 * @param src Element item source locator.
 * @param size Byte spacing metrics.
 * @param b Target tracking context receiver.
 * @return Operational verification flag.
 */
bool cbuffer_push_front (const void *src, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_pop_back(void *dest, u32 size, struct cbuffer *b)
 * @brief Pops an item out from the trailing frame boundary edge safely.
 *
 * @param dest Storage verification target interface pointer.
 * @param size Element footprint constraints metrics.
 * @param b Context model tracking values.
 * @return Validation confirmation indicator flags.
 */
bool cbuffer_pop_back (void *dest, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_pop_front(void *dest, u32 size, struct cbuffer *b)
 * @brief Pops an item out from the leading boundary line edge safely.
 *
 * @param dest Storage verification target interface pointer.
 * @param size Element footprint constraints metrics.
 * @param b Context model tracking values.
 * @return Validation confirmation indicator flags.
 */
bool cbuffer_pop_front (void *dest, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_peek_back(void *dest, u32 size, const struct cbuffer *b)
 * @brief Non-destructively clones contents residing at trailing array offsets.
 *
 * @param dest Output data mirror workspace.
 * @param size Explicit element allocation thresholds.
 * @param b Constant system tracking state context block.
 * @return Verification tracking output indicators.
 */
bool cbuffer_peek_back (void *dest, u32 size, const struct cbuffer *b);

/**
 * @fn bool cbuffer_peek_front(void *dest, u32 size, const struct cbuffer *b)
 * @brief Non-destructively clones contents residing at leading head pointer
 * locations.
 *
 * @param dest Output data mirror workspace.
 * @param size Explicit element allocation thresholds.
 * @param b Constant system tracking state context block.
 * @return Verification tracking output indicators.
 */
bool cbuffer_peek_front (void *dest, u32 size, const struct cbuffer *b);

/**
 * @def cbuffer_push_back_expect
 * @brief Pushes an item to the trailing boundary edge - ASSERTs if full.
 */
#define cbuffer_push_back_expect(src, size, b)     \
  do                                               \
  {                                                \
    bool __ret = cbuffer_push_back (src, size, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/**
 * @def cbuffer_push_front_expect
 * @brief Pushes an item to the leading boundary edge - ASSERTs if full.
 */
#define cbuffer_push_front_expect(src, size, b)     \
  do                                                \
  {                                                 \
    bool __ret = cbuffer_push_front (src, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/**
 * @def cbuffer_pop_back_expect
 * @brief Pops an item from the trailing boundary edge - ASSERTs if empty.
 */
#define cbuffer_pop_back_expect(dest, size, b)     \
  do                                               \
  {                                                \
    bool __ret = cbuffer_pop_back (dest, size, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/**
 * @def cbuffer_pop_front_expect
 * @brief Pops an item from the leading boundary edge - ASSERTs if empty.
 */
#define cbuffer_pop_front_expect(dest, size, b)     \
  do                                                \
  {                                                 \
    bool __ret = cbuffer_pop_front (dest, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/**
 * @def cbuffer_peek_back_expect
 * @brief Peeks at the trailing boundary edge - ASSERTs if empty.
 */
#define cbuffer_peek_back_expect(dest, size, b)     \
  do                                                \
  {                                                 \
    bool __ret = cbuffer_peek_back (dest, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/**
 * @def cbuffer_peek_front_expect
 * @brief Peeks at the leading boundary edge - ASSERTs if empty.
 */
#define cbuffer_peek_front_expect(dest, size, b)     \
  do                                                 \
  {                                                  \
    bool __ret = cbuffer_peek_front (dest, size, b); \
    ASSERT (__ret);                                  \
  }                                                  \
  while (0)

/**
 * @def cbuffer_pushb_back_expect
 * @brief Explicitly pushes a single raw 8-bit byte value onto the back of the
 * buffer.
 */
#define cbuffer_pushb_back_expect(src, b)         \
  do                                              \
  {                                               \
    u8   _src  = src;                             \
    bool __ret = cbuffer_push_back (&_src, 1, b); \
    ASSERT (__ret);                               \
  }                                               \
  while (0)

/**
 * @def cbuffer_pushb_front_expect
 * @brief Explicitly pushes a single raw 8-bit byte value onto the front of the
 * buffer.
 */
#define cbuffer_pushb_front_expect(src, b)         \
  do                                               \
  {                                                \
    u8   _src  = src;                              \
    bool __ret = cbuffer_push_front (&_src, 1, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/******************************************************************************
 * SECTION: Byte Accessor
 * ----------------------------------------------------------------------------
 * @brief A way to copy offset and strided data from one place to another
 *
 * The motivation for a byte accessor comes from the numstore type system.
 *
 * Consider a (packed) struct:
 *
 * struct foo {
 *    u32 a;
 *    u64 b;
 *    f32 c[20];
 * };
 *
 * Lets say I have an instance of foo and I want
 * to read into a byte buffer:
 *
 * [ foo.a, foo.a, foo.b, foo.b, foo.a ]
 *
 * (Yes, foo.a foo.b are all redundant duplicate data)
 *
 * Byte accessor defines this memcopy as:
 *
 * @code
 * [
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 *    foo.b -- SELECT(src_size = 8, dest_size = 8, bofst = 4, sub_ba = TAKE)
 *    foo.b -- SELECT(src_size = 8, dest_size = 8, bofst = 4, sub_ba = TAKE)
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 * ]
 * @endcode
 *
 * TODO - this example could be better
 ******************************************************************************/

struct byte_accessor
{
  enum ta_type
  {
    TA_TAKE,
    TA_SELECT,
    TA_RANGE,
  } type;

  u32 src_size;  // total size this ba takes up on source
  u32 dest_size; // total size this ba puts into dest

  union {
    struct select_ba
    {
      u32                   bofst;  // Offset in bytes
      struct byte_accessor *sub_ba; // Next accessor
    } select;

    struct range_ba
    {
      struct stride         stride; // Stride on src
      struct byte_accessor *sub_ba; // For each stride, the next ba
    } range;
  };
};

u32 ba_memcpy_from (u8 *dest, const u8 *src, struct byte_accessor *acc);
u32 ba_memcpy_to (u8 *dest, const u8 *src, struct byte_accessor *acc);

#endif // COLLECTIONS_H
