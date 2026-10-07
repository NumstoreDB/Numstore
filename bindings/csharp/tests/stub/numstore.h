#ifndef NUMSTORE_H
#define NUMSTORE_H
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
typedef struct nsdb      nsdb_t;
typedef struct txn       txn_t;
typedef struct nsdb_var  nsdb_var_t;
typedef struct nsdb_plan nsdb_plan_t;
typedef uint32_t         t_size;
typedef uint64_t         b_size;
typedef int64_t          sb_size;
nsdb_t *ns_open (const char *path);
nsdb_t *ns_smfile_open (const char *path);
int ns_cleanup (const char *path);
int ns_close (nsdb_t *ns);
int ns_crash (nsdb_t *ns);
b_size ns_var_len (nsdb_var_t *var);
void ns_var_free (nsdb_var_t *var);
const char *ns_strerror (nsdb_t *ns);
const char *ns_plan_strerror (nsdb_plan_t *plan);
txn_t *ns_begin (nsdb_t *ns);
int ns_commit (nsdb_t *ns, txn_t *txn);
int ns_rollback (nsdb_t *ns, txn_t *txn);
nsdb_plan_t *ns_plan_fcreate (nsdb_t *db, const char *fmt, ...);
void ns_plan_free (nsdb_plan_t *plan);
int ns_execute (nsdb_t *db, struct txn *tx, const char *fmt, ...);
int ns_plan_execute (nsdb_plan_t *plan, struct txn *tx);
nsdb_var_t *ns_get_var (nsdb_t *db, struct txn *tx, const char *query, ...);
nsdb_var_t *ns_plan_get_var (nsdb_plan_t *plan, struct txn *tx);
sb_size ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...);
sb_size ns_plan_read (nsdb_plan_t *plan, txn_t *txn, void *dest, b_size dlen);
sb_size ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...);
sb_size ns_plan_write (nsdb_plan_t *plan, txn_t *tx, const void *src, b_size dlen);
void *ns_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...);
void *ns_plan_malloc (nsdb_plan_t *plan, txn_t *tx, b_size *dlen);
sb_size ns_smfile_size (nsdb_t *smf, txn_t *tx);
sb_size ns_smfile_insert (nsdb_t *smf, txn_t *tx, const void *src, sb_size bofst, b_size slen);
sb_size ns_smfile_write (
    nsdb_t     *smf,
    txn_t      *tx,
    const void *src,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);
sb_size ns_smfile_read (
    nsdb_t *smf,
    txn_t  *tx,
    void   *dest,
    t_size  size,
    sb_size bofst,
    sb_size stride,
    b_size  nelem
);
sb_size ns_smfile_remove (
    nsdb_t *smf,
    txn_t  *tx,
    void   *dest,
    t_size  size,
    sb_size bofst,
    sb_size stride,
    b_size  nelem
);
#endif
