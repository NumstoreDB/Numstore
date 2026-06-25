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

#include "types.h"

/******************************************************************************
 * SECTION: Variables
 * ----------------------------------------------------------------------------
 * @brief In Memory representation of a variable
 ******************************************************************************/

/**
 * @struct variable
 * @brief The main Representation of a variable inside Numstore
 *
 * @var variable::vname
 * @brief The variable name as a sized string
 *
 * @var variable::dtype
 * @brief A pointer to the variable data type
 *        allocation is agnostic
 *
 * @var variable::var_root
 * @brief The root page for the variable - this remains
 *        constant throughout the variable's lifecycle
 *
 * @param variable::rpt_root
 * @brief The root page for the variable data - this changes
 *        constantly during rebalances
 *
 * @var variable::nbytes
 * @brief The number of bytes in a variable
 */
struct variable
{
  struct string vname;
  struct type  *dtype;
  pgno          var_root;
  pgno          rpt_root;
  b_size        nbytes;
};

err_t i_print_variable (struct variable *v, error *e);

/**
 * @def DEFAULT_VARIABLE
 * @brief The default variable name for application level libraries
 *
 * Certain libraries (E.g. smartfiles) use a default variable - this
 * is the name of the default variable
 */
#define DEFAULT_VARIABLE "."

/**
 * @fn string vname_or_default(const char *name)
 * @brief Fetch a string representing the variable name in [name]
 * or else
 *
 * @param name
 */
static inline struct string
vname_or_default (const char *name)
{
  if (name != NULL)
  {
    return strfcstr (name);
  }
  else
  {
    return strfcstr (DEFAULT_VARIABLE);
  }
}

struct nsdb_var
{
  struct variable    *var;
  struct chunk_alloc *alloc;
};
bool variable_equal (const struct variable *left, const struct variable *right);
err_t validate_vname (struct string vname, error *e);
void  var_random_name (char *buffer, int length);
err_t rand_varname (
    struct string      *dest,
    struct chunk_alloc *alloc,
    const u32           minlen,
    const u32           maxlen,
    error              *e
);
err_t rand_varname_same_hash (
    struct string      *name1,
    struct string      *name2,
    struct chunk_alloc *alloc,
    error              *e
);
err_t rand_varname_different_hash (
    struct string      *name1,
    struct string      *name2,
    struct chunk_alloc *alloc,
    error              *e
);
struct variable *
variable_malloc_copy (struct variable *v, struct malloc_plan *plan);

HEADER_FUNC b_size
var_resolve_index (struct variable *v, sb_size bofst)
{
  // Translate negative
  if (bofst < 0)
  {
    bofst = v->nbytes + bofst;
  }

  // was so negative it's still negative after conversion
  if (bofst < 0)
  {
    bofst = 0;
  }

  // Translate indexes past nybtes
  if ((b_size)bofst
      > v->nbytes) // also: > not >=, so nbytes itself is valid (append)
  {
    bofst = v->nbytes;
  }

  return bofst;
}

HEADER_FUNC b_size
var_resolve_nelem (struct variable *v, b_size bofst, b_size nelem, t_size size)
{
  b_size remainder = (v->nbytes - bofst) / size;
  if (nelem > remainder)
  {
    nelem = remainder;
  }
  return nelem;
}
#endif // VARIABLES_H
