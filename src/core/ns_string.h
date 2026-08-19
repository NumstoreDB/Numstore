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

/**
 * @file
 * @brief Functions and data structures on bytes or strings
 */

#ifndef STRING_H
#define STRING_H

#include <stdbool.h>

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_memory.h"

/******************************************************************************
 * SECTION: String
 * ----------------------------------------------------------------------------
 * @brief A sized constant string
 *
 * Avoids the heavy usage of c - strings
 ******************************************************************************/

struct string
{
  u32         len;
  const char *data;
};

struct string        strfcstr (const char *cstr);
u64                  line_length (const char *buf, u64 max);
int                  strings_all_unique (const struct string *strs, u32 count);
bool                 string_equal (const struct string s1, const struct string s2);
const struct string *strings_are_disjoint (
    const struct string *left,
    u32                  llen,
    const struct string *right,
    u32                  rlen
);
bool  string_contains (const struct string superset, const struct string subset);
bool  string_less_string (const struct string left, const struct string right);
bool  string_greater_string (const struct string left, const struct string right);
bool  string_less_equal_string (const struct string left, const struct string right);
bool  string_greater_equal_string (const struct string left, const struct string right);
err_t string_copy (struct string *dest, struct string src, struct i_mem mem, error *e);
#define strfmt(str) (str)->len, (str)->data

#endif
