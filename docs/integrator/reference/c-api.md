# C API Reference

Numstore is a C11 library with one public header, `include/numstore.h`, and one
static archive, `libnumstore.a`. There are no dependencies beyond libc, pthreads
and libm.

If you are new to Numstore, read [the tour](../../index.md) first - it explains the
query language and the type system this page assumes. It is written in Python,
but the queries are identical; only the host language changes. For the Python
bindings themselves, see [the Python API](python-api.md).

Every program on this page is complete and compiles as shown, and every output
block is what it actually printed.

## Transactions

`ns_begin` opens a transaction; pass it where you were passing `NULL` and the
queries become one atomic unit. `ns_commit` makes it durable, `ns_rollback`
throws it away, and reads inside the transaction see its own uncommitted work:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

static void
print_prices (nsdb_t *db, const char *label)
{
  double  out[8];
  sb_size n = ns_read (db, NULL, out, sizeof (out), "read prices[0:]");
  printf ("%s:", label);
  for (sb_size i = 0; i < n; i++) {
    printf (" %g", out[i]);
  }
  printf ("\n");
}

int
main (void)
{
  ns_cleanup ("txn.db");
  nsdb_t *db = ns_open ("txn.db");
  ns_execute (db, NULL, "create prices f64");

  // Commit: both inserts land together, or neither does.
  txn_t *tx = ns_begin (db);
  if (tx == NULL) {
    fprintf (stderr, "begin failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }

  const double first[] = {1.5, 2.25};
  const double second[] = {3.75};
  ns_write (db, tx, first, sizeof (first), "insert prices 0 2");
  ns_write (db, tx, second, sizeof (second), "insert prices 2 1");

  if (ns_commit (db, tx) != 0) {
    fprintf (stderr, "commit failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }
  print_prices (db, "after commit ");

  // Rollback: the insert is thrown away.
  tx = ns_begin (db);
  const double third[] = {99.0};
  ns_write (db, tx, third, sizeof (third), "insert prices 0 1");

  double  inside[8];
  sb_size n = ns_read (db, tx, inside, sizeof (inside), "read prices[0:]");
  printf ("inside txn  : %" PRId64 " elements, first %g\n", n, inside[0]);

  ns_rollback (db, tx);
  print_prices (db, "after rollback");

  return ns_close (db);
}
```

```text
after commit : 1.5 2.25 3.75
inside txn  : 4 elements, first 99
after rollback: 1.5 2.25 3.75
```

A `txn_t *` is spent once you commit or roll it back - don't reuse it. Finish
every transaction you open: `ns_close` blocks while one is still open, and
`ns_begin` blocks while another is still open. See
[Concurrency and threading](#concurrency-and-threading).

## Variable metadata

`ns_get_var` hands you a handle to one variable's metadata. Any query that names
a variable works and the query itself never runs, so you can ask what a
destructive query *would* touch. Free the handle with `ns_var_free`;
`ns_visit_nsdb_vars` walks every variable in the database instead:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

struct total
{
  unsigned seen;
  b_size   elements;
};

static void
tally (nsdb_var_t *var, void *ctx)
{
  struct total *t = ctx;
  t->seen++;
  t->elements += ns_var_len (var);
  // Do not call ns_var_free here: the visitor owns this handle.
}

int
main (void)
{
  ns_cleanup ("vars.db");
  nsdb_t *db = ns_open ("vars.db");

  ns_execute (db, NULL, "create v1 i32");
  ns_execute (db, NULL, "create v2 f64");

  const int32_t src[] = {1, 2, 3};
  ns_write (db, NULL, src, sizeof (src), "insert v1 0 3");

  // One variable at a time.
  nsdb_var_t *var = ns_get_var (db, NULL, "get v1");
  if (var == NULL) {
    fprintf (stderr, "get failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }
  printf ("v1 has %" PRIu64 " elements\n", ns_var_len (var));
  ns_var_free (var);

  // Any query that names a variable works, and the query never runs.
  var = ns_get_var (db, NULL, "remove v1[0:]");
  printf ("v1 still has %" PRIu64 " elements\n", ns_var_len (var));
  ns_var_free (var);

  // Every variable in the database.
  struct total t = {0};
  if (ns_visit_nsdb_vars (db, tally, NULL, &t) != 0) {
    fprintf (stderr, "visit failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }
  printf ("%u variables, %" PRIu64 " elements total\n", t.seen, t.elements);

  return ns_close (db);
}
```

```text
v1 has 3 elements
v1 still has 3 elements
2 variables, 3 elements total
```

Two ownership rules worth keeping straight:

* A handle from `ns_get_var` is yours, and `ns_var_free` releases it. Variables
  get their own memory, so you may free them before or after `ns_close`.
* A handle passed to a `ns_visit_nsdb_vars` callback is **not** yours. Don't
  free it, and don't keep it past the callback - copy out what you need.

`ns_visit_nsdb_vars` takes a transaction argument for symmetry but ignores it
today: the walk always sees committed state.

In C, `ns_var_len` is the whole metadata interface - length in elements. The
type and element size are not exposed as C accessors; when you need those, read
them from the type you wrote in the `create` query, or use
[`Var` in Python](python-api.md).

## Concurrency and threading

Numstore serializes transactions strictly: **one transaction is open at a time
per handle**. `ns_begin` blocks until the open one commits or rolls back, which
is what makes transactions strictly serializable - the schedule is the order
they commit in, with no interleaving to reason about.

What that means in practice:

* Finish each transaction before opening the next one on the same handle. Call
  `ns_begin` twice without committing and the second call blocks forever on a
  transaction only you can finish.
* `ns_close` blocks while a transaction is open, so commit or roll back first.
* Don't open the same file twice, in one process or across processes. Both calls
  succeed today, but the second handle does not see the first handle's commits.
  Treat a database file as owned by one handle at a time.

Concurrent readers and writers are in development. Until they land, the
supported shape is one handle per file with one transaction in flight at a time;
a crash at any point is still safe, which is what
[recovery](../how-to/c-recover-after-a-crash.md) is for.

## Function reference

### Lifecycle

| Function | Returns | Notes |
|----------|---------|-------|
| `nsdb_t *ns_open (const char *path)` | handle, `NULL` on failure | opens or creates a database |
| `nsdb_t *ns_open_with_error (const char *path, char erstr[], unsigned elen)` | handle, `NULL` on failure | same, and copies the failure message into `erstr`; zero the buffer first |
| `nsdb_t *ns_smfile_open (const char *path)` | handle, `NULL` on failure | opens or creates a smart file |
| `nsdb_t *ns_smfile_open_with_error (const char *path, char erstr[], unsigned elen)` | handle, `NULL` on failure | same, with a message |
| `int ns_close (nsdb_t *ns)` | `0` on success | flushes and closes; blocks while a transaction is open |
| `int ns_crash (nsdb_t *ns)` | `0` on success | drops the handle without flushing; open transactions are rolled back on the next open |
| `int ns_cleanup (const char *path)` | `0` on success | deletes the database or smart file at `path` |

### Queries

| Function | Returns | Notes |
|----------|---------|-------|
| `int ns_execute (nsdb_t *db, txn_t *tx, const char *fmt, ...)` | `0` on success | `CREATE` `DELETE` `REMOVE` |
| `sb_size ns_read (nsdb_t *db, txn_t *tx, void *dest, b_size dlen, const char *fmt, ...)` | elements read, `< 0` on failure | `READ` `REMOVE`; `dlen` is bytes |
| `sb_size ns_write (nsdb_t *db, txn_t *tx, const void *src, b_size slen, const char *fmt, ...)` | elements written, `< 0` on failure | `INSERT` `WRITE`; `slen` is bytes |
| `void *ns_malloc (nsdb_t *db, txn_t *tx, b_size *dlen, const char *fmt, ...)` | buffer, `NULL` on failure | `READ` `REMOVE`; writes the byte count to `dlen`; release with `free` |
| `nsdb_var_t *ns_get_var (nsdb_t *db, txn_t *tx, const char *query, ...)` | handle, `NULL` on failure | any query naming a variable; the query never runs |

### Transactions

| Function | Returns | Notes |
|----------|---------|-------|
| `txn_t *ns_begin (nsdb_t *ns)` | handle, `NULL` on failure | blocks while another transaction is open |
| `int ns_commit (nsdb_t *ns, txn_t *txn)` | `0` on success | durable when it returns; spends the handle |
| `int ns_rollback (nsdb_t *ns, txn_t *txn)` | `0` on success | undoes everything; spends the handle |

### Variables

| Function | Returns | Notes |
|----------|---------|-------|
| `b_size ns_var_len (nsdb_var_t *var)` | elements | length when the handle was taken |
| `void ns_var_free (nsdb_var_t *var)` | | frees a handle from `ns_get_var`; may run after `ns_close` |
| `int ns_visit_nsdb_vars (nsdb_t *db, nsdb_var_consumer consumer, txn_t *tx, void *ctx)` | `0` on success | calls `consumer (var, ctx)` per variable; the callback must not free `var`; `tx` is ignored |

### Plans

| Function | Returns | Notes |
|----------|---------|-------|
| `nsdb_plan_t *ns_plan_fcreate (nsdb_t *db, const char *fmt, ...)` | plan, `NULL` on failure | compiles a query |
| `void ns_plan_free (nsdb_plan_t *plan)` | | free before closing the database |
| `int ns_plan_execute (nsdb_plan_t *plan, txn_t *tx)` | `0` on success | |
| `sb_size ns_plan_read (nsdb_plan_t *plan, txn_t *tx, void *dest, b_size dlen)` | elements read, `< 0` on failure | |
| `sb_size ns_plan_write (nsdb_plan_t *plan, txn_t *tx, const void *src, b_size slen)` | elements written, `< 0` on failure | |
| `void *ns_plan_malloc (nsdb_plan_t *plan, txn_t *tx, b_size *dlen)` | buffer, `NULL` on failure | release with `free` |
| `nsdb_var_t *ns_plan_get_var (nsdb_plan_t *plan, txn_t *tx)` | handle, `NULL` on failure | free with `ns_var_free` |

### Errors

| Function | Returns | Notes |
|----------|---------|-------|
| `const char *ns_strerror (nsdb_t *ns)` | message | describes the last failure on this handle |
| `const char *ns_plan_strerror (nsdb_plan_t *plan)` | message | describes the last failure on this plan |

### Smart files

| Function | Returns | Notes |
|----------|---------|-------|
| `sb_size ns_smfile_size (nsdb_t *smf, txn_t *tx)` | bytes | size under `tx`'s view |
| `sb_size ns_smfile_insert (nsdb_t *smf, txn_t *tx, const void *src, sb_size bofst, b_size slen)` | bytes inserted, `< 0` on failure | pushes `slen` bytes in at byte offset `bofst` |
| `sb_size ns_smfile_write (nsdb_t *smf, txn_t *tx, const void *src, t_size size, sb_size bofst, sb_size stride, b_size nelem)` | elements written, `< 0` on failure | overwrites `nelem` elements of `size` bytes, stepping `stride` |
| `sb_size ns_smfile_read (nsdb_t *smf, txn_t *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)` | elements read, `< 0` on failure | same addressing, reading |
| `sb_size ns_smfile_remove (nsdb_t *smf, txn_t *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)` | elements removed, `< 0` on failure | removes and optionally hands the bytes back in `dest` |

## Types and constants

| Name | Underlying | What it is |
|------|------------|------------|
| `nsdb_t` | opaque | a database or smart file handle |
| `txn_t` | opaque | a transaction |
| `nsdb_var_t` | opaque | a variable metadata handle |
| `nsdb_plan_t` | opaque | a compiled query |
| `t_size` | `uint32_t` | the size of one type, in bytes |
| `b_size` | `uint64_t` | a byte count or byte offset |
| `sb_size` | `int64_t` | a signed `b_size`; negative means failure |
| `nsdb_var_consumer` | `void (*) (nsdb_var_t *, void *)` | the `ns_visit_nsdb_vars` callback |
| `MAX_ERRSTR_LEN` | `256` | big enough for any Numstore error message |
| `NS_END` | `INT64_MAX` | as a smart file `nelem`, "to the end of the file" - see the warning in [Smart files](../how-to/c-use-smart-files.md) |

Print `b_size` with `PRIu64` and `sb_size` with `PRId64` from `<inttypes.h>`,
which `numstore.h` already includes.
