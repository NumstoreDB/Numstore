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

#ifndef NS_EXECUTE_H
#define NS_EXECUTE_H

#include "core/ns_alloc.h"
#include "core/ns_dbl_buffer.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_query.h"
#include "nscore/variables/ns_variables.h"

sb_size nsdb_execute_on_buffer (
    struct nsdb      *ns,
    struct ns_txn    *txn,
    struct query     *q,
    void             *data,
    struct allocator *alc
);

/******************************************************************************
 * SECTION: Query literal routines
 * ----------------------------------------------------------------------------
 * @brief Individual actions you can take on a numstore database
 ******************************************************************************/

// Get a variable and print it to the console
err_t nsdb_get_and_print (struct nsdb *db, struct get_query *query, struct allocator *alloc);

// Read data from a variable and print it to the console
sb_size nsdb_read_and_print (struct nsdb *db, struct read_query *query, struct allocator *alloc);

err_t nsdb_execute_in_console (struct nsdb *ns, struct query *q, struct allocator *alc);

#endif
