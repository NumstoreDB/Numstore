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

#include "alloc.h"
#include "error.h"
#include "nsdb.h"
#include "numstore.h"
#include "os.h"
#include "pager.h"
#include "rope_algorithms.h"
#include "var_algorithms.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

// smfile

int
smfile_perror (smfile_t *ns, const char *prefix)
{
  return nsdb_perror ((struct nsdb *)ns, prefix);
}

#ifdef TESTING
TEST (smfile_perror)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  // stride == 0 => ERROR
  test_assert (smfile_pread (s, "foo", buffer, 10, 0, 0, 10) < 0);
  test_assert (smfile_perror (s, "bar") > 0);

  smfile_close (s);
}
#endif

const char *
smfile_strerror (smfile_t *ns)
{
  return nsdb_strerror ((struct nsdb *)ns);
}

#ifdef TESTING
TEST (smfile_strerror)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  // stride == 0 => ERROR
  test_assert (smfile_pread (s, "foo", buffer, 10, 0, 0, 10) < 0);
  test_assert (
      string_contains (strfcstr (smfile_strerror (s)), strfcstr ("stride == 0"))
  );

  smfile_close (s);
}
#endif

int
smfile_cleanup (const char *path)
{
  return nsdb_cleanup (path);
}

#ifdef TESTING
TEST (smfile_cleanup)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  smfile_close (s);
  error e = error_create ();

  bool exists;
  i_file_exists ("test", &exists, &e);
  test_assert (exists);

  smfile_cleanup ("test");
  i_file_exists ("test", &exists, &e);
  test_assert (!exists);
}
#endif

sb_size
smfile_size (smfile_t *smf)
{
  return smfile_psize (smf, NULL);
}

#ifdef TESTING
TEST (smfile_size)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");

  test_assert_equal (smfile_size (s), 0);

  u8 buffer[2048];
  smfile_insert (s, buffer, 0, sizeof (buffer));

  test_assert_equal (smfile_size (s), sizeof (buffer));

  smfile_insert (s, buffer, 0, sizeof (buffer));

  test_assert_equal (smfile_size (s), 2 * sizeof (buffer));

  smfile_close (s);
}
#endif

int
smfile_close (smfile_t *ns)
{
  return nsdb_close ((struct nsdb *)ns);
}

#ifdef TESTING
TEST (smfile_close)
{
  smfile_cleanup ("test");

  error          e = error_create ();
  struct smfile *s = smfile_open ("test");
  smfile_close (s);

  bool exists;
  i_file_exists ("test", &exists, &e);
  test_assert (exists);
  i_file_exists ("test.wal", &exists, &e);
  test_assert (!exists);
}
#endif

int
smfile_crash (smfile_t *ns)
{
  return nsdb_crash ((struct nsdb *)ns);
}

#ifdef TESTING
TEST (smfile_crash)
{
  smfile_cleanup ("test");

  error          e = error_create ();
  struct smfile *s = smfile_open ("test");
  smfile_crash (s);

  bool exists;
  i_file_exists ("test", &exists, &e);
  test_assert (exists);
  i_file_exists ("test.wal", &exists, &e);
  test_assert (exists);
}
#endif

int
smfile_begin (smfile_t *_smf)
{
  return nsdb_begin ((struct nsdb *)_smf);
}

int
smfile_commit (smfile_t *_smf)
{
  return nsdb_commit ((struct nsdb *)_smf);
}

int
smfile_rollback (smfile_t *smf)
{
  return nsdb_rollback ((struct nsdb *)smf);
}

#ifdef TESTING
TEST (smfile_txns)
{
  smfile_cleanup ("test");

  u8             buffer[2048];
  error          e = error_create ();
  struct smfile *s = smfile_open ("test");

  test_assert_equal (smfile_size (s), 0);

  smfile_begin (s);
  smfile_insert (s, buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_size (s), sizeof (buffer));
  smfile_commit (s);
  test_assert_equal (smfile_size (s), sizeof (buffer));

  smfile_begin (s);
  smfile_insert (s, buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_size (s), 2 * sizeof (buffer));
  smfile_rollback (s);
  test_assert_equal (smfile_size (s), sizeof (buffer));

  smfile_close (s);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Delete

int
smfile_delete (smfile_t *_smf, const char *vname)
{
  struct nsdb *smf = (struct nsdb *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  error *e = &smf->e;

  struct txn auto_txn;

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (smf, e), failed);

  i_log_debug ("DELETE (txn = %" PRtxid "): %s\n", smf->atx->tid, vname);

  struct string vnamestr = strfcstr (vname);
  {
    // DELETE
    struct ns_var_delete_params params = {
        .p     = smf->root->p,
        .tx    = smf->atx,
        .vname = strfcstr (vname),
    };
    err_t err = ns_var_delete (params, e);
    if (err == ERR_VARIABLE_NE)
    {
      // It's ok - just return the error
      goto commit;
    }
    if (err < SUCCESS)
    {
      goto failed_rollback;
    }
  }

commit:

  if (nsdb_auto_commit (smf, e))
  {
    goto failed_rollback;
  }
  return error_trace (e);

failed_rollback:

  nsdb_auto_rollback (smf);

failed:
  return error_trace (e);
}

#ifdef TESTING
TEST (smfile_delete)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_psize (s, "foo"), sizeof (buffer));

  smfile_delete (s, "foo");
  test_assert (smfile_psize (s, "foo") < 0);

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_psize (s, "foo"), sizeof (buffer));

  smfile_close (s);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Open

smfile_t *
smfile_open (const char *path)
{
  struct nsdb *ret = nsdb_open (path);

  if (ret == NULL)
  {
    return NULL;
  }

  // Create the default variable
  if (pgr_isnew (ret->root->p))
  {
    // BEGIN TXN
    struct txn tx;
    if (pgr_begin_txn (&tx, ret->root->p, &ret->e))
    {
      goto failed;
    }

    struct ns_var_create_params params = {
        .p     = ret->root->p,
        .tx    = &tx,
        .vname = strfcstr (DEFAULT_VARIABLE),
        .type  = &(struct type){.type = T_PRIM, .p = U8},
    };
    if (ns_var_create (params, &ret->e))
    {
      goto failed;
    }

    // COMMIT
    if (pgr_commit (ret->root->p, &tx, &ret->e))
    {
      goto failed;
    }
  }

  return (smfile_t *)ret;

failed:
  nsdb_close (ret);

  return NULL;
}

#ifdef TESTING
TEST (smfile_open)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  test_assert (s != NULL);
  test_assert_equal (smfile_size (s), 0);

  smfile_close (s);

  // Reopening an existing file should succeed and preserve its data.
  struct smfile *s2 = smfile_open ("test");
  test_assert (s2 != NULL);
  test_assert_equal (smfile_size (s2), 0);

  smfile_close (s2);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Insert

sb_size
smfile_pinsert (
    smfile_t   *_smf,
    const char *name,
    const void *src,
    sb_size     bofst,
    b_size      slen
)
{
  struct nsdb *smf = (struct nsdb *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  error *e = &smf->e;

  ALLOC_INIT (temp);

  sb_size                            ret;        // Return value
  b_size                             ofst;       // Resolved offset
  struct stream                      _input;     // Input stream
  struct stream_ibuf_ctx             ctx;        // Context for input stream
  struct ns_var_get_or_create_params gparams;    // Get or create operation
  struct ns_insert_params            iparams;    // Insert operation
  struct ns_var_update_params        uparams;    // Update operation
  struct string vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (slen == 0)
  {
    return 0;
  }

  stream_ibuf_init (&_input, &ctx, src, slen);

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (smf, e), failed);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = smf->root->p,
        .tx    = smf->atx,
        .vname = vname,
        .type  = &(struct type){.type = T_PRIM, .p = U8},
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    ofst = var_resolve_index (&gparams.dest, bofst);
  }

  // INSERT
  {
    iparams = (struct ns_insert_params){
        .p     = smf->root->p,
        .src   = &_input,
        .tx    = smf->atx,
        .root  = gparams.dest.rpt_root,
        .bofst = bofst,
    };
    ret = ns_insert (&iparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // UPDATE VARIABLE
  {
    uparams = (struct ns_var_update_params){
        .p  = smf->root->p,
        .tx = smf->atx,
        .retr =
            (struct var_retrieval){
                .type = VR_PG,
                .root = gparams.dest.var_root,
            },
        .newpg  = iparams.root,
        .nbytes = gparams.dest.nbytes + ret,
    };
    WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (smf, e), failed_rollback);

  ALLOC_CLOSE (temp);

  return ret;

failed_rollback:

  nsdb_auto_rollback (smf);

failed:
  ALLOC_CLOSE (temp);

  return error_trace (e);
}

#ifdef TESTING
TEST (smfile_pinsert)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_psize (s, "foo"), sizeof (buffer));

  smfile_close (s);
}
#endif

sb_size
smfile_insert (smfile_t *smf, const void *src, sb_size bofst, b_size slen)
{
  return smfile_pinsert (smf, NULL, src, bofst, slen);
}

#ifdef TESTING
TEST (smfile_insert)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  smfile_insert (s, buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_size (s), sizeof (buffer));

  smfile_close (s);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Read

sb_size
smfile_pread (
    smfile_t   *_smf,
    const char *name,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  struct nsdb *smf = (struct nsdb *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  error *e = &smf->e;

  ALLOC_INIT (temp);

  sb_size                  ret;           // Return value
  b_size                   ofst;          // Resolved offset
  struct stream            _output;       // Output stream if present
  struct stream_obuf_ctx   ctx;           // Context for output stream
  struct stream           *output = NULL; // Pointer to output stream
  struct ns_var_get_params gparams;       // Get operation
  struct ns_read_params    rparams;       // Read operation
  struct string            vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (stride < 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Negative strides aren't supported yet"
    );
  }
  if (stride == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot read with stride == 0"
    );
  }
  if (size == 0)
  {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot read with size == 0");
  }
  if (nelem == 0)
  {
    return 0;
  }

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (smf, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = smf->root->p,
        .tx    = smf->atx,
        .vname = vname,
        .alloc = &temp,
    };
    err_t err = ns_var_get (&gparams, e);
    if (err == ERR_VARIABLE_NE)
    {
      ret           = 0;
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);
  }

  // Resolve sizes
  {
    ofst  = var_resolve_index (&gparams.dest, bofst);
    nelem = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
    if (nelem == 0)
    {
      ret = 0;
      goto commit;
    }
    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, size * nelem);
      output = &_output;
    }
  }

  // READ
  {
    rparams = (struct ns_read_params){
        .p      = smf->root->p,
        .dest   = output,
        .tx     = smf->atx,
        .root   = gparams.dest.rpt_root,
        .size   = size,
        .bofst  = ofst,
        .stride = stride,
        .nelem  = nelem,
    };
    ret = ns_read (rparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

commit:

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (smf, e), failed_rollback);
  ALLOC_CLOSE (temp);
  return ret;

failed_rollback:

  nsdb_auto_rollback (smf);

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}

#ifdef TESTING
TEST (smfile_pread)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[16];
  for (u32 i = 0; i < sizeof (buffer); i++)
  {
    buffer[i] = (u8)i;
  }

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));

  u8      out[16] = {0};
  sb_size n       = smfile_pread (s, "foo", out, 1, 0, 1, sizeof (buffer));

  test_assert_equal (n, sizeof (buffer));
  test_assert (memcmp (out, buffer, sizeof (buffer)) == 0);

  smfile_close (s);
}
#endif

sb_size
smfile_read (smfile_t *smf, void *dest, sb_size bofst, b_size nelem)
{
  return smfile_pread (smf, NULL, dest, 1, bofst, 1, nelem);
}

#ifdef TESTING
TEST (smfile_read)
{
  smfile_cleanup ("test");

  struct smfile *s         = smfile_open ("test");
  u8             buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  smfile_insert (s, buffer, 0, sizeof (buffer));

  u8      out[8] = {0};
  sb_size n      = smfile_read (s, out, 0, sizeof (buffer));

  test_assert_equal (n, sizeof (buffer));
  test_assert (memcmp (out, buffer, sizeof (buffer)) == 0);

  smfile_close (s);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Remove

sb_size
smfile_premove (
    smfile_t   *_smf,
    const char *name,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  struct nsdb *smf = (struct nsdb *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  error *e = &smf->e;

  ALLOC_INIT (temp);

  sb_size                     ret;           // Return value
  b_size                      ofst;          // Resolved offset
  struct stream               _output;       // Output stream if present
  struct stream_obuf_ctx      ctx;           // Context for output stream
  struct stream              *output = NULL; // Pointer to output stream
  struct ns_var_get_params    gparams;       // Get operation
  struct ns_remove_params     rparams;       // Remove operation
  struct ns_var_update_params uparams;       // Update operation
  struct string               vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (stride < 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Negative strides aren't supported yet"
    );
  }
  if (stride == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot remove with stride == 0"
    );
  }
  if (size == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot remove with size == 0"
    );
  }
  if (nelem == 0)
  {
    return 0;
  }

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (smf, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = smf->root->p,
        .tx    = smf->atx,
        .vname = vname,
        .alloc = &temp,
    };
    err_t err = ns_var_get (&gparams, e);
    if (err == ERR_VARIABLE_NE)
    {
      ret           = 0;
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);
  }

  // Resolve sizes
  {
    ofst  = var_resolve_index (&gparams.dest, bofst);
    nelem = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
    if (nelem == 0)
    {
      ret = 0;
      goto commit;
    }
    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, size * nelem);
      output = &_output;
    }
  }

  // REMOVE
  {
    rparams = (struct ns_remove_params){
        .p      = smf->root->p,
        .dest   = output,
        .tx     = smf->atx,
        .root   = gparams.dest.rpt_root,
        .size   = size,
        .bofst  = ofst,
        .stride = stride,
        .nelem  = nelem,
    };
    ret = ns_remove (&rparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // UPDATE VARIABLE
  {
    uparams = (struct ns_var_update_params){
        .p  = smf->root->p,
        .tx = smf->atx,
        .retr =
            (struct var_retrieval){
                .type = VR_PG,
                .root = gparams.dest.var_root,
            },
        .newpg  = rparams.root,
        .nbytes = gparams.dest.nbytes - ret * size,
    };
    WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
  }

commit:

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (smf, e), failed_rollback);
  ALLOC_CLOSE (temp);
  return ret;

failed_rollback:

  nsdb_auto_rollback (smf);

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}

#ifdef TESTING
TEST (smfile_premove)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[16];
  for (u32 i = 0; i < sizeof (buffer); i++)
  {
    buffer[i] = (u8)i;
  }

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_psize (s, "foo"), sizeof (buffer));

  u8      out[16] = {0};
  sb_size n       = smfile_premove (s, "foo", out, 1, 0, 1, sizeof (buffer));

  test_assert_equal (n, sizeof (buffer));
  test_assert (memcmp (out, buffer, sizeof (buffer)) == 0);
  test_assert_equal (smfile_psize (s, "foo"), 0);

  smfile_close (s);
}
#endif

sb_size
smfile_remove (smfile_t *smf, void *dest, sb_size bofst, b_size nelem)
{
  return smfile_premove (smf, NULL, dest, 1, bofst, 1, nelem);
}

#ifdef TESTING
TEST (smfile_remove)
{
  smfile_cleanup ("test");

  struct smfile *s         = smfile_open ("test");
  u8             buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  smfile_insert (s, buffer, 0, sizeof (buffer));

  u8      out[8] = {0};
  sb_size n      = smfile_remove (s, out, 0, sizeof (buffer));

  test_assert_equal (n, sizeof (buffer));
  test_assert (memcmp (out, buffer, sizeof (buffer)) == 0);
  test_assert_equal (smfile_size (s), 0);

  smfile_close (s);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Write

sb_size
smfile_pwrite (
    smfile_t   *_smf,
    const char *name,
    const void *src,
    t_size      size,
    b_size      bofst,
    sb_size     stride,
    b_size      nelem
)
{
  struct nsdb *smf = (struct nsdb *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  error *e = &smf->e;

  ALLOC_INIT (temp);

  sb_size                ret;          // Return value
  sb_size                inserted;     // Number of bytes inserted
  b_size                 ofst;         // Resolved offset
  b_size                 write_nelem;  // Elements that fit in existing variable
  b_size                 insert_nelem; // Remainder to insert past the end
  struct stream          _input;       // Input stream
  struct stream_ibuf_ctx ctx;          // Context for input stream
  struct ns_var_get_or_create_params gparams;    // Get or create operation
  struct ns_write_params             wparams;    // Write operation
  struct ns_insert_params            iparams;    // Insert operation
  struct ns_var_update_params        uparams;    // Update operation
  struct string vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (stride < 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Negative strides aren't supported yet"
    );
  }
  if (stride == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot write with stride == 0"
    );
  }
  if (size == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot write with size == 0"
    );
  }
  if (nelem == 0)
  {
    return 0;
  }

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (smf, e), failed);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = smf->root->p,
        .tx    = smf->atx,
        .vname = vname,
        .type  = &(struct type){.type = T_PRIM, .p = U8},
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    ofst         = var_resolve_index (&gparams.dest, bofst);
    write_nelem  = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
    insert_nelem = nelem - write_nelem;
    if (insert_nelem > 0 && stride != 1)
    {
      error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "Cannot write past end with stride != 1"
      );
      goto failed_rollback;
    }
  }

  // WRITE
  {
    stream_ibuf_init (&_input, &ctx, src, size * write_nelem);

    wparams = (struct ns_write_params){
        .p      = smf->root->p,
        .src    = &_input,
        .tx     = smf->atx,
        .root   = gparams.dest.rpt_root,
        .size   = size,
        .bofst  = ofst,
        .stride = stride,
        .nelem  = write_nelem,
    };

    ret = ns_write (wparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // INSERT REMAINDER
  if (insert_nelem > 0)
  {
    // INSERT
    {
      stream_ibuf_init (
          &_input,
          &ctx,
          (u8 *)src + write_nelem * size,
          insert_nelem * size
      );

      iparams = (struct ns_insert_params){
          .p     = smf->root->p,
          .src   = &_input,
          .tx    = smf->atx,
          .root  = wparams.root,
          .bofst = gparams.dest.nbytes, // Append
      };

      inserted = ns_insert (&iparams, e);
      WRAP_GOTO (inserted, failed_rollback);
      ret += inserted;
    }

    // UPDATE VARIABLE
    {
      uparams = (struct ns_var_update_params){
          .p  = smf->root->p,
          .tx = smf->atx,
          .retr =
              (struct var_retrieval){
                  .type = VR_PG,
                  .root = gparams.dest.var_root,
              },
          .newpg  = iparams.root,
          .nbytes = gparams.dest.nbytes + inserted,
      };
      WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
    }
  }

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (smf, e), failed_rollback);
  ALLOC_CLOSE (temp);
  return ret;

failed_rollback:

  nsdb_auto_rollback (smf);

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}

#ifdef TESTING
TEST (smfile_pwrite)
{
  smfile_cleanup ("test");

  struct smfile *s         = smfile_open ("test");
  u8             buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));

  // Overwrite the first 4 bytes in place.
  u8      overwrite[4] = {9, 9, 9, 9};
  sb_size n = smfile_pwrite (s, "foo", overwrite, 1, 0, 1, sizeof (overwrite));

  test_assert_equal (n, sizeof (overwrite));
  test_assert_equal (smfile_psize (s, "foo"), sizeof (buffer));

  u8 out[8] = {0};
  smfile_pread (s, "foo", out, 1, 0, 1, sizeof (out));

  u8 expected[8] = {9, 9, 9, 9, 5, 6, 7, 8};
  test_assert (memcmp (out, expected, sizeof (expected)) == 0);

  // Writing past the end should append (insert the remainder).
  u8 append[4] = {11, 12, 13, 14};
  n = smfile_pwrite (s, "foo", append, 1, sizeof (buffer), 1, sizeof (append));

  test_assert_equal (n, sizeof (append));
  test_assert_equal (
      smfile_psize (s, "foo"),
      sizeof (buffer) + sizeof (append)
  );

  smfile_close (s);
}
#endif

sb_size
smfile_write (smfile_t *smf, const void *src, b_size bofst, b_size nelem)
{
  return smfile_pwrite (smf, NULL, src, 1, bofst, 1, nelem);
}

#ifdef TESTING
TEST (smfile_write)
{
  smfile_cleanup ("test");

  struct smfile *s         = smfile_open ("test");
  u8             buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  smfile_insert (s, buffer, 0, sizeof (buffer));

  u8 overwrite[4] = {9, 9, 9, 9};
  smfile_write (s, overwrite, 0, sizeof (overwrite));

  u8 out[8] = {0};
  smfile_read (s, out, 0, sizeof (out));

  u8 expected[8] = {9, 9, 9, 9, 5, 6, 7, 8};
  test_assert (memcmp (out, expected, sizeof (expected)) == 0);

  smfile_close (s);
}
#endif

/////////////////////////////////////////////////////////////////////
////// Size

sb_size
smfile_psize (smfile_t *_smf, const char *name)
{
  struct nsdb *smf = (struct nsdb *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  error *e = &smf->e;

  ALLOC_INIT (temp);

  struct string vname = vname_or_default (name);
  b_size        ret;

  // BEGIN TXN
  if (nsdb_auto_begin_txn (smf, e) < 0)
  {
    goto failed;
  }

  // GET
  struct ns_var_get_params gparams = {
      .p     = smf->root->p,
      .tx    = smf->atx,
      .vname = vname,
      .alloc = &temp,
  };
  if (ns_var_get (&gparams, e))
  {
    goto failed;
  }

  ret = gparams.dest.nbytes;

  // COMMIT
  if (nsdb_auto_commit (smf, e) < 0)
  {
    goto failed_rollback;
  }

  ALLOC_CLOSE (temp);

  return ret;

failed_rollback:

  nsdb_auto_rollback (smf);

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}

#ifdef TESTING
TEST (smfile_psize)
{
  smfile_cleanup ("test");

  struct smfile *s = smfile_open ("test");
  u8             buffer[2048];

  smfile_pinsert (s, "foo", buffer, 0, sizeof (buffer));
  test_assert_equal (smfile_psize (s, "foo"), sizeof (buffer));

  smfile_close (s);
}
#endif
