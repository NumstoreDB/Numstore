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

#ifndef NS_NUMSTORE_INTERNAL_H
#define NS_NUMSTORE_INTERNAL_H

#include "core/ns_error.h"
#include "numstore/numstore.h"

HEADER_FUNC bool
numstore_plan_has_option (const struct numstore_plan *plan, numstore_plan_opt_t opt)
{
  return (plan->options & opt) != 0;
}

HEADER_FUNC err_t
numstore_plan_validate (const struct numstore_plan *plan, error *e)
{
  if (numstore_plan_has_option (plan, NSDB_PLAN_OPT_ALLOCATE_DATA)) {
    if (plan->data != NULL || plan->dlen != 0) {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "data/dlen must be NULL/0 when allocate-data mode is enabled"
      );
    }
  }

  if (numstore_plan_has_option (plan, NSDB_PLAN_OPT_CAPTURE_VAR)) {
    if (plan->var != NULL) {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "var must be NULL when capture-var mode is enabled"
      );
    }
  }

  return SUCCESS;
}

#endif
