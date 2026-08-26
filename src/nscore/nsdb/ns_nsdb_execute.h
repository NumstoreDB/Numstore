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
#include "core/ns_string.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_variables.h"

// Execute Internal
sb_size nsdb_execute_on_buffer (
    struct nsdb      *ns,
    struct query     *q,
    void             *data,
    struct allocator *alc
);

/******************************************************************************
 * SECTION: Query literal routines
 * ----------------------------------------------------------------------------
 * @brief Individual actions you can take on a numstore database
 ******************************************************************************/

int nsdb_create (struct nsdb *db, struct allocator *alloc, struct string vname, struct type dtype);
err_t nsdb_delete (struct nsdb *db, struct delete_query *query);
err_t nsdb_get (
    struct nsdb      *db,
    struct get_query *query,
    struct allocator *alloc,
    struct variable **dest
);
err_t nsdb_get_and_print (struct nsdb *db, struct get_query *query, struct allocator *alloc);
sb_size nsdb_insert (
    struct nsdb         *db,
    struct insert_query *query,
    struct allocator    *alloc,
    struct stream       *src
);
sb_size nsdb_read (
    struct nsdb       *db,
    struct read_query *query,
    struct allocator  *alloc,
    struct stream     *dest
);
sb_size nsdb_read_and_print (struct nsdb *db, struct read_query *query, struct allocator *alloc);
sb_size nsdb_write (
    struct nsdb        *db,
    struct write_query *query,
    struct allocator   *alloc,
    struct stream      *src
);
sb_size nsdb_remove (
    struct nsdb         *db,
    struct remove_query *query,
    struct allocator    *alloc,
    struct stream       *dest
);

#endif
