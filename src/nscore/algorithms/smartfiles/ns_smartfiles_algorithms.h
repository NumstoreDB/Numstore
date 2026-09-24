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

#ifndef NS_SMARTFILES_ALGORITHMS_H
#define NS_SMARTFILES_ALGORITHMS_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stream.h"
#include "nscore/pager/ns_pager.h"

err_t smartfiles_init_pager (struct pager *p, error *e);

sb_size smartfiles_size (struct pager *p, struct txn *tx, struct arena_alloc *alloc, error *e);

sb_size smartfiles_insert (
    struct pager       *p,
    struct txn         *tx,
    struct stream      *src,
    sb_size             bofst,
    b_size              slen,
    struct arena_alloc *alloc,
    error              *e
);

sb_size smartfiles_write (
    struct pager       *p,
    struct txn         *tx,
    struct stream      *src,
    t_size              size,
    sb_size             bofst,
    sb_size             stride,
    b_size              nelem,
    struct arena_alloc *alloc,
    error              *e
);

sb_size smartfiles_read (
    struct pager       *p,
    struct txn         *tx,
    struct stream      *dest,
    t_size              size,
    sb_size             bofst,
    sb_size             stride,
    b_size              nelem,
    struct arena_alloc *alloc,
    error              *e
);

sb_size smartfiles_remove (
    struct pager       *p,
    struct txn         *tx,
    struct stream      *dest,
    t_size              size,
    sb_size             bofst,
    sb_size             stride,
    b_size              nelem,
    struct arena_alloc *alloc,
    error              *e
);

#endif // NS_SMARTFILES_ALGORITHMS_H
