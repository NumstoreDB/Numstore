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

#pragma once

#include "c_specx.h"

#define FAKE_DB_MAX_VARS 64

struct fake_var
{
  char               *name;
  char               *typestr;
  u32                 elem_size;
  struct block_array *data;
};

struct fake_database
{
  struct fake_var vars[FAKE_DB_MAX_VARS];
  int             n;
};

// Lifecycle
struct fake_database *fake_db_create (void);
void                  fake_db_free (struct fake_database *db);
struct fake_database *fake_db_clone (const struct fake_database *src);

// Main Api
int              fake_db_var_count (const struct fake_database *db);
struct fake_var *fake_db_var_at (struct fake_database *db, int idx);
struct fake_var *fake_db_find (struct fake_database *db, const char *name);
int fake_db_add_var (struct fake_database *db, char *name, char *typestr, u32 elem_size);
int fake_db_remove_var (struct fake_database *db, const char *name);
