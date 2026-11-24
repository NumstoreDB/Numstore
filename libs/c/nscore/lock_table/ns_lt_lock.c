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

#include "lock_table/ns_lt_lock.h"

#include "ns_csx_assert.h"
#include "ns_htable.h"

#include <string.h>

u32
lt_lock_key (const struct lt_lock lock)
{
  char     hcode[sizeof (union lt_lock_data) + sizeof (u8)];
  u32      hcodelen = 0;
  const u8 _type    = lock.type;

  hcodelen += memcpy_inc (&hcode[hcodelen], &_type, sizeof (_type));

  switch (lock.type) {
    case LOCK_DB:
    case LOCK_VHMAP: {
      break;
    }
    case LOCK_VAR_PAGE: {
      pgno pg = lock.data.pg;
      hcodelen += memcpy_inc (&hcode[hcodelen], &pg, sizeof (pg));
      break;
    }
  }

  const struct string lock_type_hcode = {
      .data = hcode,
      .len  = hcodelen,
  };

  return fnv1a_hash (lock_type_hcode);
}

/**
 * Get the parent lock (heirarchial locking)
 */
bool
get_parent (struct lt_lock *parent, const struct lt_lock lock)
{
  switch (lock.type) {
    case LOCK_DB: {
      return false;
    }
    case LOCK_VHMAP: {
      *parent = lock_db ();
      return true;
    }
    case LOCK_VAR_PAGE: {
      *parent = lock_vhmap ();
      return true;
    }
  }

  UNREACHABLE ();
}

bool
lt_lock_equal (const struct lt_lock left, const struct lt_lock right)
{
  if (left.type != right.type) {
    return false;
  }

  switch (left.type) {
    case LOCK_DB:
    case LOCK_VHMAP: {
      return true;
    }
    case LOCK_VAR_PAGE: {
      return left.data.pg == right.data.pg;
    }
  }
  UNREACHABLE ();
}
