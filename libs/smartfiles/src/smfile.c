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

#include "smfile.h"

#include "_smfile.h"
#include "nscore/nshandle.h"
#include "nscore/pager.h"

#include <c_specx.h>

// smfile

int
smfile_perror (smfile_t *ns, const char *prefix)
{ return nsh_perror ((struct nshandle *)ns, prefix); }
const char *
smfile_strerror (smfile_t *ns)
{ return nsh_strerror ((struct nshandle *)ns); }

int
smfile_cleanup (const char *path)
{ return nsh_cleanup (path); }

// Core Operations
sb_size
smfile_size (smfile_t *smf)
{ return smfile_psize (smf, NULL); }

sb_size
smfile_insert (smfile_t *smf, const void *src, sb_size bofst, b_size slen)
{ return smfile_pinsert (smf, NULL, src, bofst, slen); }

sb_size
smfile_write (smfile_t *smf, const void *src, b_size bofst, b_size nelem)
{ return smfile_pwrite (smf, NULL, src, 1, bofst, 1, nelem); }

sb_size
smfile_read (smfile_t *smf, void *dest, sb_size bofst, b_size nelem)
{ return smfile_pread (smf, NULL, dest, 1, bofst, 1, nelem); }

sb_size
smfile_remove (smfile_t *smf, void *dest, sb_size bofst, b_size nelem)
{ return smfile_premove (smf, NULL, dest, 1, bofst, 1, nelem); }

smfile_t *
smfile_new_context (smfile_t *n)
{ return (smfile_t *)nsh_new_context ((struct nshandle *)n); }

int
smfile_close (smfile_t *ns)
{ return nsh_close ((struct nshandle *)ns); }
int
_smfile_crash (smfile_t *ns)
{ return nsh_crash ((struct nshandle *)ns); }

int
smfile_begin (smfile_t *_smf)
{ return nsh_begin ((struct nshandle *)_smf); }
int
smfile_commit (smfile_t *_smf)
{ return nsh_commit ((struct nshandle *)_smf); }
int
smfile_rollback (smfile_t *smf)
{ return nsh_rollback ((struct nshandle *)smf); }
