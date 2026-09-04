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

#ifndef VARIABLES_H
#define VARIABLES_H

#include "core/ns_error.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "nscore/types/ns_types.h"

#include <stdbool.h>
#include <stddef.h>

#define DEFAULT_VARIABLE    "."
#define MAX_VARIABLE_LENGTH 4096

struct variable
{
  struct string vname;
  struct type  *dtype;
  pgno          var_root;
  pgno          rpt_root;
  b_size        nbytes;
};

struct nsdb_var
{
  struct variable  *var;
  struct allocator *alloc;
};

err_t i_print_variable (struct variable *v, error *e);

// If name is NULL - returns default variable name
struct string vname_or_default (const char *name);
bool variable_equal (const struct variable *left, const struct variable *right);
err_t validate_vname (struct string vname, error *e);
void var_random_name (char *buffer, u32 length);
err_t rand_varname (struct string *dest, struct allocator *alloc, u32 minlen, u32 maxlen, error *e);
err_t rand_varname_same_hash (
    struct string    *name1,
    struct string    *name2,
    struct allocator *alloc,
    error            *e
);
err_t rand_varname_different_hash (
    struct string    *name1,
    struct string    *name2,
    struct allocator *alloc,
    error            *e
);
err_t variable_copy (
    struct variable       *dest,
    const struct variable *src,
    struct allocator      *alloc,
    error                 *e
);
b_size var_resolve_index (struct variable *v, sb_size bofst);
b_size var_resolve_nelem (struct variable *v, b_size bofst, b_size nelem, t_size size);

#endif // VARIABLES_H
