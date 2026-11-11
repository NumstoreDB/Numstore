/// Copyright 2026 Theo Linckelock_ta
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

#ifndef NS_LT_LOCK_H
#define NS_LT_LOCK_H

#include "ns_platform.h"
#include "ns_stdtypes.h"

#include <stdbool.h>

struct lt_lock
{
  enum lt_lock_type
  {
    LOCK_DB,
    LOCK_VHMAP,    // -> LOCK_DB
    LOCK_VAR_PAGE, // -> LOCK_VHMAP
  } type;

  union lt_lock_data {
    pgno pg;
  } data;
};

u32 lt_lock_key (struct lt_lock lock);
bool lt_lock_equal (struct lt_lock left, struct lt_lock right);
bool get_parent (struct lt_lock *parent, struct lt_lock lock);

///////////////////// Constructors

HEADER_FUNC struct lt_lock
lock_create (const enum lt_lock_type type, const union lt_lock_data data)
{
  return (struct lt_lock){
      .type = type,
      .data = data,
  };
}

HEADER_FUNC struct lt_lock
lock_db (void)
{
  return lock_create (LOCK_DB, (union lt_lock_data){0});
}

HEADER_FUNC struct lt_lock
lock_vhmap ()
{
  return lock_create (LOCK_VHMAP, (union lt_lock_data){0});
}

HEADER_FUNC struct lt_lock
lock_var_page (pgno pg)
{
  return lock_create (LOCK_VAR_PAGE, (union lt_lock_data){.pg = pg});
}

#endif
