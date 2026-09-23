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

#ifndef NS_NSDBVAR_H
#define NS_NSDBVAR_H

#include "core/ns_error.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"

#include <stdbool.h>
#include <stddef.h>

struct numstore_var
{
  struct variable    var;
  struct arena_alloc alloc;
  struct i_mem       mem;
};

struct numstore_var *nsdb_var_create (struct i_mem mem, error *e);
void nsdb_var_free (struct numstore_var *var);

#endif
