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
#include "platform.h"

#define NS_END INT64_MAX

//////////////////////////////////////////////////
/// Types

typedef struct nsdb     nsdb_t;
typedef struct nsdb_var nsdb_var_t;

//////////////////////////////////////////////////
/// Lifecycle and db tools

nsdb_t *nsdb_open (const char *path);
int     nsdb_cleanup (const char *path);
int     nsdb_close (nsdb_t *ns);
int     nsdb_crash (nsdb_t *ns);

//////////////////////////////////////////////////
/// Variable manipulation

b_size nsdb_var_len (nsdb_var_t *var);
void   nsdb_var_free (nsdb_var_t *var);

//////////////////////////////////////////////////
/// Error management

const char *nsdb_strerror (nsdb_t *ns);
int         nsdb_perror (nsdb_t *ns, const char *prefix);

//////////////////////////////////////////////////
/// Transaction control

int nsdb_begin (nsdb_t *ns);
int nsdb_commit (nsdb_t *ns);
int nsdb_rollback (nsdb_t *ns);

//////////////////////////////////////////////////
/// Execute an operation

sb_size
nsdb_execute (nsdb_t *ns, const char *query, void *data, ...) PRINTF_ATTR (
    2,
    4
);
