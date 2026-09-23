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
 * @brief Internals of numstore user exposed type
 */

#ifndef NS_NUMSTORE_ALGORITHMS_H
#define NS_NUMSTORE_ALGORITHMS_H

#include "core/ns_error.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/variables/ns_variables.h"

err_t numstore_init_pager (struct pager *p, error *e);

sb_size numstore_insert (
    struct pager    *p,
    struct ns_txn   *tx,
    struct variable *var,
    b_size           ofst,
    b_size           len,
    struct stream   *src,
    error           *e
);

sb_size numstore_read (
    struct pager      *p,
    struct ns_txn     *tx,
    struct variable   *var,
    struct user_stride ustr,
    struct stream     *dest,
    error             *e
);

void *numstore_read_malloc (
    struct pager      *p,
    struct ns_txn     *tx,
    struct variable   *var,
    struct user_stride ustr,
    b_size            *dlen,
    struct i_mem       mem,
    error             *e
);

sb_size numstore_write (
    struct pager      *p,
    struct ns_txn     *tx,
    struct variable   *var,
    struct user_stride ustr,
    struct stream     *src,
    error             *e
);

sb_size numstore_remove (
    struct pager      *p,
    struct ns_txn     *tx,
    struct variable   *var,
    struct user_stride ustr,
    struct stream     *dest,
    error             *e
);

void *numstore_remove_malloc (
    struct pager      *p,
    struct ns_txn     *tx,
    struct variable   *var,
    struct user_stride ustr,
    b_size            *dlen,
    struct i_mem       mem,
    error             *e
);

err_t numstore_get (
    struct pager       *p,
    struct ns_txn      *tx,
    bool                if_exists,
    struct string       name,
    struct arena_alloc *alloc,
    struct variable    *var,
    error              *e
);

err_t numstore_delete (
    struct pager  *p,
    struct ns_txn *tx,
    struct string  name,
    bool           if_exists,
    error         *e
);

err_t numstore_create (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,
    struct type         type,
    struct arena_alloc *valloc,
    struct variable    *var,
    error              *e
);

#endif
