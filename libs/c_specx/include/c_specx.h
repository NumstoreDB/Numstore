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

#ifndef C_SPECX_H
#define C_SPECX_H

// Platform detection, compiler attributes, system includes
#include <c_specx/platform.h>

// Primitive type aliases (u8, u32, i64, f32, etc.)
#include <c_specx/stdtypes.h>

// Simple utility macros (FPREFIX, container_of, arrlen, MIN, MAX, etc.)
#include <c_specx/utils.h>

// Error type, error codes, WRAP macros
#include <c_specx/error.h>

// Threading primitives (i_mutex, i_semaphore, i_rwlock, i_thread, i_cond)
#include <c_specx/threading.h>

// Timer and sleep utilities
#include <c_specx/time.h>

// Socket types and operations
#include <c_specx/socket.h>

// Byte buffer view types (struct bytes, struct cbytes)
#include <c_specx/bytes.h>

// File system vtable and I/O helpers
#include <c_specx/file_system.h>

// Virtual memory vtable (i_vmem)
#include <c_specx/memory.h>

// Logging macros, color codes, ASSERT, UNREACHABLE, panic
#include <c_specx/logging.h>

// Spinlock latch (latch type, latch_lock/unlock/trylock)
#include <c_specx/latch.h>

// Shared/exclusive spinlock (sx_latch)
#include <c_specx/spx_latch.h>

// Linear bump allocator (struct lalloc)
#include <c_specx/lalloc.h>

// Slab allocator (struct slab_alloc)
#include <c_specx/slab_alloc.h>

// Chunk-based arena allocator (struct chunk_alloc)
#include <c_specx/chunk_alloc.h>

// Serializer (struct serializer)
#include <c_specx/serializer.h>

// Deserializer (struct deserializer)
#include <c_specx/deserializer.h>

// Malloc plan (struct malloc_plan)
#include <c_specx/malloc_plan.h>

// Generic allocator wrapper (struct alloc)
#include <c_specx/alloc.h>

// Intrusive linked list (struct llnode)
#include <c_specx/llist.h>

// Length-prefixed string view (struct string)
#include <c_specx/string.h>

// Stride descriptors (struct stride, struct user_stride, struct multi_user_stride)
#include <c_specx/stride.h>

// Data writer vtable (struct data_writer)
#include <c_specx/data_writer.h>

// Hash table result enums (hti_res, hta_res)
#include <c_specx/ht_models.h>

// Block array (struct block_array)
#include <c_specx/block_array.h>

// Circular buffer (struct cbuffer)
#include <c_specx/cbuffer.h>

// Doubling buffer (struct dbl_buffer)
#include <c_specx/dbl_buffer.h>

// Extensible array (struct ext_array)
#include <c_specx/ext_array.h>

// Chained hash table (struct htable, struct hnode)
#include <c_specx/hash_table.h>

// Polymorphic byte-oriented I/O stream (struct stream)
#include <c_specx/stream.h>

// Byte accessor (struct byte_accessor)
#include <c_specx/byte_accessor.h>

// Safe arithmetic macros (safe_add_u32, safe_sub_u64, etc.)
#include <c_specx/bounds.h>

// CRC / checksum utilities
#include <c_specx/checksums.h>

// FNV-1a hashing
#include <c_specx/hashing.h>

// Max capture placeholder
#include <c_specx/max_capture.h>

// Number parsing utilities
#include <c_specx/numbers.h>

// Random number generation
#include <c_specx/random.h>

// Math helpers and array utilities
#include <c_specx/math.h>

// Unit test framework macros
#include <c_specx/testing.h>

// Data validator (testing utility)
#include <c_specx/data_validator.h>

// Granular lock (struct gr_lock)
#include <c_specx/gr_lock.h>

// Periodic background task (struct periodic_task)
#include <c_specx/periodic_task.h>

// TCP client (struct client)
#include <c_specx/client.h>

// Polling server (struct polling_server)
#include <c_specx/polling_server.h>

// Echo server
#include <c_specx/echo_server.h>

#endif // C_SPECX_H

////////////////////////////////////////////////////////////
// Robin Hood Hash Table
// Key-Value variant: define KTYPE, VTYPE, SUFFIX before re-including

#ifdef KTYPE

#include <string.h>

#ifndef KTYPE
#  define KTYPE int
#endif
#ifndef VTYPE
#  define VTYPE int
#endif
#ifndef SUFFIX
#  define SUFFIX int
#endif

#define HDATA_T      RH__XCAT (hdata_, SUFFIX)
#define HENWRAP_T    RH__XCAT (hentry_, SUFFIX)
#define HTIR_T       RH__XCAT (htir_res_, SUFFIX)
#define HT_T         RH__XCAT (ht_, SUFFIX)
#define HASH_TABLE_T RH__XCAT (hash_table_, SUFFIX)

// Functions
#define HT_INIT          RH__XCAT (ht_init_, SUFFIX)
#define HT_INSERT        RH__XCAT (ht_insert_, SUFFIX)
#define HT_INSERT_EXPECT RH__XCAT (ht_insert_expect_, SUFFIX)
#define HT_GET           RH__XCAT (ht_get_, SUFFIX)
#define HT_GET_EXPECT    RH__XCAT (ht_get_expect_, SUFFIX)
#define HT_DELETE        RH__XCAT (ht_delete_, SUFFIX)
#define HT_COUNT         RH__XCAT (ht_count_, SUFFIX)
#define HT_DELETE_EXPECT RH__XCAT (ht_delete_expect_, SUFFIX)

typedef struct
{
  KTYPE key; // Hash key
  VTYPE value;
} HDATA_T;

typedef struct
{
  HDATA_T data;    // The data we store
  KTYPE   dib;     // Distance from initial bucket
  bool    present; // Exists or not
} HENWRAP_T;

typedef struct
{
  u32        cap;
  HENWRAP_T *elems;
  latch      l;
} HASH_TABLE_T;

HEADER_FUNC void
HT_INIT (HASH_TABLE_T *dest, HENWRAP_T *arr, const u32 nelem)
{
  ASSERT (dest);
  ASSERT (arr);

  memset (arr, 0, nelem * sizeof *arr);
  dest->elems = arr;
  dest->cap   = nelem;
  latch_init (&dest->l);
}

HEADER_FUNC hti_res
HT_INSERT (HASH_TABLE_T *ht, HDATA_T data)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  KTYPE   dibn = 0; // Current distance from initial bucket
  hti_res ret  = HTIR_FULL;

  latch_lock (&ht->l);

  for (KTYPE i = 0; i < (KTYPE)ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    KTYPE _i = (data.key + dibn) % (KTYPE)ht->cap;

    // If not present, insert
    if (!ht->elems[_i].present)
    {
      ht->elems[_i].data    = data;
      ht->elems[_i].dib     = dibn;
      ht->elems[_i].present = true;
      ret                   = HTIR_SUCCESS;
      goto theend;
    }

    // Swap (lt means dib != dibn, therefore key != key)
    if (ht->elems[_i].dib < dibn)
    {
      HDATA_T temp_data = ht->elems[_i].data;
      KTYPE   temp_dib  = ht->elems[_i].dib;

      ht->elems[_i].data = data;
      ht->elems[_i].dib  = dibn;

      dibn = temp_dib;
      data = temp_data;
    }

    // Compare keys for duplicates
    if (ht->elems[_i].data.key == data.key)
    {
      ret = HTIR_EXISTS;
      goto theend;
    }
  }

theend:
  latch_unlock (&ht->l);
  return ret;
}

HEADER_FUNC void
HT_INSERT_EXPECT (HASH_TABLE_T *ht, HDATA_T data)
{
  const hti_res ret = HT_INSERT (ht, data);
  ASSERT (ret == HTIR_SUCCESS);
}

HEADER_FUNC hta_res
HT_GET (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  KTYPE dibn = 0;

  hta_res ret = HTAR_DOESNT_EXIST;

  latch_lock (&ht->l);

  for (KTYPE i = 0; i < (KTYPE)ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    KTYPE _i = (key + i) % (KTYPE)ht->cap;

    // If not present, return
    if (!ht->elems[_i].present)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Check for key
    if (ht->elems[_i].data.key == key)
    {
      if (dest)
      {
        *dest = (HDATA_T){
            .value = ht->elems[_i].data.value,
            .key   = ht->elems[_i].data.key,
        };
      }
      ret = HTAR_SUCCESS;
      goto theend;
    }
  }

theend:
  latch_unlock (&ht->l);
  return ret;
}

HEADER_FUNC void
HT_GET_EXPECT (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  const hta_res ret = HT_GET (ht, dest, key);
  ASSERT (ret == HTAR_SUCCESS);
}

HEADER_FUNC hta_res
HT_DELETE (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  KTYPE dibn = 0;
  KTYPE i    = 0;

  hta_res ret = HTAR_DOESNT_EXIST;

  latch_lock (&ht->l);

  for (i = 0; i < (KTYPE)ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    KTYPE _i = (key + i) % (KTYPE)ht->cap;

    // If not present, return
    if (!ht->elems[_i].present)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Check for key
    if (ht->elems[_i].data.key == key)
    {
      if (dest) { *dest = ht->elems[_i].data; }
      break;
    }
  }

  // Shift all full elements to the left
  for (; i < (KTYPE)ht->cap; ++i)
  {
    // Mapped index after probing
    KTYPE hole = (key + i) % (KTYPE)ht->cap;
    KTYPE next = (key + i + 1) % (KTYPE)ht->cap;

    // Right value is empty, set this to empty and return
    if (!ht->elems[next].present || ht->elems[next].dib == 0)
    {
      ht->elems[hole].present = false;
      ret                     = HTAR_SUCCESS;
      goto theend;
    }

    // Shift left
    ht->elems[hole].data = ht->elems[next].data;
    ASSERT (ht->elems[next].dib > 0);
    ht->elems[hole].dib = ht->elems[next].dib - 1;
    ASSERT (ht->elems[hole].present);
  }

theend:
  latch_unlock (&ht->l);
  return ret;
}

HEADER_FUNC u32
HT_COUNT (HASH_TABLE_T *ht)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 ret = 0;

  latch_lock (&ht->l);

  for (u32 i = 0; i < ht->cap; ++i)
  {
    if (ht->elems[i].present) { ret++; }
  }

  latch_unlock (&ht->l);

  return ret;
}

HEADER_FUNC void
HT_DELETE_EXPECT (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  const hta_res ret = HT_DELETE (ht, dest, key);
  ASSERT (ret == HTAR_SUCCESS);
}

#undef HDATA_T
#undef HENWRAP_T
#undef HTIR_T
#undef HT_T
#undef HASH_TABLE_T
#undef HT_INIT
#undef HT_INSERT
#undef HT_INSERT_EXPECT
#undef HT_GET
#undef HT_GET_EXPECT
#undef HT_DELETE
#undef HT_COUNT
#undef HT_DELETE_EXPECT

#endif // KTYPE

////////////////////////////////////////////////////////////
// TEMPLATE: ROBIN HOOD HASH TABLE
// Define HASH_FUNC, CMP_FUNC, VTYPE, SUFFIX before re-including
////////////////////////////////////////////////////////////
#ifdef HASH_FUNC

#include <string.h>

#ifndef VTYPE
#  define VTYPE int
#endif
#ifndef SUFFIX
#  define SUFFIX int
#endif
#ifndef HASH_FUNC
#  define HASH_FUNC(v) ((u32)(v))
#endif
#ifndef CMP_FUNC
#  define CMP_FUNC(a, b) ((a) == (b))
#endif

#define HENWRAP_T    RH__XCAT (hentry_, SUFFIX)
#define HASH_TABLE_T RH__XCAT (hash_table_, SUFFIX)

#define HT_INIT          RH__XCAT (ht_init_, SUFFIX)
#define HT_INSERT        RH__XCAT (ht_insert_, SUFFIX)
#define HT_INSERT_EXPECT RH__XCAT (ht_insert_expect_, SUFFIX)
#define HT_GET           RH__XCAT (ht_get_, SUFFIX)
#define HT_GET_EXPECT    RH__XCAT (ht_get_expect_, SUFFIX)
#define HT_DELETE        RH__XCAT (ht_delete_, SUFFIX)
#define HT_DELETE_EXPECT RH__XCAT (ht_delete_expect_, SUFFIX)

typedef struct
{
  VTYPE value;   // The value we store
  u32   hash;    // Hash of the value
  u32   dib;     // Distance from initial bucket
  bool  present; // Exists or not
} HENWRAP_T;

typedef struct
{
  u32        cap;
  HENWRAP_T *elems;
} HASH_TABLE_T;

HEADER_FUNC void
HT_INIT (HASH_TABLE_T *dest, HENWRAP_T *arr, u32 nelem)
{
  ASSERT (dest);
  ASSERT (arr);

  memset (arr, 0, nelem * sizeof *arr);
  dest->elems = arr;
  dest->cap   = nelem;
}

HEADER_FUNC hti_res
HT_INSERT (HASH_TABLE_T *ht, VTYPE value)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 hash = HASH_FUNC (value);
  u32 dibn = 0; // Current distance from initial bucket

  for (u32 i = 0; i < ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    u32 _i = (hash + dibn) % ht->cap;

    // If not present, insert
    if (!ht->elems[_i].present)
    {
      ht->elems[_i].value   = value;
      ht->elems[_i].hash    = hash;
      ht->elems[_i].dib     = dibn;
      ht->elems[_i].present = true;
      return HTIR_SUCCESS;
    }

    // Swap (lt means dib != dibn, therefore different
    // values)
    if (ht->elems[_i].dib < dibn)
    {
      VTYPE temp_value = ht->elems[_i].value;
      u32   temp_hash  = ht->elems[_i].hash;
      u32   temp_dib   = ht->elems[_i].dib;

      ht->elems[_i].value = value;
      ht->elems[_i].hash  = hash;
      ht->elems[_i].dib   = dibn;

      dibn  = temp_dib;
      value = temp_value;
      hash  = temp_hash;
    }

    // Compare values for duplicates
    if (ht->elems[_i].hash == hash && CMP_FUNC (ht->elems[_i].value, value)) { return HTIR_EXISTS; }
  }

  return HTIR_FULL;
}

HEADER_FUNC void
HT_INSERT_EXPECT (HASH_TABLE_T *ht, VTYPE value)
{
  hti_res ret = HT_INSERT (ht, value);
  ASSERT (ret == HTIR_SUCCESS);
}

HEADER_FUNC hta_res
HT_GET (const HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 hash = HASH_FUNC (value);
  u32 dibn = 0;

  for (u32 i = 0; i < ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    u32 _i = (hash + i) % ht->cap;

    // If not present, return
    if (!ht->elems[_i].present) { return HTAR_DOESNT_EXIST; }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn) { return HTAR_DOESNT_EXIST; }

    // Check for value match
    if (ht->elems[_i].hash == hash && CMP_FUNC (ht->elems[_i].value, value))
    {
      if (dest) { *dest = ht->elems[_i].value; }
      return HTAR_SUCCESS;
    }
  }

  return HTAR_DOESNT_EXIST;
}

HEADER_FUNC void
HT_GET_EXPECT (const HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  hta_res ret = HT_GET (ht, dest, value);
  ASSERT (ret == HTAR_SUCCESS);
}

HEADER_FUNC hta_res
HT_DELETE (HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 hash = HASH_FUNC (value);
  u32 dibn = 0;
  u32 i    = 0;

  for (i = 0; i < ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    u32 _i = (hash + i) % ht->cap;

    // If not present, return
    if (!ht->elems[_i].present) { return HTAR_DOESNT_EXIST; }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn) { return HTAR_DOESNT_EXIST; }

    // Check for value match
    if (ht->elems[_i].hash == hash && CMP_FUNC (ht->elems[_i].value, value))
    {
      if (dest) { *dest = ht->elems[_i].value; }
      break;
    }
  }

  // Shift all full elements to the left
  for (; i < ht->cap; ++i)
  {
    // Mapped index after probing
    u32 hole = (hash + i) % ht->cap;
    u32 next = (hash + i + 1) % ht->cap;

    // Right value is empty, set this to empty and return
    if (!ht->elems[next].present || ht->elems[next].dib == 0)
    {
      ht->elems[hole].present = false;
      return HTAR_SUCCESS;
    }

    // Shift left
    ht->elems[hole].value = ht->elems[next].value;
    ht->elems[hole].hash  = ht->elems[next].hash;
    ASSERT (ht->elems[next].dib > 0);
    ht->elems[hole].dib = ht->elems[next].dib - 1;
    ASSERT (ht->elems[hole].present);
  }

  return HTAR_DOESNT_EXIST;
}

HEADER_FUNC void
HT_DELETE_EXPECT (HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  hta_res ret = HT_DELETE (ht, dest, value);
  ASSERT (ret == HTAR_SUCCESS);
}

#undef HENWRAP_T
#undef HASH_TABLE_T
#undef HT_INIT
#undef HT_INSERT
#undef HT_INSERT_EXPECT
#undef HT_GET
#undef HT_GET_EXPECT
#undef HT_DELETE
#undef HT_DELETE_EXPECT
#undef VTYPE
#undef SUFFIX
#undef HASH_FUNC
#undef CMP_FUNC

#endif // HASH_FUNC
