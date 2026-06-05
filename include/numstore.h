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

#include <inttypes.h>
#include <stdint.h>

#include "compile_config.h"

//////////////////////////////////////////////////
/// Types

typedef struct nsdb     nsdb_t;
typedef struct nsdb_var nsdb_var_t;

// In array notation [a:b:c]
#define STOP_PRESENT  (1 << 0) // [:c]
#define STEP_PRESENT  (1 << 1) // [:b:]
#define START_PRESENT (1 << 2) // [a:]
#define COLON_PRESENT (1 << 3) // [:]

//////////////////////////////////////////////////
/// Lifecycle and db tools

nsdb_t *nsdb_open (const char *path);
int     nsdb_cleanup (const char *path);
nsdb_t *nsdb_new_context (nsdb_t *ns);
int     nsdb_close (nsdb_t *ns);
int     nsdb_validate (nsdb_t *ns);

//////////////////////////////////////////////////
/// Error management

const char *nsdb_strerror (nsdb_t *ns);
int         nsdb_perror (nsdb_t *ns, const char *prefix);

//////////////////////////////////////////////////
/// Variable API

int     nsdb_delete (nsdb_t *ns, const char *vname);
int     nsdb_create (nsdb_t *ns, const char *name, const char *type);
sb_size nsdb_len (nsdb_t *ns, const char *vname);

//////////////////////////////////////////////////
/// Transactions

int nsdb_begin (nsdb_t *ns);
int nsdb_commit (nsdb_t *ns);
int nsdb_rollback (nsdb_t *ns);

//////////////////////////////////////////////////
/// Fetch a variable reference handle

nsdb_var_t *nsdb_get (nsdb_t *db, const char *name);
int         nsdb_get_if_exists (nsdb_t *db, nsdb_var_t **var, const char *name);
void        nsdb_free (nsdb_var_t *var);

//////////////////////////////////////////////////
/// Core Array Operations

sb_size nsdb_insert (
    nsdb_t     *ns,
    nsdb_var_t *var,
    const void *src,
    sb_size     ofst,
    b_size      slen
);

sb_size nsdb_write (
    nsdb_t     *ns,
    nsdb_var_t *var,
    const void *src,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
);

sb_size nsdb_read (
    nsdb_t     *ns,
    nsdb_var_t *var,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
);

sb_size nsdb_remove (
    nsdb_t     *ns,
    nsdb_var_t *var,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
);

//////////////////////////////////////////////////
/// Do the same operations on just a variable name

sb_size nsdb_vinsert (
    nsdb_t     *ns,
    const char *name,
    const void *src,
    sb_size     ofst,
    b_size      slen
);

sb_size nsdb_vwrite (
    nsdb_t     *ns,
    const char *name,
    const void *src,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
);

sb_size nsdb_vread (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
);

sb_size nsdb_vremove (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
);
